# Build
1. Modify configure.sh to change $NDK to point to your NDK folder.
2. Uncomment corresponding flags in configure.sh to compile for different architecture. Uncomment flags for ARMv7 in addition to 32-bit ARM to compile it for ARMv7.
3. Run ```configure.sh``` to configure Samba project.
4. Run ```compile.sh``` to compile

# Install
copy out/samba dir to android 
```
mkdir -p /data/samba/private
mkdir -p /data/samba/var
mkdir -p /data/samba/etc

cp /data/samba/smb.conf /data/samba/etc/
cp /data/samba/smbpasswd /data/samba/etc/

export LD_LIBRARY_PATH=/data/samba/lib:/data/samba/lib/private
export TMPDIR=/data/local/tmp


./data/samba/bin/smbd -D
./data/samba/bin/nmbd -D
```

# Use
https://blog.csdn.net/weixin_40806910/article/details/81917077


# Reference:
   * https://github.com/google/samba-documents-provider
   * https://github.com/elliott10/samba-4.5.1
   * https://github.com/berserker/android_samba