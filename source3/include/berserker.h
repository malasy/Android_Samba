#ifndef __BERSERKER_H__
#define __BERSERKER_H__


#define SAMBADROID_ROOT     "/data/samba"
#define SAMBADROID_TMP      SAMBADROID_ROOT "/tmp"
#define SBINDIR             SAMBADROID_ROOT "/sbin"
#define BINDIR              SAMBADROID_ROOT "/bin"
#define SWATDIR             SAMBADROID_ROOT "/swat"
#define CONFIGDIR           SAMBADROID_ROOT "/lib"
#define CONFIGFILE          CONFIGDIR "/smb.conf"
#define VARDIR              SAMBADROID_ROOT "/var"
#define LOGFILEBASE         VARDIR
#define LMHOSTSFILE         CONFIGDIR "/lmhosts"
#define MODULESDIR          SAMBADROID_ROOT "/lib"
#define CODEPAGEDIR         MODULESDIR
#define LIBDIR              SAMBADROID_ROOT "/lib"
#define LOCKDIR             VARDIR "/locks"
#define STATEDIR            LOCKDIR
#define CACHEDIR            LOCKDIR
#define PIDDIR              VARDIR "/locks"
#define NMBDSOCKETDIR       VARDIR "/nmbd"
#define NCALRPCDIR          VARDIR "/ncalrpc"
#define PRIVATEDIR          SAMBADROID_ROOT "/private"
#define PRIVATE_DIR         PRIVATEDIR
#define SMB_PASSWD_FILE     PRIVATEDIR "/smbpasswd"
#define LOCALEDIR           SAMBADROID_ROOT "/share/locale"

#include <unistd.h>
#include <sys/types.h>

#define _PWD_H_ /* disabilita l'inclusione del file pwd.h sotto android */

struct passwd
{
    char* pw_name;
    char* pw_passwd;
    uid_t pw_uid;
    gid_t pw_gid;
    char* pw_gecos;
    char* pw_dir;
    char* pw_shell;
};

struct passwd* getpwnam(const char*);
struct passwd* getpwuid(uid_t);

void endpwent(void);
struct passwd* getpwent(void);
int setpwent(void);

void swab(const void *from, void *to, ssize_t n);

/*
long telldir(DIR *dir);
void seekdir(DIR *dir, long ofs);
*/

/* fix per defines mancanti di utmp.ut_type */
#ifndef DEAD_PROCESS
    #define DEAD_PROCESS  8
#endif

#endif