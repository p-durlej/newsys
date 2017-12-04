#
# Copyright (c) 2017, Piotr Durlej
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

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
	rm -f kern/arch
	rm -f lib/load/arch
	rm -f lib/arch
	rm -f arch
	rm -f drv-arch

symlinks: clean-symlinks
	ln -s machine-$(MACH)				machine
	ln -s boot-$(MACH)				boot
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
