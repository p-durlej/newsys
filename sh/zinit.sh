#!/bin/sh
#
# Copyright (c) 2017, Piotr Durlej
# All rights reserved.
#

make_zinit()
{
	rm -rf zinit.tmp
	mkdir -p zinit.tmp/etc
	mkdir -p zinit.tmp/sbin
	mkdir -p zinit.tmp/lib/drv
	mkdir -p zinit.tmp/lib/sys
	
	cp -pf kern/os386	zinit.tmp/lib/sys/kern.bin
	cp -pf lib/load/load	zinit.tmp/lib/sys/user.bin
	cp -pf cmd/zlib/zinit	zinit.tmp/sbin/init
	cp -pf drv/rd.drv	zinit.tmp/lib/drv
	cp -pf tree/etc/bootmsg	zinit.tmp/etc/bootmsg
	
	${STRIP} zinit.tmp/sbin/*
	
	cross/mkbfs   zinit.tmp/lib/sys/root.img 0 128 tree.tmp
	cross/zpipe < zinit.tmp/lib/sys/root.img > zinit.tmp/lib/sys/root
	rm -f zinit.tmp/lib/sys/root.img
	
	dd status=none if=boot-pc/fdboot.bin	of=disks/zinit.img
	dd status=none if=sysload/sysload	of=disks/zinit.img	seek=2
	dd status=none if=/dev/null		of=disks/zinit.img	seek=2880
	cross/mkbfs disks/zinit.img 0 128 zinit.tmp
}
