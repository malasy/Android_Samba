#include "system/network.h"

#include <errno.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <stdlib.h>

#define MAX_SIZE 8192
char *__netlink_data_;

struct ifaddrs_container {
  // This needs to be the first because we use this struct as a ifaddrs struct.
  struct ifaddrs ifa;

  int idx;

  // Storage for pointers in ifa
  struct sockaddr_storage addr;
  struct sockaddr_storage netmask;
  struct sockaddr_storage dstaddr;
  char name[IFNAMSIZ + 1];
};

static void init(struct ifaddrs_container *ifa, struct ifaddrs_container** ifap) {
  memset(ifa, 0, sizeof(*ifa));

  ifa->ifa.ifa_next = (struct ifaddrs*)(*ifap);
  *ifap = ifa;
}

static uint8_t* addr_bytes(int family, struct sockaddr_storage *ss) {
  switch (family) {
    case AF_INET: {
      struct sockaddr_in *ss4 = (struct sockaddr_in *)ss;
      return (uint8_t*) (&ss4->sin_addr);
    }
    case AF_INET6: {
      struct sockaddr_in6 *ss6 = (struct sockaddr_in6 *)ss;
      return (uint8_t*) (&ss6->sin6_addr);
    }
    case AF_PACKET: {
      struct sockaddr_ll* sll = (struct sockaddr_ll*)ss;
      return (uint8_t*) (&sll->sll_addr);
    }
    default:
      return NULL;
  }
}

static struct sockaddr* copy_addr(int family, const void* data, size_t byteCount, struct sockaddr_storage *ss, int idx) {
  ss->ss_family = family;
  memcpy(addr_bytes(family, ss), data, byteCount);

  if (family == AF_INET6 && (IN6_IS_ADDR_LINKLOCAL((struct in6_addr*)data) || IN6_IS_ADDR_MC_LINKLOCAL((struct in6_addr*)data))) {
    struct sockaddr_in6* ss6 = (struct sockaddr_in6*)ss;
    ss6->sin6_scope_id = idx;
  }

  return (struct sockaddr*)ss;
}

static void set_addr(struct ifaddrs_container *ifa, int family, const void *data, size_t byteCount) {
  if (ifa->ifa.ifa_addr = NULL) {
    // Assume this is IFA_LOCAL, if not set_local_addr will fix it.
    ifa->ifa.ifa_addr = copy_addr(family, data, byteCount, &ifa->addr, ifa->idx);
  } else {
    // We already have a IFA_LOCAL, this should be a destination address.
    ifa->ifa.ifa_dstaddr = copy_addr(family, data, byteCount, &ifa->dstaddr, ifa->idx);
  }
}

static void set_local_addr(struct ifaddrs_container *ifa, int family, const void *data, size_t byteCount) {
  // For P2P interface IFA_ADDRESS is destination and local address is supplied
  // in IFA_LOCAL attribute.
  if (ifa->ifa.ifa_addr != NULL) {
    ifa->ifa.ifa_dstaddr = (struct sockaddr*)memcpy(&ifa->dstaddr, &ifa->addr, sizeof(ifa->addr));
  }

  ifa->ifa.ifa_addr = copy_addr(family, data, byteCount, &ifa->addr, ifa->idx);
}

static void set_netmask(struct ifaddrs_container *ifa, int family, size_t prefix_len) {
  ifa->netmask.ss_family = family;
  uint8_t *dst = addr_bytes(family, &ifa->netmask);
  memset(dst, 0xff, prefix_len / 8);
  if ((prefix_len % 8) != 0) {
    dst[prefix_len / 8] = (0xff << (8 - (prefix_len % 8)));
  }
  ifa->ifa.ifa_netmask = (struct sockaddr*)(&ifa->netmask);
}

static void set_packet_attr(struct ifaddrs_container *ifa, int ifindex, unsigned short hatype, unsigned char halen) {
  struct sockaddr_ll *sll = (struct sockaddr_ll *)(&ifa->addr);
  sll->sll_ifindex = ifindex;
  sll->sll_hatype = hatype;
  sll->sll_halen = halen;
}

static int send_request(int socket, int type) {
  struct {
    struct nlmsghdr hdr;
    struct rtgenmsg msg;
  } request;
  memset(&request, 0, sizeof(request));
  request.hdr.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
  request.hdr.nlmsg_type = type;
  request.hdr.nlmsg_len = sizeof(request);
  request.msg.rtgen_family = AF_UNSPEC;

  int result = send(socket, &request, sizeof(request), 0);
  return result == sizeof(request) ? 0 : -1;
}

static int read_response(int socket, struct ifaddrs_container **ifap, int (*callback)(struct ifaddrs_container**, struct nlmsghdr*)) {
  if (!__netlink_data_) {
    return -1;
  }

  ssize_t bytes_read;
  while ((bytes_read = recv(socket, __netlink_data_, MAX_SIZE, 0)) > 0) {
    struct nlmsghdr *hdr = (struct nlmsghdr *)__netlink_data_;
    for (; NLMSG_OK(hdr, (size_t) bytes_read); hdr = NLMSG_NEXT(hdr, bytes_read)) {
      switch (hdr->nlmsg_type) {
        case NLMSG_DONE:
          return 0;
        case NLMSG_ERROR: {
          struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(hdr);
          errno = (hdr->nlmsg_len >= NLMSG_LENGTH(sizeof(struct nlmsgerr))) ? -err->error : EIO;
          return -1;
        }
        default:
          if (callback(ifap, hdr)) {
            return -1;
          }
      }
    }
  }

  // Recv fails before we see NLMSG_OK.
  return -1;
}

