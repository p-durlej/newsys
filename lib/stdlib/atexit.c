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

#include <priv/libc.h>
#include <stdlib.h>

struct atexit
{
	struct atexit *next;
	
	void	(*func)(void);
	void *	arg;
	void *	dso;
};

static struct atexit *atexit_list;

void __cxa_finalize(void *dso)
{
	struct atexit *ax;
	struct atexit **p;
	
restart:
	for (p = &atexit_list, ax = atexit_list; ax; ax = ax->next)
		if (!dso || ax->dso == dso)
		{
			*p = ax->next;
			ax->func();
			free(ax);
			goto restart;
		}
}

void __libc_call_atexit(void)
{
	/* struct atexit *ax;
	
	for (ax = atexit_list;ax;ax = ax->next)
		ax->func(); */
	__cxa_finalize(NULL);
}

int __cxa_atexit(void (*func)(void *), void *arg, void *dso)
{
	struct atexit *ax;
	
	ax = malloc(sizeof *ax);
	if (!ax)
		return -1;
	
	ax->next = atexit_list;
	ax->func = func;
	ax->arg	 = arg;
	ax->dso  = dso;
	atexit_list = ax;
	return 0;
}

int atexit(void (*func)(void))
{
	return __cxa_atexit((void *)func, NULL, NULL);
}
