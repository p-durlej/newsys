
CMD := init init.desktop init.profile mkfs shutdown mount umount \
       sync taskman login hwclock sysinstall diskfree \
       fault_test usermgr fsck wmount wumount \
       wsync init.devs list_devs find_devs edit_devs \
       check_devs fdisk mode list_devs \
       secure wdmesg showload pref.filesys \
       vtty-con wlogin daemon dmesg stress fxstat rdconfig tunefs \
       fdfmt fdctl diskperf pcidevs

include cmd.mk

fault_test: fault_test.c $(LIB_DEP)
	$(CC) $< -o $@ $(CPPFLAGS) $(CFLAGS) $(LDFLAGS)
