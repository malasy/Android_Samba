#!/bin/bash
CWD=$(pwd)

WAF_MAKE=1 python $CWD/buildtools/bin/waf build --targets=nmbd/nmbd,smbd/smbd,smbpasswd $*

rm -rf out
mkdir -p out/samba/lib
mkdir -p out/samba/bin
#bin
cp ./bin/default/source3/smbd/smbd out/samba/bin/
cp ./bin/default/source3/nmbd/nmbd out/samba/bin/
cp ./bin/default/source3/smbpasswd out/samba/bin/
#lib
cp -rL ./bin/shared/* out/samba/lib/
#conf
cp ./smb.conf out/samba/

# Create a default user
# UserName:root Password:root
# https://docs.microsoft.com/en-us/troubleshoot/windows-server/networking/guest-access-in-smb2-is-disabled-by-default
cp ./smbpasswd out/samba/
