/* Copyright (c) 2016, Piotr Durlej
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <priv/libc_globals.h>
#include <priv/libc.h>
#include <priv/sys.h>
#include <sys/types.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <os386.h>

char **	__libc_argv;
int	__libc_argc;
mode_t	__libc_umask;

char *	__libc_progname;
int	__libc_progfd;
int	__libc_arg_len;
char *	__libc_arg;

static const char __libc_unimplemented_msg[] = ": __libc_unimplemented: ";
static const char __libc_panic_msg[] = ": __libc_panic: ";

void __libc_unimplemented(char *func)
{
	write(STDERR_FILENO, __libc_progname, strlen(__libc_progname));
	write(STDERR_FILENO, __libc_unimplemented_msg, sizeof __libc_unimplemented_msg - 1);
	write(STDERR_FILENO, func, strlen(func));
	write(STDERR_FILENO, "\n", 1);
	
	_sysmesg(__libc_progname);
	_sysmesg(__libc_unimplemented_msg);
	_sysmesg(func);
	_sysmesg("\n");
	_exit(255);
}

void __libc_panic(char *desc)
{
	write(STDERR_FILENO, __libc_progname, strlen(__libc_progname));
	write(STDERR_FILENO, __libc_panic_msg, sizeof __libc_panic_msg - 1);
	write(STDERR_FILENO, desc, strlen(desc));
	write(STDERR_FILENO, "\n", 1);
	
	_sysmesg(__libc_progname);
	_sysmesg(__libc_panic_msg);
	_sysmesg(desc);
	_sysmesg("\n");
	_exit(255);
}

void __libc_initargenv(struct arg_buf *arg, int arg_size)
{
	int ac = arg->hd.argc;
	int ec = arg->hd.envc;
	int cnt;
	char **env;
	char *p;
	int i;
	
	if (arg_size < sizeof(struct arg_head) || arg_size > ARG_MAX)
		__libc_panic("__libc_initargenv: bad arg_size");
	
	if (ac >= arg_size || ec >= arg_size)
		__libc_panic("__libc_initargenv: bad argc or envc");
	
	for (cnt = 0, i = 0; i < arg_size - sizeof(struct arg_head); i++)
		if (!arg->buf[i])
			cnt++;
	if (cnt != ac + ec)
		__libc_panic("__libc_initargenv: wrong nr of strings");
	
	__libc_argv = malloc((ac + 1) * sizeof *__libc_argv);
	env	    = malloc((ec + 1) * sizeof *env);
	if (!__libc_argv || !env)
		__libc_panic("__libc_initargenv: unable to allocate argv or environ");
	
	for (p = arg->buf, i = 0; i < ac; i++)
	{
		__libc_argv[i] = p;
		p = strchr(p, 0) + 1;
	}
	__libc_argv[i] = NULL;
	
	for (i = 0; i < ec; i++)
	{
		env[i] = p;
		p = strchr(p, 0) + 1;
	}
	env[i] = NULL;
	
	_set_environ(env);
	__libc_argc  = ac;
	__libc_umask = arg->hd.umask;
}
