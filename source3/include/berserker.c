/*
 * From https://github.com/berserker/android_samba
 * Refer https://github.com/berserker/android_samba
 */
#include "includes.h"
#include <linux/swab.h>
#include <android/log.h>

struct passwd *g_root_passwd = NULL;

static void log_message(int level, const char *message)
{
    if(message != NULL)
      __android_log_write(level, "SambaDroidDaemon", message);	
}

struct passwd * getpasswd_wrapper_impl(const char* name)
{
    /* 
        As there is no /etc/passwd, need to fake one 
        - only allow names that are <= 8 characters in length
        - contain a-z A-Z 0-9
    */

    size_t c;
    size_t len;

    if(!name || !name[0])
        return NULL;

    len = strlen(name);
    if(len > 8) 
        return NULL;

    for(c = 0; c < len; c++) 
    {
        if(!isalnum(name[c])) 
            return NULL;
    }

    /*
    struct passwd *ptmp;
    ptmp = (struct passwd *)malloc(sizeof(struct passwd));
    ptmp->pw_name = (char *)malloc(len + 1);
    fstrcpy(ptmp->pw_name,name);
    ptmp->pw_passwd = (char *)malloc(strlen("*") + 1);
    fstrcpy(ptmp->pw_passwd,"*");
    ptmp->pw_uid = 0;
    ptmp->pw_gid = 0;
    ptmp->pw_gecos = (char *) malloc(len + 1);
    fstrcpy(ptmp->pw_gecos,name);
    ptmp->pw_dir = (char *) malloc(strlen("/data/local")+1);
    fstrcpy(ptmp->pw_dir,"/data/local");
    ptmp->pw_shell = (char *) malloc(strlen("/system/xbin/sh")+1);
    fstrcpy(ptmp->pw_shell,"/system/xbin/sh");
    return ptmp;    
    */

    //struct passwd *ptmp = (struct passwd *) malloc(sizeof(struct passwd));
    struct passwd *ptmp = SMB_MALLOC_ARRAY(struct passwd, sizeof(struct passwd));
    //ptmp->pw_name = (char *) malloc(len + 1);
    ptmp->pw_name = SMB_MALLOC_ARRAY(char, len + 1);
    fstrcpy(ptmp->pw_name, name);
    //ptmp->pw_passwd = (char *) malloc(strlen("*") + 1);
    ptmp->pw_passwd = SMB_MALLOC_ARRAY(char, strlen("x") + 1);
    fstrcpy(ptmp->pw_passwd, "x");
    ptmp->pw_uid = 0;
    ptmp->pw_gid = 0;
    //ptmp->pw_gecos = (char *) malloc(len + 1);
    ptmp->pw_gecos = SMB_MALLOC_ARRAY(char, len + 1);
    fstrcpy(ptmp->pw_gecos, name);
    ptmp->pw_dir = "/data/samba";
    ptmp->pw_shell = "/system/xbin/sh";

    return ptmp;
}

struct passwd * getpasswd_wrapper()
{
//  return getpasswd_wrapper_impl("root");

    if(g_root_passwd == NULL)
    {
        //struct passwd *ptmp = (struct passwd *) malloc(sizeof(struct passwd));
        struct passwd *ptmp = SMB_MALLOC_ARRAY(struct passwd, sizeof(struct passwd));
        ptmp->pw_name = "root";
        ptmp->pw_passwd = "x";
        ptmp->pw_uid = 0;
        ptmp->pw_gid = 0;
        ptmp->pw_gecos = "root";
        ptmp->pw_dir = "/data/samba";
        ptmp->pw_shell = "/system/xbin/sh";
        g_root_passwd = ptmp;
    }

    return g_root_passwd;
}

/* getpwent restituisce il prossimo della lista */
struct passwd * getpwnam(const char* name)
{
    return getpasswd_wrapper();
}

struct passwd * getpwuid(uid_t uid)
{
    return getpasswd_wrapper();
}

/* setpwent resetta la lista per iterare su passwd */
int setpwent()
{
    return 0;
}

/* getpwent restituisce la prossima struttura passwd */
struct passwd * getpwent()
{
    return getpasswd_wrapper();
}

/* termina il ciclo di iterazione su passwd */
void endpwent()
{

}

void swab(const void *from, void *to, ssize_t n)
{
    ssize_t i;

    if(n < 0)
        return;

    for(i = 0; i < (n/2)*2; i += 2)
        *((uint16_t*)to+i) = ___constant_swab16(*((uint16_t*)from+i));
}
/*
struct DIR {
    int fd_;
};
long telldir(DIR *dir)
{
    log_message(ANDROID_LOG_INFO, "telldir called");
    return (long) lseek(dir->fd_, 0, SEEK_CUR);
}

void seekdir(DIR *dir, long ofs)
{
    log_message(ANDROID_LOG_INFO, "seekdir called");
    (void) lseek(dir->fd_, ofs, SEEK_SET);
}
*/