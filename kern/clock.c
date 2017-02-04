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
#include <kern/signal.h>
#include <kern/sched.h>
#include <kern/clock.h>
#include <kern/intr.h>
#include <kern/task.h>
#include <kern/lib.h>

static volatile int		cputime_cnt;
static volatile int		cputime;

static volatile time_t		uptime;
static volatile unsigned	upticks;

volatile time_t			time = 1;
volatile unsigned		ticks;

volatile unsigned		alarm_ticks = 0;
volatile time_t			alarm_time = 0;

struct clock_handler
{
	void	(*proc)(void *cx);
	void *	cx;
};

static struct clock_handler *clock_procs;
static int clock_proc_cnt;

void set_alarm(time_t time, unsigned ticks)
{
	int s;
	
	s = intr_dis();
	if (!alarm_time || time < alarm_time || (time == alarm_time && ticks < alarm_ticks))
	{
		alarm_ticks = ticks;
		alarm_time  = time;
	}
	intr_res(s);
}

void chk_alarm(struct task *p)
{
	if (p->alarm)
	{
		unsigned tc;
		time_t tm;
		int s;
		
		s = intr_dis();
		tc = ticks;
		tm = time;
		intr_res(s);
		
		if ((p->alarm == tm && p->alarm_ticks <= tc) || p->alarm < tm)
		{
			signal_send_k(p, SIGALRM);
			p->alarm = 0;
			
			if (p->alarm_repeat)
			{
				p->alarm_ticks = tc + p->alarm_repeat;
				p->alarm = tm;
				if (p->alarm_ticks >= clock_hz())
				{
					p->alarm += p->alarm_ticks / clock_hz();
					p->alarm_ticks %= clock_hz();
				}
				set_alarm(p->alarm, p->alarm_ticks);
			}
		}
		else
			set_alarm(p->alarm, p->alarm_ticks);
	}
}

void clock_intr(int count)
{
	void win_clock(void);
	void fd_clock(void);
	
	int hz;
	int i;
	
	hz = clock_hz();
	
	if (curr != NULL)
	{
		curr->cputime_cnt++;
		cputime_cnt++;
	}
	
	if (++upticks >= hz)
	{
		for (i = 0; i < TASK_MAX; i++)
			if (task[i] != NULL)
			{
				task[i]->cputime = task[i]->cputime_cnt++;
				task[i]->cputime_cnt = 0;
			}
		
		switch_freq = switch_cnt;
		switch_cnt  = 0;
		
		cputime	    = cputime_cnt;
		cputime_cnt = 0;
		
		upticks = 0;
		uptime++;
	}
	
	if (++ticks >= hz)
	{
		ticks = 0;
		time++;
	}
	
	if (curr)
	{
		curr->time_slice--;
		if (curr->time_slice < 0)
			resched = 1;
	}
	
	if (alarm_time)
	{
		if ((alarm_time == time && alarm_ticks <= ticks) || alarm_time < time)
		{
			alarm_time = 0;
			
			for (i = 0; i < TASK_MAX; i++)
				if (task[i])
					chk_alarm(task[i]);
		}
	}
	
	for (i = 0; i < clock_proc_cnt; i++)
		clock_procs[i].proc(clock_procs[i].cx);
	win_clock();
	fs_clock();
}

int clock_cputime(void)
{
	return cputime; /* XXX i386 */
}

time_t clock_uptime(void)
{
	return uptime; /* XXX i386 */
}

unsigned clock_ticks(void)
{
	return ticks; /* XXX i386 */
}

time_t clock_time(void)
{
	return time; /* XXX i386 */
}

void clock_delay(time_t delay)
{
	unsigned end_ticks;
	time_t end_time;
	int s;
	
	s = intr_dis();
	end_ticks = ticks;
	end_time  = time;
	intr_res(s);
	
	end_ticks += delay;
	end_time  += end_ticks / clock_hz();
	end_ticks %= clock_hz();
	
	intr_aena();
	while (time < end_time);
	while (ticks < end_ticks && time == end_time);
}

int clock_hz(void)
{
	return 250; /* XXX */
}

int clock_ihand2(void (*proc)(void *), void *cx)
{
	struct clock_handler *n, *o;
	int err;
	int s;
	
	err = kmalloc(&n, (clock_proc_cnt + 1) * sizeof *n, "clock");
	if (err)
		return err;
	memcpy(n, clock_procs, clock_proc_cnt * sizeof *n);
	n[clock_proc_cnt].proc = proc;
	n[clock_proc_cnt].cx   = cx;
	
	s = intr_dis();
	clock_proc_cnt++;
	o = clock_procs;
	clock_procs = n;
	intr_res(s);
	
	free(o);
	return 0;
}

int clock_ihand(void (*proc)(void))
{
	return clock_ihand2((void *)proc, NULL); /* XXX */
}
