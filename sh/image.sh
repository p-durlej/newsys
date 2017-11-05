#!/bin/sh
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

set -e

fonts="mono system mono-large system-large mono-narrow mono-large-narrow"
examples="aclock.c avail.c box.c calc.c edit.c ps.c textbench.c uptime.c"
sexamples="taskman.c"

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

make_sdk_lists()
{
	(cd tree.tmp && find usr/include usr/lib -type d) | while read i; do
		echo "755 /$i"
	done > tree.tmp/etc/sdk.mkdir
	
	(cd tree.tmp && find usr/include usr/lib -type f) | while read i; do
		echo "644 /$i"
	done > tree.tmp/etc/sdk.copy
	
	for i in $examples $sexamples; do
		echo "644 /usr/examples/$i"
	done >> tree.tmp/etc/sdk.copy
	
	cat >> tree.tmp/etc/sdk.copy << EOF
755 /bin/code
755 /usr/bin/examples
755 /usr/bin/fnc
755 /usr/bin/mgen
755 /usr/bin/modinfo
755 /usr/bin/tcc
644 /lib/icons/code.pnm
644 /lib/icons64/code.pnm
644 /etc/filetype/ext-c
644 /etc/filetype/ext-h
644 /etc/skel/.desktop/Apps/Code Editor
644 /root/.desktop/Apps/Code Editor
644 /usr/examples/.dirinfo
EOF
}

copy_sdk()
{
	cp -LRpf	include/*		tree.tmp/usr/include/
	cp -pf		cmd/tcc.include/*	tree.tmp/usr/include/
	cp -pf		lib/arch/crt0/libc.a	tree.tmp/usr/lib/
	cp -pf		lib/arch/crt0/crt0.o	tree.tmp/usr/lib/
	cp -pf		cmd/tcc.lib/libtcc1.a	tree.tmp/usr/lib/
	cp -pf		cmd/tcc/tcc		tree.tmp/usr/bin/
	cp -pf		cmd/sdk.bin/*		tree.tmp/usr/bin/
	
	for i in $examples; do
		cp -f cmd/bin/$i tree.tmp/usr/examples/
	done
	
	for i in $sexamples; do
		cp -f cmd/sbin/$i tree.tmp/usr/examples/
	done
	
	rm -rf		tree.tmp/usr/include/kern/machine-*
	rm -rf		tree.tmp/usr/include/kern/arch-*
	rm -rf		tree.tmp/usr/include/machine-*
	rm -rf		tree.tmp/usr/include/arch-*
	
	do_strip	tree.tmp/usr/bin/tcc
	
	make_sdk_lists
}

make_disks()
{
	[ -d disks ] || mkdir disks
	
	sh machine/disks.sh
}

make_dirs()
{
	while read i; do
		mkdir -p "tree.tmp/$i"
	done < tree.dirs
}
