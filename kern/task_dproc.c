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

#include <kern/panic.h>
#include <kern/intr.h>
#include <kern/task.h>

static struct
{
	task_dproc *proc;
	void *cx;
} task_dprocs[64];

static int task_dproc_head;
static int task_dproc_cnt;

void task_defer(task_dproc *proc, void *cx)
{
	int i;
	int s;
	
	if (!proc)
		panic("task_defer: !proc");
	
	s = intr_dis();
	
	if (task_dproc_cnt >= sizeof task_dprocs / sizeof *task_dprocs)
		panic("task_defer: too many procs");
	
	i  = task_dproc_head + task_dproc_cnt;
	i %= sizeof task_dprocs / sizeof *task_dprocs;
	
	task_dproc_cnt++;
	
	intr_res(s);
	
	task_dprocs[i].proc = proc;
	task_dprocs[i].cx   = cx;
}

void task_rundp(void)
{
	task_dproc *proc;
	void *cx;
	int s;
	
	for (;;)
	{
		s = intr_dis();
		
		if (!task_dproc_cnt)
		{
			intr_res(s);
			return;
		}
		task_dproc_cnt--;
		
		proc = task_dprocs[task_dproc_head].proc;
		cx   = task_dprocs[task_dproc_head].cx;
		
		task_dproc_head++;
		task_dproc_head %= sizeof task_dprocs / sizeof *task_dprocs;
		
		intr_res(s);
		
		if (!proc)
			panic("task_rundp: !proc");
		proc(cx);
	}
}
