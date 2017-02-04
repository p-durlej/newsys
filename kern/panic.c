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

#include <kern/machine/machine.h>
#include <kern/machine/start.h>
#include <kern/console.h>
#include <kern/printk.h>
#include <kern/module.h>
#include <kern/config.h>
#include <kern/panic.h>
#include <kern/task.h>
#include <kern/exec.h>
#include <kern/lib.h>
#include <arch/archdef.h>

static panic_proc *panic_handler;

static unsigned panicking;

void *getbp();

static struct module *find_module(void *p)
{
	struct module *m = NULL;
	int i;
	
	for (i = 0; i < MODULE_MAX; i++)
	{
		if (!module[i].in_use)
			continue;
		if (module[i].code > p)
			continue;
		if (m != NULL && module[i].code < m->code)
			continue;
		m = &module[i];
	}
	return m;
}

void backtrace(void *bp0)
{
	struct module *m;
	void **bp = bp0;
	
	printk("\n\n **** backtrace listing\n\n");
	while (bp)
	{
		m = find_module(bp[1]);
		if (m != NULL)
			printk("  %p %s + %p\n", bp[1], m->name, (char *)bp[1] - (char *)m->code);
		else
			printk("  %p\n", bp[1]);
		bp = *bp;
	}
}

void panic_install(panic_proc *proc)
{
	panic_handler = proc;
}

#if VERBOSE_PANIC

void panic(const char *msg)
{
	panicking++;
	
	if (panic_handler)
		panic_handler(msg);
	
	if (panicking < 4)
	{
		con_sstatus(CSTATUS_ERROR | CSTATUS_PANIC | CSTATUS_HALT);
		con_reset();
		intr_dis();
	}
	else
	{
		intr_dis();
		for (;;);
	}
	
	if (panicking < 3)
	{
		if (curr)
			printk("\n\n **** kernel panic in task %i (%s)\n\n", curr->pid, curr->exec_name);
		else
			printk("\n\n **** kernel panic\n\n");
		printk(" %s", msg);
	}
	
	if (panicking < 2)
		backtrace(getbp());
	
	printk("\n\n **** system halted");
	intr_dis();
	for (;;)
		asm("hlt");
}

#else

void panic(const char *msg)
{
	panicking++;
	
	if (panic_handler)
		panic_handler(msg);
	
	if (panicking < 3)
	{
		con_sstatus(CSTATUS_ERROR | CSTATUS_PANIC | CSTATUS_HALT);
		con_reset();
		intr_dis();
	}
	else
	{
		intr_dis();
		for (;;);
	}
	
	if (panicking < 2)
	{
		printk("panic: %s\n", msg);
		printk("panic: system halted\n");
	}
	
	intr_dis();
	for (;;)
		asm("hlt");
}

#endif
