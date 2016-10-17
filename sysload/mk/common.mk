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

include config.mk

SYSLOAD_O := $(MACHINE_O) $(ARCH_O) main.o disk.o console.o itoa.o atoi.o \
	     fs.o fs-boot.o fs-native.o

SYSLOAD_ELF := sysload.elf
SYSLOAD_BIN := sysload.bin
SYSLOAD_CD  := sysload.cdrom
SYSLOAD_PXE := sysload.pxe

all: sysload $(SYSLOAD_BIN) $(SYSLOAD_ELF) $(SYSLOAD_CD) $(SYSLOAD_PXE)

sysload: $(SYSLOAD)
	cp -p $(SYSLOAD) sysload

$(SYSLOAD_BIN): $(SYSLOAD_ELF) $(LDS)
	$(OBJCOPY) -O binary $(SYSLOAD_ELF) $(SYSLOAD_BIN)

$(SYSLOAD_ELF): $(SYSLOAD_O) $(LDS)
	$(LD) -T $(LDS) $(SYSLOAD_O) $(LIBGCC) -o $(SYSLOAD_ELF)

$(SYSLOAD_CD): $(SYSLOAD_BIN)
	cp -p $(SYSLOAD_BIN) $(SYSLOAD_CD)
	../cross/psysload $(SYSLOAD_CD) cdrom

$(SYSLOAD_PXE): $(SYSLOAD_BIN)
	cp -p $(SYSLOAD_BIN) $(SYSLOAD_PXE)
	../cross/psysload $(SYSLOAD_PXE) pxe

clean:
	rm -f $(SYSLOAD_BIN) $(SYSLOAD_SYS) $(SYSLOAD_ELF) $(SYSLOAD_REL) \
	      $(SYSLOAD_CD) $(SYSLOAD_PXE)				  \
	      $(SYSLOAD_O) sysload
