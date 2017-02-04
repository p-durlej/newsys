/* Copyright (c) 2017, Piotr Durlej
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

#ifndef _PRIV_LIBC_H
#define _PRIV_LIBC_H

#include <sys/types.h>
#include <limits.h>

struct arg_head
{
	mode_t umask;
	int    argc;
	int    envc;
};

struct arg_buf
{
	struct arg_head hd;
	char buf[ARG_MAX - sizeof(struct arg_head)];
};

/* libcall handling */

void	libc_entry(void);

/* library initialization and cleanup */

void	__libc_initargenv(struct arg_buf *arg, int arg_size);
void	__libc_malloc_init(void);
int	__libc_stdio_init(void);
void	__libc_rand_init(void);

void	__libc_stdio_cleanup(void);
void	__libc_call_atexit(void);

/* fatal errors */

void	__libc_unimplemented(char *func) __attribute__((noreturn));
void	__libc_panic(char *desc) __attribute__((noreturn));

/* environment */

void	_set_environ_ptr(char ***env_p);
void	_set_environ(char **env);
char **	_get_environ(void);

/* exec and newtask */

size_t __libc_mkarg(struct arg_buf **buf, char * const* argv, char * const* envp);

/* stdio */

#if FLOAT
char *convf_float(float v, int prec);
char *convf_double(double v, int prec);
char *convf_long(long double v, int prec);
#endif

#endif
