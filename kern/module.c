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

#include <sysload/kparam.h>

#include <machine/machdef.h>
#include <arch/archdef.h>

#include <kern/machine/machine.h>
#include <kern/machine/start.h>
#include <kern/shutdown.h>
#include <kern/exports.h>
#include <kern/console.h>
#include <kern/syscall.h>
#include <kern/module.h>
#include <kern/mqueue.h>
#include <kern/printk.h>
#include <kern/config.h>
#include <kern/panic.h>
#include <kern/power.h>
#include <kern/start.h>
#include <kern/errno.h>
#include <kern/sched.h>
#include <kern/clock.h>
#include <kern/intr.h>
#include <kern/task_queue.h>
#include <kern/task.h>
#include <kern/umem.h>
#include <kern/lib.h>
#include <kern/cpu.h>
#include <kern/fs.h>
#include <kern/event.h>

#include <sys/types.h>

extern void _start;

struct module module[MODULE_MAX];

static struct ksym
{
	char *name;
	void *value;
} ksym[] =
{
	{ "printk",			printk			},
	{ "perror",			perror			},
	{ "panic",			panic			},
	{ "panic_install",		panic_install		},
	
	{ "con_install",		con_install		},
	{ "con_reset",			con_reset		},
	{ "con_clear",			con_clear		},
	
	{ "signal_pending",		signal_pending		},
	{ "geteuid",			geteuid			},
	{ "getegid",			getegid			},
	{ "getruid",			getruid			},
	{ "getrgid",			getrgid			},
	
	{ "blk_install",		blk_install		},
	{ "blk_uinstall",		blk_uinstall		},
	{ "blk_get",			blk_get			},
	{ "blk_put",			blk_put			},
	
	{ "cio_install",		cio_install		},
	{ "cio_uinstall",		cio_uinstall		},
	{ "cio_setstate",		cio_setstate		},
	{ "cio_clrstate",		cio_clrstate		},
	{ "cio_state",			cio_state		},
	
	{ "fs_install",			fs_install		},
	{ "fs_uinstall",		fs_uinstall		},
	
	{ "clock_ticks",		clock_ticks		},
	{ "clock_time",			clock_time		},
	{ "clock_delay",		clock_delay		},
	{ "clock_hz",			clock_hz		},
	{ "clock_ihand",		clock_ihand		},
	{ "clock_intr",			clock_intr		},
	
	{ "outb",			outb			},
	{ "outw",			outw			},
	{ "outl",			outl			},
	
	{ "inb",			inb			},
	{ "inw",			inw			},
	{ "inl",			inl			},
	
	{ "insb",			insb			},
	{ "insw",			insw			},
	{ "insl",			insl			},
	
	{ "outsb",			outsb			},
	{ "outsw",			outsw			},
	{ "outsl",			outsl			},
	
#ifdef __ARCH_I386__
	{ "gdt_desc",			&gdt_desc		},
	{ "idt_desc",			&idt_desc		},
	{ "tss_desc",			&tss_desc		},
	{ "kv86_tss_desc",		&kv86_tss_desc		},
#endif
	
	{ "intr_aena",			intr_aena		},
	{ "intr_res",			intr_res		},
	{ "intr_ena",			intr_ena		},
	{ "intr_dis",			intr_dis		},
	{ "intr_enter",			intr_enter		},
	{ "intr_leave",			intr_leave		},
	{ "intr_set",			intr_set		},
	{ "ureg",			&ureg			},
	
	{ "pg_adget",			pg_adget		},
	{ "pg_adput",			pg_adput		},
	{ "pg_atmem",			pg_atmem		},
	{ "pg_dtmem",			pg_dtmem		},
	{ "pg_atdma",			pg_atdma		},
	{ "pg_dtdma",			pg_dtdma		},
	{ "pg_atphys",			pg_atphys		},
	{ "pg_dtphys",			pg_dtphys		},
	{ "pg_getphys",			pg_getphys		},
#ifdef __ARCH_I386__
	{ "pg_ena0",			pg_ena0			},
	{ "pg_dis0",			pg_dis0			},
#endif
	{ "pg_getdir",			pg_getdir		},
	
	{ "win_getdesktop",		win_getdesktop		},
	{ "win_display",		win_display		},
	{ "win_input",			win_input		},
	{ "win_uninstall_display",	win_uninstall_display	},
	{ "win_uninstall_input",	win_uninstall_input	},
	{ "win_keydown",		win_keydown		},
	{ "win_keyup",			win_keyup		},
	{ "win_ptrmove_rel",		win_ptrmove_rel		},
	{ "win_ptrmove",		win_ptrmove		},
	{ "win_ptrdown",		win_ptrdown		},
	{ "win_ptrup",			win_ptrup		},
	
	{ "phys_map",			phys_map		},
	{ "phys_unmap",			phys_unmap		},
	
	{ "memmove",			memmove			},
	{ "memcpy",			memcpy			},
	{ "memcmp",			memcmp			},
	{ "memset",			memset			},
	{ "strcpy",			strcpy			},
	{ "strcmp",			strcmp			},
	{ "strcat",			strcat			},
	{ "strncpy",			strncpy			},
	{ "strncmp",			strncmp			},
	{ "strlen",			strlen			},
	
	{ "kmalloc",			kmalloc			},
	{ "krealloc",			krealloc		},
	{ "malloc",			malloc			},
	{ "realloc",			realloc			},
	{ "free",			free			},
	
#if MACH_DMA_MALLOC
	{ "dma_malloc",			dma_malloc		},
	{ "dma_free",			dma_free		},
#endif
	
	{ "task_suspend",		task_suspend,		},
	{ "task_resume",		task_resume,		},
	{ "task_dequeue",		task_dequeue,		},
	{ "task_insert",		task_insert,		},
	{ "task_remove",		task_remove,		},
	{ "task_qinit",			task_qinit,		},
	{ "sched",			sched			},
	{ "curr",			&curr			},
	
	{ "mtx_init",			mtx_init		},
	{ "mtx_enter",			mtx_enter		},
	{ "mtx_leave",			mtx_leave		},
	
	{ "mq_init",			mq_init			},
	{ "mq_free",			mq_free			},
	{ "mq_put",			mq_put			},
	{ "mq_get",			mq_get			},
	
	{ "tucpy",			tucpy			},
	{ "fucpy",			fucpy			},
	{ "fustr",			fustr			},
	{ "uset",			uset			},
	{ "urchk",			urchk			},
	{ "uwchk",			uwchk			},
	
	{ "send_event",			send_event		},
	
	{ "on_shutdown",		on_shutdown		},
	
	{ "pwr_install",		pwr_install		},
	
	{ "kparam",			&kparam			},
	
	{ NULL,				NULL			}
};

