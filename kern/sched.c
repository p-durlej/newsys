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
#include <kern/task_queue.h>
#include <kern/console.h>
#include <kern/switch.h>
#include <kern/printk.h>
#include <kern/config.h>
#include <kern/signal.h>
#include <kern/sched.h>
#include <kern/page.h>
#include <kern/task.h>
#include <kern/lib.h>
#include <kern/hw.h>

extern int fpu_present;

int switch_freq = 0;
int switch_cnt = 0;
int resched = 0;

void sched(void)
{
	struct task *next;
	struct task *prev;
	int i;
	int s;
	
	s = intr_dis();
	resched = 0;
	if (task_pcount + 1 == task_count && !curr->paused && !curr->exited)
	{
		curr->time_slice = TIME_SLICE;
		intr_res(s);
		return;
	}
	prev = curr;
	curr = NULL;
	if (!prev->exited && !prev->paused)
		task_insert(&ready_queue[prev->priority], prev);
	intr_ena();
	
	intr_dis();
	while (task_count == task_pcount)
		asm volatile("sti; hlt; cli");
	intr_ena();
	
	for (;;)
	{
		for (i = 0; i <= TASK_PRIO_LOW; i++)
		{
			next = task_dequeue(&ready_queue[i]);
			if (next != NULL)
				break;
		}
		if (next == NULL)
		{
			printk("task_pcount = %i\n", task_pcount);
			printk("task_count  = %i\n", task_count);
			panic("sched: next == NULL");
		}
		
		next->time_slice += TIME_SLICE;
		if (next->time_slice > TIME_SLICE_MAX)
			next->time_slice = TIME_SLICE_MAX;
		
		if (next->time_slice < 0)
		{
			task_insert(&ready_queue[next->priority], next);
			continue;
		}
		
		if (next->exited || next->paused)
		{
			printk("next            = %p\n", next);
			printk("next->exited    = %i\n", next->exited);
			printk("next->paused    = %i\n", next->paused);
			printk("next->pid       = %i\n", next->pid);
			printk("next->exec_name = \"%s\"\n", next->exec_name);
			panic("sched: next->exited || next->paused");
		}
		break;
	}
	
	if (prev != next)
	{
		pg_setdir(next->pg_dir);
		
		if (fpu_present)
		{
			if (prev)
			{
				fpu_save(prev->fpu_state);
				prev->fpu_saved = 1;
			}
			if (next->fpu_saved)
				fpu_load(next->fpu_state);
			else
				fpu_init();
		}
		
		intr_dis();
		switch_cnt++;
		intr_ena();
		
		intr_dis();
		switch_stack(prev, next);
		intr_res(s);
	}
	else
	{
		curr = next;
		intr_res(s);
	}
}
