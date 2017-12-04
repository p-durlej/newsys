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

ARCH		?= i386
MACH		:= $(ARCH)-pc

TOOLDIR		:= $(TOPDIR)/../tools
GCCVER		:= 4.3.2# 4.4.3
OBJCOPY		:= $(TOOLDIR)/bin/$(ARCH)-os386-elf-objcopy
CC		:= $(TOOLDIR)/bin/$(ARCH)-os386-elf-gcc
CPP		:= $(TOOLDIR)/bin/$(ARCH)-os386-elf-cpp
CXX		:= /bin/false
LD		:= $(TOOLDIR)/bin/$(ARCH)-os386-elf-ld
AR		:= $(TOOLDIR)/bin/$(ARCH)-os386-elf-ar
STRIP		:= $(TOOLDIR)/bin/$(ARCH)-os386-elf-strip
MODGEN		:= $(TOPDIR)/cross/mgen/mgen
MAKE		:= gmake
CPPFLAGS	:= -I$(TOPDIR)/include -I$(TOPDIR)/include-$(ARCH) -I$(TOPDIR)/include-$(MACH) -fno-builtin
ASFLAGS		:=
CFLAGS		:=
CXXFLAGS	:=
LDFLAGS		:= -B$(TOPDIR)/lib/arch-$(ARCH)/crt0 -L$(TOPDIR)/lib/dummy

include arch-$(ARCH).mk
include const.mk