int signal_pending(void)
{
	return curr->signal_pending;
}

uid_t geteuid(void)
{
	return curr->euid;
}

gid_t getegid(void)
{
	return curr->egid;
}

uid_t getruid(void)
{
	return curr->ruid;
}

gid_t getrgid(void)
{
	return curr->rgid;
}

static int mod_getksym(const char *name, intptr_t *buf)
{
	void *p;
	int i = 0;
	
	while (ksym[i].value)
	{
		if (!strcmp(ksym[i].name, name))
		{
			*buf = (intptr_t)ksym[i].value;
			return 0;
		}
		i++;
	}
	
	for (i = 0; i < sizeof module / sizeof *module; i++)
		if (module[i].in_use && !mod_pgetsym(&module[i], name, &p))
		{
			*buf = (intptr_t)p;
			return 0;
		}
	
	printk("mod_getksym: \"%s\": unresolved symbol\n", name);
	return ENSYM;
}

static int mod_reloc(struct module *m)
{
	int err;
	int i;
	
	for (i = 0; i < m->head->rel_count; i++)
	{
		err = mod_proc_rel(m, &m->rel[i]);
		if (err)
			return err;
	}
	
	return 0;
}

static int mod_resolve(struct module *m)
{
	intptr_t sym;
	intptr_t *p;
	
	struct modrel rel;
	int err;
	int i;
	
	for (i = 0; i < m->head->sym_count; i++)
	{
		if (m->sym[i].flags & SYM_IMPORT)
		{
			err = mod_getksym(m->sym[i].name, &sym);
			if (err)
				return err;
			
			rel.shift = m->sym[i].shift;
			rel.size  = m->sym[i].size;
			rel.addr  = m->sym[i].addr;
			rel.type  = MR_T_NORMAL;
#if defined __ARCH_I386__ || defined __ARCH_AMD64__
			if (m->sym[i].addr + 3 >= m->head->code_size)
				return ENOEXEC;
			
			p   = (intptr_t *)((intptr_t)m->code + m->sym[i].addr); // XXX abuse of intptr_t *
			*p += sym;
#else
#error Unknown arch
#endif
		}
	}
	
	return 0;
}

