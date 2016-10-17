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
#include <kern/arch/selector.h>
#include <kern/task_queue.h>
#include <kern/config.h>
#include <kern/signal.h>
#include <kern/limits.h>
#include <kern/printk.h>
#include <kern/switch.h>
#include <kern/errno.h>
#include <kern/sched.h>
#include <kern/start.h>
#include <kern/intr.h>
#include <kern/task.h>
#include <kern/page.h>
#include <kern/umem.h>
#include <kern/exec.h>
#include <kern/lib.h>
#include <kern/fs.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <arch/archdef.h>
#include <fcntl.h>

struct task_queue ready_queue[TASK_PRIO_LOW + 1];

struct task *task[TASK_MAX];
struct task *curr;
struct task *init;
int task_pcount;
int task_count;

static vpage ptab_page;

pid_t newpid(void)
{
	static pid_t last = 0;
	int i;
	
restart:
	
	last++;
	if (last > PID_MAX)
		last = PID_RESTART;
	for (i = 0; i < TASK_MAX; i++)
		if (task[i] && task[i]->pid == last)
			goto restart;
	
	return last;
}

int task_getslot(int *p)
{
	vpage task_page;
	struct task *ts;
	int err;
	int i;
	
	for (i = 0; i < TASK_MAX; i++)
		if (!task[i])
		{
			task_page = ptab_page + i * (PAGES_PER_TASK + 1);
			err = pg_atmem(task_page, PAGES_PER_TASK, 0);
			if (err)
				return err;
			
			ts = (void *)((intptr_t)task_page * PAGE_SIZE);
			memset(ts, 0, sizeof *ts);
			task[i] = ts;
			*p = i;
			return 0;
		}
	
	printk("task.c: task_getslot: task table full\n");
	return EAGAIN;
}

void task_putslot(int i)
{
	task[i] = NULL;
	pg_dtmem(ptab_page + i * (PAGES_PER_TASK + 1), PAGES_PER_TASK);
}

void task_init(void)
{
	int err;
	int i;
	
#if KVERBOSE
	printk("task.c: task_init: PAGES_PER_TASK      = %i\n", PAGES_PER_TASK);
	printk("task.c: task_init: TASK_MAX            = %i\n", TASK_MAX);
	printk("task.c: task_init: sizeof(struct task) = %i\n", sizeof(struct task));
#endif
	
	for (i = 0; i < TASK_MAX; i++)
		task[i] = NULL;
	
	if (pg_adget(&ptab_page, (PAGES_PER_TASK + 1) * TASK_MAX))
		panic("task_init: pg_adget failed");
	
#if KVERBOSE
	printk("task.c: task_init: ptab_page           = %i\n", ptab_page);
#endif
	
	for (i = 0; i <= TASK_PRIO_LOW; i++)
		task_qinit(&ready_queue[i], "ready");
	
	err = task_getslot(&i);
	if (err)
		panic("task_init: getslot failed");
	
	curr = init = task[i];
	curr->priority	 = TASK_PRIO_USER;
	curr->last_event = -1;
	curr->parent	 = curr;
	curr->pid	 = newpid();
	if (pg_newdir(curr->pg_dir))
		panic("task_init: pg_newdir failed");
	pg_setdir(curr->pg_dir);
	
	for (i = 0; i < NSIG; i++)
		curr->signal_handler[i] = SIG_IGN;
	curr->signal_handler[SIGCONT - 1] = SIG_DFL;
	
	task_pcount = 0;
	task_count  = 1;
	
#if KVERBOSE
	printk("task.c: task_init: done\n");
#endif
}

int task_exec(const char *name, const void *arg, int arg_size)
{
	char fqn[PATH_MAX];
	int err;
	
	err = fs_fqname(fqn, name);
	if (err)
		return err;
	
	curr->exec_arg_size = arg_size;
	curr->exec	    = 1;
	strcpy(curr->exec_name, fqn);
	memcpy(curr->exec_arg, arg, arg_size);
	return 0;
}

