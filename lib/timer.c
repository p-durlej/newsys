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

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <event.h>
#include <timer.h>
#include <errno.h>
#include <time.h>
#include <list.h>

struct timer
{
	struct list_item list_item;
	int		 listed;
	
	struct timeval	repeat;
	struct timeval	delay;
	
	timer_cb *	cb;
	void *		data;
};

static struct list tmr_list;

static int tv_cmp(const struct timeval *tv1, const struct timeval *tv2)
{
	if (tv1->tv_sec > tv2->tv_sec)
		return 1;
	if (tv1->tv_sec < tv2->tv_sec)
		return -1;
	if (tv1->tv_usec > tv2->tv_usec)
		return 1;
	if (tv1->tv_usec < tv2->tv_usec)
		return -1;
	return 0;
}

static void tv_add(struct timeval *d, const struct timeval *s)
{
	d->tv_sec  += s->tv_sec;
	d->tv_usec += s->tv_usec;
	d->tv_sec  += d->tv_usec / 1000000;
	d->tv_usec %= 1000000;
}

static void tv_sub(struct timeval *d, const struct timeval *s)
{
	if (d->tv_usec < s->tv_usec)
	{
		d->tv_usec += 1000000;
		d->tv_sec--;
	}
	d->tv_usec -= s->tv_usec;
	d->tv_sec  -= s->tv_sec;
}

static void tmr_insert(struct timer *tmr)
{
	struct timer *p;
	
	for (p = list_first(&tmr_list); p; p = list_next(&tmr_list, p))
		if (tv_cmp(&tmr->delay, &p->delay) < 0)
			break;
	if (p)
		list_ib(&tmr_list, p, tmr);
	else
		list_app(&tmr_list, tmr);
	tmr->listed = 1;
}

static void tmr_sig_alrm(int nr)
{
	struct timeval tv, d;
	struct timer *tmr;
	struct timer *n;
	
	evt_signal(SIGALRM, tmr_sig_alrm);
	gettimeofday(&tv, NULL);
restart:
	tmr = list_first(&tmr_list);
	while (tmr && tv_cmp(&tv, &tmr->delay) >= 0)
	{
		list_rm(&tmr_list, tmr);
		tv_add(&tmr->delay, &tmr->repeat);
		tmr_insert(tmr);
		n = list_next(&tmr_list, tmr);
		tmr->cb(tmr->data);
	}
	
	tmr = list_first(&tmr_list);
	if (!tmr)
		return;
	gettimeofday(&tv, NULL);
	d = tmr->delay;
	if (tv_cmp(&tv, &d) >= 0)
		goto restart;
	tv_sub(&d, &tv);
	
	if (d.tv_sec)
		alarm(d.tv_sec);
	else if (d.tv_usec)
		ualarm(d.tv_usec, 0);
}

struct timer *tmr_creat(const struct timeval *delay, const struct timeval *repeat, timer_cb *cb, void *data, int start)
{
	struct timer *tmr;
	
	if (!repeat->tv_sec && !repeat->tv_usec)
	{
		_set_errno(EINVAL);
		return NULL;
	}
	
	evt_signal(SIGALRM, tmr_sig_alrm);
	tmr = calloc(sizeof *tmr, 1);
	if (!tmr)
		return NULL;
	gettimeofday(&tmr->delay, NULL);
	tv_add(&tmr->delay, delay);
	tmr->repeat = *repeat;
	tmr->cb	    = cb;
	tmr->data   = data;
	if (start)
		tmr_start(tmr);
	return tmr;
}

void tmr_free(struct timer *tmr)
{
	tmr_stop(tmr);
	free(tmr);
}

void tmr_reset(struct timer *tmr, const struct timeval *delay, const struct timeval *repeat)
{
	int r;
	
	if (!repeat->tv_sec && !repeat->tv_usec)
	{
		_set_errno(EINVAL);
		return;
	}
	
	r = tmr->listed;
	if (r)
		tmr_stop(tmr);
	gettimeofday(&tmr->delay, NULL);
	tv_add(&tmr->delay, delay);
	tmr->repeat = *repeat;
	if (r)
		tmr_start(tmr);
}

void tmr_start(struct timer *tmr)
{
	if (tmr->listed)
		return;
	tmr->listed = 1;
	tmr_insert(tmr);
	tmr_sig_alrm(0);
}

void tmr_stop(struct timer *tmr)
{
	if (!tmr->listed)
		return;
	list_rm(&tmr_list, tmr);
	tmr->listed = 0;
}
