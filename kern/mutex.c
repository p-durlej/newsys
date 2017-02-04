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

#include <kern/console.h>
#include <kern/errno.h>
#include <kern/mutex.h>
#include <kern/intr.h>
#include <kern/task.h>
#include <kern/lib.h>

void mtx_init(struct mutex *mtx, const char *name)
{
	memset(mtx, 0, sizeof *mtx);
	task_qinit(&mtx->tq, name);
}

int mtx_enter(struct mutex *mtx, int wait)
{
	int err;
	int s;
	
	s = intr_dis();
	if (mtx->owner == NULL)
	{
		mtx->owner   = curr;
		mtx->lockcnt = 1;
		intr_res(s);
		return 0;
	}
	if (mtx->owner == curr)
	{
		mtx->lockcnt++;
		intr_res(s);
		return 0;
	}
	while (mtx->lockcnt)
	{
		if (!wait)
		{
			intr_res(s);
			return EAGAIN;
		}
		
		err = task_suspend(&mtx->tq, wait);
		if (err)
		{
			intr_res(s);
			return err;
		}
	}
	mtx->owner   = curr;
	mtx->lockcnt = 1;
	intr_res(s);
	return 0;
}

void mtx_leave(struct mutex *mtx)
{
	struct task *t;
	
	if (mtx->owner != curr)
		panic("mtx_leave: mtx->owner != curr");
	
	mtx->lockcnt--;
	if (mtx->lockcnt)
		return;
	
	mtx->owner = NULL;
	while (t = task_dequeue(&mtx->tq), t != NULL)
		task_resume(t);
}
