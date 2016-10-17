#!/bin/sh
#
# Copyright (c) 2016, Piotr Durlej
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

set -e

fonts="mono system mono-large system-large"

regen_tree()
{
	rm -Rf  tree.tmp
	cp -Rpf tree tree.tmp
	
	[ -d arch/tree	  ] && cp -Rpf arch/tree/*	tree.tmp
	[ -d machine/tree ] && cp -Rpf machine/tree/*	tree.tmp
}

do_strip()
{
	${STRIP} "$1" || :
}

copy_sysinstall()
{
	cat machine/sysinstall.mkdir >> tree.tmp/etc/sysinstall.mkdir
	cat machine/sysinstall.copy  >> tree.tmp/etc/sysinstall.copy
	cat arch/sysinstall.mkdir    >> tree.tmp/etc/sysinstall.mkdir
	cat arch/sysinstall.copy     >> tree.tmp/etc/sysinstall.copy
}

copy_files()
{
	for i in cmd/sh/* cmd/bin/* cmd/acex-bin/* cmd/tests/*		\
		 cmd/filemgr/filemgr cmd/filemgr/pref.filemgr		\
		 cmd/filemgr/filemgr.slave
	do
		if [ -x $i ]
		then
			cp -pf $i tree.tmp/bin/$(basename $i)
			do_strip tree.tmp/bin/$(basename $i)
		fi
	done
	
	cp -pf cmd/filemgr/filemgr.mkfsi tree.tmp/sbin/
	do_strip tree.tmp/sbin/filemgr.mkfsi
	
	for i in cmd/sbin/* cmd/acex-sbin/*
	do
		if [ -x $i ]
		then
			cp -pf $i tree.tmp/sbin/$(basename $i)
			do_strip tree.tmp/sbin/$(basename $i)
		fi
	done
	
	for i in drv/*.drv drv/*.sys drv-arch/*.drv drv-pc/*.drv
	do
		if [ -f $i ]
		then
			cp -pf $i tree.tmp/lib/drv/$(basename $i)
		fi
	done
	
	for i in $fonts
	do
		cp -pf fonts/$i tree.tmp/lib/fonts/$i
	done
	
	cp -pf lib/load/load	tree.tmp/lib/sys/user.bin
}

copy_sys()
{
	cp -pf sysload/sysload	tree.tmp/lib/sys/sysload
	cp -pf kern/os386	tree.tmp/lib/sys/kern.bin
	cp -pf boot/*.bin	tree.tmp/lib/sys/
	
	chmod a-x tree.tmp/lib/sys/*.bin > /dev/null 2>&1 # XXX
}

make_disks()
{
	[ -d disks ] || mkdir disks
	
	for disk in machine/disks/*.mkdir
	do
		disk=$(basename $disk .mkdir)
		rm -rf disks/$disk
		mkdir  disks/$disk
		
		while read i; do
			mkdir "disks/$disk/$i"
		done < machine/disks/$disk.mkdir
	done
	
	for disk in machine/disks/*.copy
	do
		disk=$(basename $disk .copy)
		
		while read i; do
			cp -pf "tree.tmp/$i" "disks/$disk/$i"
		done < "machine/disks/$disk.copy"
		
		rm -f	    disks/$disk.img
		cross/mkbfs disks/$disk.img 0 128 disks/$disk
	done
	
	sh machine/disks.sh
}

make_dirs()
{
	while read i; do
		mkdir -p "tree.tmp/$i"
	done < tree.dirs
}