int task_suspend(struct task_queue *tq, int type)
{
	int s;
	
	if (curr->paused)
		panic("task_suspend: curr->paused");
	
	if (tq >= ready_queue && tq < ready_queue + TASK_PRIO_LOW + 1)
		panic("task_suspend: tq == &ready_queue");
	
	if (type != WAIT_INTR && type != WAIT_NOINTR)
		panic("task_suspend: bad type");
	
	s = intr_dis();
	if ((curr->signal_pending || curr->unseen_events) && !curr->stopped && type == WAIT_INTR)
	{
		curr->unseen_events = 0;
		intr_res(s);
		return EINTR;
	}
	curr->paused = type;
	task_pcount++;
	
	if (tq)
		task_insert(tq, curr);
	
	while (curr->paused)
		sched();
	intr_res(s);
	
	if (curr->paused)
		panic("task_suspend: curr->paused");
	return 0;
}

void task_resume(struct task *task)
{
	int s;
	
	if (task == NULL)
		panic("task_resume: task == NULL");
	if (task->exited)
		panic("task_resume: task->exited");
	
	s = intr_dis();
	if (task->paused)
	{
		task->time_slice = TIME_SLICE;
		task->paused = 0;
		task_pcount--;
		if (task_pcount < 0)
			panic("task_resume: task_pcount < 0");
		if (task->queue)
			task_remove(task);
		if (task != curr)
			task_insert(&ready_queue[task->priority], task);
		else
			printk("task_resume: resumed curr\n");
		
		if (curr != NULL && task->priority < curr->priority)
			resched = 1;
	}
	intr_res(s);
}

int task_creat(const char *name, const void *arg, int arg_size, pid_t *pid)
{
	char fqn[PATH_MAX];
	struct task *p;
	int err;
	int cnt;
	int i;
	
	if (curr->ruid)
	{
		cnt = 0;
		for (i = 0; i < TASK_MAX; i++)
			if (task[i] != NULL && task[i]->ruid)
				cnt++;
	}
	
	err = fs_fqname(fqn, name);
	if (err)
		return err;
	
	err = task_getslot(&i);
	if (err)
		return err;
	p  = task[i];
	*p = *curr;
	
	p->parent	 = curr;
	p->pid		 = newpid();
	p->time_slice	 = -1;
	p->priority	 = curr->priority;
	p->alarm_repeat	 = 0;
	p->alarm	 = 0;
	p->unseen_events = 0;
	p->event_count	 = 0;
	p->first_event	 = 0;
	p->last_event	 = -1;
	p->pg_count	 = 0;
	
	err = pg_newdir(p->pg_dir);
	if (err)
	{
		task_putslot(i);
		return err;
	}
	
	memset(&p->mach_task, 0, sizeof p->mach_task);
	memset(&p->k_stack, 0x55, sizeof p->k_stack);
	
	p->exec_arg_size = arg_size;
	strcpy(p->exec_name, fqn);
	memcpy(p->exec_arg, arg, arg_size);
	
	memset(&p->win_task, 0, sizeof p->win_task);
	win_newtask(p);
	fs_newtask(p);
	
	p->signal_pending = 0;
	for (i = 0; i < NSIG; i++)
		if (p->signal_handler[i] != SIG_IGN)
		{
			p->signal_handler[i]  = SIG_DFL;
			p->signal_received[i] = 0;
		}
	
	p->k_sp = NULL;
	
	task_insert(&ready_queue[p->priority], p);
	task_count++;
	
	*pid = p->pid;
	return 0;
}

void new_task_entry(void)
{
	struct intr_regs u;
	
	if (fpu_present)
		fpu_init();
	
	memset(&u, 0, sizeof u);
	ureg = &u;
	
#ifdef __ARCH_I386__
	asm("pushfl; popl %0" : "=d" (u.eflags));
#elif defined __ARCH_AMD64__
	asm("pushfq; popq %0" : "=d" (u.rflags));
#else
#error Unknown arch
#endif
	if (do_exec())
		task_exit(SIGEXEC);
	
	uexec();
}

