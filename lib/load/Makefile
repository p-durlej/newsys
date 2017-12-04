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

include config.mk
include	mk/$(MACH).mk

# LOAD_O	= $(ARCH_O) load.o exports.o elo/elo.o elo/elosys.o ../libc.a
LOAD_O	= $(ARCH_O) load.o exports.o elo/elo.o elo/elosys.o
CRTB_O	= ../arch-$(ARCH)/crt0/crtbegin.o ../libc.o
CRTE_O	= ../arch-$(ARCH)/crt0/crtend.o
OBJS	= $(HEAD_O) $(CRTB_O) $(LOAD_O) $(CRTE_O)

all: load load.elf

exports.c: ../libc_syms mkexports
	./mkexports

load: head.o $(OBJS) $(LIBGCC)
	$(LD) -T arch-$(ARCH)/load.lds --oformat binary head.o $(OBJS) $(LIBGCC) -o load
	../../cross/ckexe -w load

load.elf: head.o $(OBJS)
	$(LD) -T arch-$(ARCH)/load.lds head.o $(OBJS) $(LIBGCC) -o load.elf

clean:
	rm -f load load.elf exports.c head.o
	rm -f $(LOAD_O) $(HEAD_O)