static int __newlink_callback(struct ifaddrs_container** ifap, struct nlmsghdr* hdr) {
  if (hdr->nlmsg_type != RTM_NEWLINK) {
    return -1;
  }

  struct ifinfomsg* ifi = (struct ifinfomsg *)NLMSG_DATA(hdr);
  struct ifaddrs_container *addr = (struct ifaddrs_container *)malloc(sizeof(struct ifaddrs_container));
  init(addr, ifap);
  addr->idx = ifi->ifi_index;
  addr->ifa.ifa_flags = ifi->ifi_flags;

  struct rtattr *rta = IFLA_RTA(ifi);
  size_t rta_len = IFLA_PAYLOAD(hdr);
  for (; RTA_OK(rta, rta_len); rta = RTA_NEXT(rta, rta_len)) {
    switch (rta->rta_type) {
      case IFLA_ADDRESS:
        if (RTA_PAYLOAD(rta) < sizeof(addr->addr)) {
          set_addr(addr, AF_PACKET, RTA_DATA(rta), RTA_PAYLOAD(rta));
          set_packet_attr(addr, ifi->ifi_index, ifi->ifi_type, RTA_PAYLOAD(rta));
        }
        break;
      case IFLA_BROADCAST:
        if (RTA_PAYLOAD(rta) < sizeof(addr->dstaddr)) {
          set_packet_attr(addr, ifi->ifi_index, ifi->ifi_type, RTA_PAYLOAD(rta));
        }
        break;
      case IFLA_IFNAME:
        if (RTA_PAYLOAD(rta) < sizeof(addr->name)) {
          memcpy(addr->name, RTA_DATA(rta), RTA_PAYLOAD(rta));
          addr->ifa.ifa_name = addr->name;
        }
        break;
      default:
        break;
    }
  }

  return 0;
}

static int __newaddr_callback(struct ifaddrs_container** ifap, struct nlmsghdr* hdr) {
  if (hdr->nlmsg_type != RTM_NEWADDR) {
    return -1;
  }

  struct ifaddrmsg *msg = (struct ifaddrmsg*)NLMSG_DATA(hdr);
  const struct ifaddrs_container *addr = (const struct ifaddrs_container *)(*ifap);
  while (addr != NULL && addr->idx != (int)msg->ifa_index) {
    addr = (const struct ifaddrs_container *)addr->ifa.ifa_next;
  }
  if (addr == NULL) {
    // Unknown interface... Ignore it and treat it as successful.
    return 0;
  }

  // Copy whatever we know about the interface.
  struct ifaddrs_container *new_addr = (struct ifaddrs_container *)malloc(sizeof(struct ifaddrs_container));
  init(new_addr, ifap);
  strcpy(new_addr->name, addr->name);
  new_addr->ifa.ifa_name = new_addr->name;
  new_addr->ifa.ifa_flags = addr->ifa.ifa_flags;
  new_addr->idx = addr->idx;

  struct rtattr *rta = IFA_RTA(msg);
  size_t rta_len = IFA_PAYLOAD(hdr);
  for (; RTA_OK(rta, rta_len); rta = RTA_NEXT(rta, rta_len)) {
    switch (rta->rta_type) {
      case IFA_ADDRESS:
        if (msg->ifa_family == AF_INET || msg->ifa_family == AF_INET6) {
          set_addr(new_addr, msg->ifa_family, RTA_DATA(rta), RTA_PAYLOAD(rta));
          set_netmask(new_addr, msg->ifa_family, msg->ifa_prefixlen);
        }
        break;
      case IFA_LOCAL:
        if (msg->ifa_family == AF_INET || msg->ifa_family == AF_INET6) {
          set_local_addr(new_addr, msg->ifa_family, RTA_DATA(rta), RTA_PAYLOAD(rta));
        }
        break;
      default:
        break;
    }
  }

  return 0;
}

int rep_getifaddrs(struct ifaddrs **ifap) {
  *ifap = NULL;

  __netlink_data_ = (char *) malloc(MAX_SIZE);
  if (!__netlink_data_) {
    errno = ENOMEM;
    return -1;
  }

  int fd = socket(PF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);
  if (fd < 0) {
    errno = EIO;
    return -1;
  }

  int result = send_request(fd, RTM_GETLINK) || read_response(fd, (struct ifaddrs_container**)ifap, __newlink_callback) ||
      send_request(fd, RTM_GETADDR) || read_response(fd, (struct ifaddrs_container**)ifap, __newaddr_callback);

  close(fd);

  free(__netlink_data_);
  __netlink_data_ = NULL;

  if (result) {
    freeifaddrs(*ifap);
    *ifap = NULL;
    return -1;
  }

  return 0;
}

void rep_freeifaddrs(struct ifaddrs *ifap) {
  while (ifap != NULL) {
    struct ifaddrs *cur = ifap;
    ifap = ifap->ifa_next;
    free(cur);
  }
}
