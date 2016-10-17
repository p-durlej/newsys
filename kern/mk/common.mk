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

CFLAGS += -g -Wall -Werror -D_KERN_

COMMON_LIB_O := lib/strcpy.o	lib/strcmp.o		\
                lib/strcat.o	lib/strchr.o		\
                lib/strrchr.o	lib/strlen.o		\
                lib/strncpy.o	lib/strncmp.o		\
                lib/memmove.o	lib/memcmp.o		\
                lib/memcpy.o	lib/memset.o		\
                lib/list.o	lib/ringbuf.o		\
                lib/ringbuf_kern.o

LIB_O := lib/malloc.o lib/phys_map.o \
         lib/printk.o lib/panic.o

NATFS_O := fs/nat/main.o fs/nat/dir.o fs/nat/mount.o fs/nat/rw.o \
           fs/nat/bmap.o fs/nat/fso.o

BFS_O := fs/bfs/bfs.o

DEVFS_O := fs/devfs/devfs.o

PTYFS_O := fs/pty/ptyfs.o

FS_O := fs/syscall.o fs/main.o fs/mount.o fs/misc.o fs/fdesc.o fs/pipe.o \
        $(BFS_O) $(DEVFS_O) $(NATFS_O) $(PTYFS_O)

WINGUI_O := wingui/syscall.o wingui/main.o wingui/event.o wingui/desktop.o \
            wingui/window.o wingui/paint.o wingui/null.o wingui/font.o

DRV_O := drv/rd.o

CONSOLE_O := console/console.o console/bootcon.o

KERN_O := $(MACHINE_O) $(ARCH_O) $(LIB_O) $(FS_O) $(WINGUI_O) $(COMMON_LIB_O) \
          $(DRV_O) $(CONSOLE_O) \
          main.o task.o exec.o sched.o block.o panic.o clock.o \
          syscall.o module.o signal.o cio.o event.o fork.o \
          power.o syslist/systab.o mqueue.o mutex.o shutdown.o \
          task_dproc.o

KERN_ELF := os386.elf
KERN_BIN := os386

all: $(KERN_BIN) $(KERN_ELF)

$(KERN_BIN): $(KERN_ELF) $(LDS)
	$(OBJCOPY) -O binary $(KERN_ELF) $(KERN_BIN)

$(KERN_ELF): $(KERN_O) $(LDS)
	$(LD) -T $(LDS) $(KERN_O) $(LIBGCC) -o $(KERN_ELF)

syslist/systab.c: syslist/syslist syslist/tabgen.sh
	(cd syslist && sh tabgen.sh)

clean:
	rm -f $(KERN_BIN) $(KERN_SYS) $(KERN_ELF) $(KERN_REL) $(KERN_O) os386
