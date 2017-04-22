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

DRV := vga.drv vgagray.drv vga256.drv vbe.drv i386-pc.drv biosdisk.drv

I386_PC_O := i386-pc/main.o	\
	     i386-pc/bios.o	\
	     i386-pc/console.o	\
	     i386-pc/asmintr.o	\
	     i386-pc/intr.o	\
	     i386-pc/clock.o	\
	     i386-pc/dma.o	\
	     i386-pc/vesacon.o	\
	     i386-pc/monofnt.o

CLEAN := $(I386_PC_O)

i386-pc.drv.o: $(I386_PC_O)
	$(LD) -r $(CFLAGS) -o $@ $(I386_PC_O)

i386-pc.drv: i386-pc.drv.o i386-pc-syms
	$(MODGEN) $@ $< i386-pc-syms

vgagray.drv.o: vga.drv.c
	$(CC) $(CFLAGS) -DGRAY -c -o $@ $<

include drv.mk
