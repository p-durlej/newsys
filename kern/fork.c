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

#include <kern/machine/machine.h>
#include <kern/task_queue.h>
#include <kern/intr_regs.h>
#include <kern/syscall.h>
#include <kern/wingui.h>
#include <kern/signal.h>
#include <kern/printk.h>
#include <kern/sched.h>
#include <kern/errno.h>
#include <kern/fork.h>
#include <kern/task.h>
#include <kern/lib.h>
#include <kern/hw.h>

void uexec(void) __attribute__((noreturn));

int copy_pages(struct task *t);

static struct intr_regs *fork_ureg(struct task *t)
{
	return (struct intr_regs *)(&t->k_stack + 1) - 1;
}

void after_fork(struct intr_regs *r)
{
restart:
	signal_handle();
	if (curr->exited || curr->paused || resched)
	{
		sched();
		goto restart;
	}
	
	ureg = fork_ureg(curr);
	uret(0);
	
	uexec();
}

int do_fork(pid_t *pid)
{
	struct intr_regs *r;
	struct task *t;
	int err;
	int i;
	int s;
	
	err = task_getslot(&i);
	if (err)
		return err;
	task_count++;
	t = task[i];
	task_insert(&ready_queue[t->priority], t);
	
	s = intr_dis();
	t->k_sp		= (void *)1;
	t->parent	= curr;
	t->pid		= newpid();
	t->euid		= curr->euid;
	t->egid		= curr->egid;
	t->ruid		= curr->ruid;
	t->rgid		= curr->rgid;
	t->pg_count	= curr->pg_count;
	t->errno	= curr->errno;
	t->signal_entry	= curr->signal_entry;
	t->event_high	= curr->event_high;
	t->priority	= curr->priority;
	for (i = 0; i < NSIG; i++)
	{
		t->signal_received[i]  = curr->signal_received[i];
		t->signal_handler[i]   = curr->signal_handler[i];
		t->signal_use_event[i] = curr->signal_use_event[i];
	}
	t->signal_pending = curr->signal_pending;
	memcpy(t->exec_name, curr->exec_name, sizeof t->exec_name);
	memset(t->k_stack, 0x55, sizeof t->k_stack);
	intr_res(s);
	
	win_newtask(t);
	fs_newtask(t);
	
	r  = fork_ureg(t);
	*r = *ureg;
	
	err = copy_pages(t); // XXX failure to initialize the page directory will crash the kernel
	if (err)
	{
		signal_fatal(t, SIGOOM);
		return err;
	}
	
	*pid = t->pid;
	return 0;
}
