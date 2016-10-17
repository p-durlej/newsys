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

#ifndef _KERN_UMEM_H
#define _KERN_UMEM_H

#ifndef NULL
#define NULL (void *)0
#endif

#include <kern/page.h>

extern int fault_jmp_set;

struct task;

int	u_alloc(vpage start, vpage end);
int	u_free(vpage start, vpage end);

void	dump_core(int status);
void	uclean(void);

#define UA_INIT		1
#define UA_READ		2
#define UA_WRITE	4

int	uga(void *pp, unsigned size, int flags);
int	uaa(void *pp, unsigned sz, unsigned nm, int flags);
int	usa(const char **p, unsigned max, int err);

int	urchk(const void *p, unsigned size);
int	uwchk(const void *p, unsigned size);

int	tucpy(void *d, const void *s, unsigned size);
int	fucpy(void *d, const void *s, unsigned size);
int	tustr(char *d, const char *s, unsigned max);
int	fustr(char *d, const char *s, unsigned max);
int	uset(void *p, unsigned v, unsigned size);

int	upush(unsigned long v);

int	fault(void);
void	fault_fini(void);
void	fault_abort(void);

int	ipcpy(struct task *dt, void *da, struct task *st, void *sa, unsigned size);

#endif
