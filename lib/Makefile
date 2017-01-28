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

all: arch/crt0/crt0.o arch/crt0/crtbegin.o arch/crt0/crtend.o \
     arch/crt0/libc.a libc.a libc.o
	(cd load; $(MAKE) all)

SYSCALL_O = arch/syscall/syscall.o

UNISTD_O = unistd/chmod.o unistd/chown.o				\
           unistd/creat.o unistd/dup.o unistd/exec.o			\
           unistd/sleep.o unistd/ttyname.o				\
           unistd/wait.o unistd/stat.o unistd/open.o unistd/dup2.o	\
           unistd/waitpid.o unistd/_xwait.o unistd/utime.o		\
           unistd/mknod.o unistd/mkdir.o unistd/umask.o unistd/read.o	\
           unistd/write.o unistd/getopt.o unistd/poll.o unistd/sync.o	\
           unistd/chdir.o

SIGNAL_O = unistd/raise.o

STDIO_O = stdio/stdfd.o stdio/putget.o stdio/vprintf.o stdio/vfprintf.o	    \
          stdio/vsprintf.o stdio/vsnprintf.o stdio/printf.o stdio/fprintf.o \
          stdio/sprintf.o stdio/snprintf.o stdio/remove.o stdio/fopen.o	    \
          stdio/freopen.o stdio/fseek.o stdio/setbuf.o stdio/readwrite.o    \
          stdio/asprintf.o stdio/clearerr.o stdio/feof.o		    \
          stdio/ferror.o stdio/fileno.o

STDLIB_O = stdlib/exit.o stdlib/strtoul.o stdlib/strtol.o stdlib/atol.o     \
           stdlib/atoi.o stdlib/abort.o stdlib/system.o stdlib/qsort.o	    \
           stdlib/mktemp.o						    \
           stdlib/atexit.o stdlib/malloc.o stdlib/abs.o			    \
           stdlib/labs.o stdlib/llabs.o stdlib/imaxabs.o stdlib/div.o	    \
           stdlib/ldiv.o stdlib/lldiv.o stdlib/imaxdiv.o stdlib/bsearch.o   \
           stdlib/strtoll.o stdlib/strtoull.o stdlib/rand.o stdlib/_Exit.o  \
           stdlib/atoll.o

ENVIRON_O = environ/environ.o

CTYPE_O = ctype/isalnum.o ctype/isalpha.o ctype/iscntrl.o ctype/isdigit.o  \
          ctype/islower.o ctype/isprint.o ctype/ispunct.o ctype/isspace.o  \
          ctype/isupper.o ctype/isxdigit.o ctype/toupper.o ctype/tolower.o \
          ctype/isgraph.o

WINGUI_O = wingui/wait.o wingui/form.o wingui/button.o wingui/label.o	      \
           wingui/msgbox.o wingui/menu.o wingui/list.o wingui/vsbar.o	      \
           wingui/input.o wingui/edbox.o wingui/dlg_comm.o		      \
           wingui/dlg_security.o wingui/chkbox.o wingui/pict.o wingui/icbox.o \
           wingui/bitmap.o wingui/stdcolor.o wingui/popup.o wingui/frame.o    \
           wingui/hsbar.o wingui/form_load.o wingui/bell.o		      \
           wingui/buf.o wingui/break.o wingui/util.o wingui/metrics.o	      \
           wingui/colorsel.o wingui/pointer.o wingui/bargraph.o		      \
           wingui/theme.o wingui/theme-flat.o wingui/draw.o

PASSWD_O = passwd/passwd.o passwd/group.o

CRYPTO_O = sha/sha.o

LOCALE_O = locale/setlocale.o locale/localeconv.o

DEV_O	 = dev/speaker.o

PREFS_O = prefs/dmode.o prefs/wbeep.o

LIB_O = crt0c.o perror.o dirent.o time.o confdb.o cprintf.o		\
        uname.o event.o assert.o list.o module.o newtask.o		\
        mkcanon.o timer.o err.o strsignal.o regexp.o findexec.o		\
        ringbuf.o termios.o fmthumansz.o vt100/vt100.o			\
        tar.o								\
        $(STRING_O) $(UNISTD_O) $(SIGNAL_O) $(STDIO_O) $(STDLIB_O)	\
        $(ENVIRON_O) $(PASSWD_O) $(CTYPE_O) $(SYSCALL_O) $(WINGUI_O)	\
        $(ARCH_O) $(CRYPTO_O) $(MATH_O) $(LOCALE_O) $(DEV_O) $(PREFS_O)

CRT0D_O = crt0d/getopt.o

include arch/arch.mk

libc.a: $(LIB_O)
	rm -f libc.a
	$(AR) -q libc.a $(LIB_O)

libc.o: $(LIB_O)
	$(LD) -r $(LIB_O) $(LIBGCC) -o libc.o

arch/crt0/autogen: libc_syms arch/crt0/mklc
	rm -rf arch/crt0/autogen.tmp
	rm -rf arch/crt0/autogen
	mkdir  arch/crt0/autogen.tmp
	(cd arch/crt0 && sh mklc)
	mv arch/crt0/autogen.tmp arch/crt0/autogen

arch/crt0/libc.a: arch/crt0/autogen $(CRT0D_O)
	$(AR) cr arch/crt0/libc.a arch/crt0/autogen/lc_*.o $(CRT0D_O)

arch/syscall/syscall.S: arch/syscall/makesys ../kern/syslist/syslist
	(cd arch/syscall && sh makesys)

clean:
	(cd load && $(MAKE) clean)
	rm -rf arch/crt0/autogen
	rm -f arch/crt0/crt0.o arch/crt0/libc.a libc.a libc.o
	rm -f arch/syscall/syscall.S
	rm -f $(LIB_O) $(CRT0D_O)
