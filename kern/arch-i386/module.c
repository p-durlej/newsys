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

#include <kern/module.h>
#include <kern/printk.h>
#include <kern/errno.h>

int mod_proc_rel(struct module *m, struct modrel *r)
{
	uintptr_t v;
	char *p;
	
	if (r->addr + r->size > m->head->code_size)
	{
		printk("mod_proc_rel: invalid offset %li\n", r->addr);
		return EINVAL;
	}
	p  = m->code;
	p += r->addr;
	switch (r->size)
	{
	case 1:
		v = *(uint8_t *)p;
		break;
	case 2:
		v = *(uint16_t *)p; /* XXX not aligned */
		break;
	case 4:
		v = *(uint32_t *)p; /* XXX not aligned */
		break;
	default:
		printk("mod_proc_rel: invalid relocation size %i\n", r->size);
		return EINVAL;
	}
	
	switch (r->type)
	{
	case MR_T_ABS:
		v -= (uintptr_t)m->code;
		break;
	case MR_T_NORMAL:
		v += (uintptr_t)m->code;
		break;
	default:
		printk("mod_proc_rel: invalid relocation type %i\n", r->type);
		return EINVAL;
	}
	
	switch (r->size)
	{
	case 1:
		*(uint8_t *)p = v;
		break;
	case 2:
		*(uint16_t *)p = v; /* XXX not aligned */
		break;
	case 4:
		*(uint32_t *)p = v; /* XXX not aligned */
		break;
	default:
		printk("mod_proc_rel: invalid relocation size %i\n", r->size);
		return EINVAL;
	}
	return 0;
}
