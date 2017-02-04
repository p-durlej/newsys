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

#ifndef _KERN_MODULE_H
#define _KERN_MODULE_H

#include <sys/types.h>
#include <module.h>
#include <limits.h>

#define MODULE_MAX	128

extern struct module
{
	int		in_use;
	int		malloced;
	
	struct modhead *head;
	struct modsym *	sym;
	struct modrel *	rel;
	void *		code;
	char		name[NAME_MAX + 1];
} module[MODULE_MAX];

typedef int mod_onload_t(unsigned md, const char *pathname, const void *data, unsigned data_size);
typedef int mod_onunload_t(unsigned md);

mod_onload_t mod_onload;
mod_onunload_t mon_onunload;

int mod_proc_rel(struct module *m, struct modrel *r);

int mod_init(struct module *m, const char *pathname, struct modhead *head, void *data, unsigned data_size);
int mod_unload(unsigned md);
int mod_getslot(struct module **buf);
int mod_getid(struct module *m);

int mod_pgetsym(struct module *m, const char *name, void **buf);
int mod_getsym(unsigned md, char *name, void **buf);

void mod_boot(void);

#endif
