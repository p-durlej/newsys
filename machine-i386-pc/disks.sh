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

make_src() {
	rm -rf disks/src
	mkdir  disks/src
	
	git clone -q . disks/src
	rm -rf disks/src/.git
	tar cf disks/src.tar -C disks src
	
	rm -rf disks/src
}

dd status=none if=boot-pc/hdflat.bin	of=disks/hdflat.img
dd status=none if=sysload/sysload	of=disks/hdflat.img	seek=2
dd status=none if=/dev/null		of=disks/hdflat.img	seek=16384
cross/mkbfs disks/hdflat.img 1 128 tree.tmp

dd status=none if=boot-pc/cdboot.bin	of=disks/cdrom.iso
dd status=none if=sysload/sysload.cdrom	of=disks/cdrom.iso	seek=4		conv=notrunc
dd status=none if=/dev/null		of=disks/cdrom.iso	seek=16384
cross/mkbfs disks/cdrom.iso 0 128 tree.tmp

cross/mkbfs disks/pxe.img 0 128 tree.tmp

make_src

mkisofs -quiet -V "NamelessOS" -o disks/altcd.iso -hard-disk-boot -eltorito-boot hdflat.img -hide boot.catalog -hide hdflat.img disks/hdflat.img doc/readme.txt disks/src.tar