int mod_getslot(struct module **buf)
{
	struct module *m;
	int i;
	
	for (i = 0, m = module; i < sizeof module / sizeof *module; i++, m++)
		if (!m->in_use)
		{
			memset(m, 0, sizeof *m);
			m->in_use = 1;
			*buf = m;
			return 0;
		}
	return ENOMEM;
}

int mod_getid(struct module *m)
{
	return m - module;
}

int mod_init(struct module *m, const char *pathname, struct modhead *head, void *data, unsigned data_size)
{
	mod_onload_t *on_load;
	size_t nlen;
	const char *name;
	int err;
	int md;
	
	md = m - module;
	
	if (memcmp(head->magic, "\0KMODULE", 8))
		return ENOEXEC;
	
	name = strrchr(pathname, '/');
	if (name == NULL)
		name = pathname;
	else
		name++;
	
	nlen = strlen(name);
	if (nlen >= sizeof m->name)
		nlen = sizeof m->name - 1;
	
	memset(m, 0, sizeof *m);
	memcpy(m->name, name, nlen);
	m->head	  = head;
	m->rel	  = (struct modrel *) ((char *)head + head->rel_start);
	m->sym	  = (struct modsym *) ((char *)head + head->sym_start);
	m->code	  =		       (char *)head + head->code_start;
	m->in_use = 1;
	
	err = mod_reloc(m);
	if (err)
		return err;
	
	err = mod_resolve(m);
	if (err)
		return err;
	
	csync(m->code, head->code_size);
	
	err = mod_pgetsym(m, "mod_onload", (void **)&on_load);
	if (err)
		return err;
	
	err = on_load(md, pathname, data, data_size);
	if (err)
	{
		m->in_use = 0;
		return err;
	}
	
	return 0;
}

int mod_unload(unsigned md)
{
	int (*on_unload)(unsigned md);
	struct module *m;
	int err;
	
	if (md >= MODULE_MAX)
		return EBADM;
	
	m = &module[md];
	if (!m->in_use)
		return EBADM;
	
	err = mod_getsym(md, "mod_onunload", (void **)&on_unload);
	if (!err)
		on_unload(md);
	
	if (module[md].malloced)
		free(module[md].head);
	memset(&module[md], 0, sizeof module[md]);
	return 0;
}

int mod_pgetsym(struct module *m, const char *name, void **buf)
{
	int i;
	
	if (!m->in_use)
		return EBADM;
	
	for (i = 0; i < m->head->sym_count; i++)
		if ((m->sym[i].flags & SYM_EXPORT) &&
		    !strcmp(m->sym[i].name, name))
		{
			*buf = (void *)((intptr_t)m->code + m->sym[i].addr);
			return 0;
		}
	return ENSYM;
}

int mod_getsym(unsigned md, char *name, void **buf)
{
	if (md >= MODULE_MAX)
		return EBADM;
	return mod_pgetsym(&module[md], name, buf);
}

void mod_boot(void)
{
	struct kmodimg *bh;
	char mname[33];
	int err;
	int md;
	
	if (!kparam.mod_list)
		return;
	
#if KVERBOSE
	printk("mod_boot: booting modules\n");
#endif
	md = 0;
	for (bh = kparam.mod_list; bh; bh = bh->next)
	{
		if (md >= MODULE_MAX)
			panic("mod_boot: too many modules");
		
		memcpy(mname, bh->name, sizeof bh->name);
		mname[sizeof mname - 1] = 0;
		
		err = mod_init(&module[md], mname, bh->image, bh->args, bh->args_size);
		if (err)
		{
			char msg[32 + NAME_MAX];
			
			strcpy(msg, "mod_boot: ");
			strcat(msg, mname);
			strcat(msg, ": module init failed");
			
			perror("mod_boot: mod_init", err);
			panic(msg);
		}
		md++;
	}
#if KVERBOSE
	printk("mod_boot: fini\n");
#endif
}