void task_exit(int status)
{
	int win_detach(void); /* XXX */
	
	int i;
	
	if (curr->exited)
		return;
	
	if (curr == init)
	{
		printk("task_exit: init is exiting with status 0x%x\n", status);
		dump_regs(ureg, "           ureg->");
		panic("init exited");
	}
	
	win_detach();
	uclean();
	fs_clean();
	
	for (i = 0; i < TASK_MAX; i++)
		if (task[i] && task[i]->parent == curr)
		{
			task[i]->parent = init;
			if (task[i]->exited)
				signal_send_k(init, SIGCHLD);
		}
	
	if (curr->parent->signal_handler[SIGCHLD - 1] != SIG_IGN)
		signal_send_k(curr->parent, SIGCHLD);
	else
	{
		curr->parent = init;
		signal_send_k(init, SIGCHLD);
	}
	
	task_count--;
	if (task_count < 0)
		panic("task_exit: task_count < 0");
	
	curr->exit_status = status;
	curr->exited = 1;
	sched();
	panic("task_exit: task is still running");
}

void task_qinit(struct task_queue *tq, const char *name)
{
	tq->first = tq->last = NULL;
	tq->name = name;
}

void task_insert(struct task_queue *tq, struct task *task)
{
	int s;
	
	if (task->queue)
	{
		printk("tq              = %p\n", tq);
		printk("task            = %p\n", task);
		printk("task->exited    = %i\n", task->exited);
		printk("task->paused    = %i\n", task->paused);
		printk("task->queue     = %p\n", task->queue);
		printk("task->pid       = %i\n", task->pid);
		printk("task->exec_name = \"%s\"\n", task->exec_name);
		panic("task_insert: task->queue");
	}
	
	s = intr_dis();
	if (!tq->first)
	{
		tq->first   = tq->last = task;
		task->prev  = NULL;
		task->next  = NULL;
		task->queue = tq;
		intr_res(s);
		return;
	}
	task->prev     = tq->last;
	task->next     = NULL;
	task->queue    = tq;
	tq->last->next = task;
	tq->last       = task;
	intr_res(s);
}

struct task *task_dequeue(struct task_queue *tq)
{
	struct task *t;
	int s;
	
	s = intr_dis();
	if (tq->first)
	{
		t = tq->first;
		
		tq->first = t->next;
		if (tq->first)
			tq->first->prev = NULL;
		else
			tq->last = NULL;
		
		t->queue = NULL;
		t->next  = NULL;
		t->prev  = NULL;
		intr_res(s);
		return t;
	}
	intr_res(s);
	return NULL;
}

void task_remove(struct task *t)
{
	struct task_queue *tq = t->queue;
	int s;
	
	if (!tq)
		panic("task_remove: !tq");
	
	s = intr_dis();
	if (t == tq->first)
	{
		if (t == tq->last)
		{
			t->queue = NULL;
			t->prev  = NULL;
			t->next  = NULL;
			tq->first = NULL;
			tq->last  = NULL;
			intr_res(s);
			return;
		}
		
		tq->first	= t->next;
		tq->first->prev = NULL;
		t->queue = NULL;
		t->prev  = NULL;
		t->next  = NULL;
		intr_res(s);
		return;
	}
	if (t == tq->last)
	{
		tq->last       = tq->last->prev;
		tq->last->next = NULL;
		t->queue = NULL;
		t->prev  = NULL;
		t->next  = NULL;
		intr_res(s);
		return;
	}
	t->prev->next = t->next;
	t->next->prev = t->prev;
	t->queue = NULL;
	t->prev  = NULL;
	t->next  = NULL;
	intr_res(s);
	return;
}

int task_find(struct task **tp, pid_t pid)
{
	int i;
	
	for (i = 0; i < TASK_MAX; i++)
	{
		if (task[i] == NULL || task[i]->pid != pid)
			continue;
		
		*tp = task[i];
		return 0;
	}
	return ESRCH;
}

void task_set_prio(int prio)
{
	if (prio < 0 || prio > TASK_PRIO_LOW)
		panic("task_set_prio: bad prio");
	
	curr->priority = prio;
}
