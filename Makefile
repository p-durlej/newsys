
include	config.mk

export TOPDIR TOOLDIR CC LD AR STRIP LIBGCC MODGEN MACH ARCH MAKEFLAGS LD_LIBRARY_PATH

ifdef LD_LIBRARY_PATH
	LD_LIBRARY_PATH := $(TOOLDIR)/lib:$(LD_LIBRARY_PATH)
else
	LD_LIBRARY_PATH := $(TOOLDIR)/lib
endif

all:
	cd cross	&& $(MAKE) all
	cd fonts	&& $(MAKE) all
	cd boot		&& $(MAKE) all
	cd sysload	&& $(MAKE) all
	cd kern		&& $(MAKE) all
	cd drv-arch	&& $(MAKE) all
	cd drv-pc	&& $(MAKE) all
	cd drv		&& $(MAKE) all
	cd lib		&& $(MAKE) all
	cd cmd		&& $(MAKE) all
	s/makedisk

base:
	cd cross	&& $(MAKE) all
	cd fonts	&& $(MAKE) all
	cd lib		&& $(MAKE) all
	cd cmd		&& $(MAKE) all
	s/makebase

clean-symlinks:
	rm -f include/kern/machine
	rm -f include/machine
	rm -f machine
	rm -f boot
	rm -f include/kern/arch
	rm -f include/arch
	rm -f kern/arch
	rm -f lib/load/arch
	rm -f lib/arch
	rm -f arch
	rm -f drv-arch

symlinks: clean-symlinks
	ln -s machine-$(MACH)				include/kern/machine
	ln -s machine-$(MACH)				include/machine
	ln -s machine-$(MACH)				machine
	ln -s boot-$(MACH)				boot
	ln -s arch-$(ARCH)				include/kern/arch
	ln -s arch-$(ARCH)				include/arch
	ln -s arch-$(ARCH)				kern/arch
	ln -s arch-$(ARCH)				lib/load/arch
	ln -s arch-$(ARCH)				lib/arch
	ln -s arch-$(ARCH)				arch
	ln -s drv-$(ARCH)				drv-arch

clean:
	cd cross	&& $(MAKE) clean
	cd fonts	&& $(MAKE) clean
	cd boot		&& $(MAKE) clean
	cd sysload	&& $(MAKE) clean
	cd kern		&& $(MAKE) clean
	cd drv-arch	&& $(MAKE) clean
	cd drv-pc	&& $(MAKE) clean
	cd drv		&& $(MAKE) clean
	cd lib		&& $(MAKE) clean
	cd cmd		&& $(MAKE) clean
	rm -Rf tree.tmp boot.sys boot.img aux.img aux2.img hdflat.img disks base.tar
