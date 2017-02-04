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

#include <kern/mqueue.h>
#include <kern/errno.h>
#include <kern/task.h>
#include <kern/intr.h>
#include <kern/lib.h>
#include <overflow.h>

int mq_init(struct mqueue *mq, const char *name, size_t itemsz, size_t maxcnt)
{
	size_t sz;
	int err;
	
	memset(mq, 0, sizeof *mq);
	
	if (ov_mul_size(itemsz, maxcnt))
		return EINVAL;
	sz = itemsz * maxcnt;
	
	err = kmalloc(&mq->buf, sz, name);
	if (err)
		return err;
	
	task_qinit(&mq->putq, name);
	task_qinit(&mq->getq, name);
	
	mq->name   = name;
	mq->itemsz = itemsz;
	mq->maxcnt = maxcnt;
	return 0;
}

void mq_free(struct mqueue *mq)
{
	free((void *)mq->name);
	memset(mq, 0, sizeof *mq);
}

int mq_put(struct mqueue *mq, const void *data, int wait)
{
	struct task *t;
	void *dp;
	int err;
	int s;
	
	s = intr_dis();
	while (mq->curcnt >= mq->maxcnt)
	{
		if (!wait)
		{
			intr_res(s);
			return EAGAIN;
		}
		err = task_suspend(&mq->putq, wait);
		if (err)
		{
			intr_res(s);
			return err;
		}
	}
	
	dp = (char *)mq->buf + (mq->first + mq->curcnt) % mq->maxcnt;
	memcpy(dp, data, mq->itemsz);
	
	mq->curcnt++;
	
	intr_res(s);
	
	while (t = task_dequeue(&mq->getq), t)
		task_resume(t);
	return 0;
}

int mq_get(struct mqueue *mq, void *data, int wait)
{
	struct task *t;
	const void *sp;
	int err;
	int s;
	
	s = intr_dis();
	
	while (!mq->curcnt)
	{
		if (!wait)
		{
			intr_res(s);
			return EAGAIN;
		}
		err = task_suspend(&mq->getq, wait);
		if (err)
		{
			intr_res(s);
			return err;
		}
	}
	
	sp = (char *)mq->buf + mq->first % mq->maxcnt;
	memcpy(data, sp, mq->itemsz);
	
	mq->curcnt--;
	mq->first++;
	mq->first %= mq->maxcnt;
	
	intr_res(s);
	
	while (t = task_dequeue(&mq->putq), t)
		task_resume(t);
	return 0;
}
