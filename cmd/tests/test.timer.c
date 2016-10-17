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

#include <stdio.h>
#include <timer.h>
#include <event.h>
#include <time.h>

static struct timer_data
{
	struct timeval	repeat;
	struct timeval	delay;
	char *		name;
	int		exp_cnt;
	int		cnt;
} timers[] = 
{
	{ { 0, 500000	}, { 9, 0	}, "timer0", 3 },
	{ { 1, 0	}, { 1, 0	}, "timer1", 10 },
	{ { 2, 0	}, { 2, 0	}, "timer2", 5  },
};

static int err;
static int com;

static void cb(void *data)
{
	struct timer_data *t = data;
	struct timeval tv;
	
	gettimeofday(&tv, NULL);
	printf("%s, sec = %li, usec = %li\n", t->name, (long)tv.tv_sec, (long)tv.tv_usec);
	if (t->exp_cnt <= ++t->cnt)
		com++;
}

int main(int argc, char **argv)
{
	struct timeval tv;
	struct timer *t;
	int i;
	
	gettimeofday(&tv, NULL);
	printf("sec = %li, usec = %li\n", (long)tv.tv_sec, (long)tv.tv_usec);
	
	for (i = 0; i < sizeof timers / sizeof *timers; i++)
	{
		t = tmr_creat(&timers[i].delay, &timers[i].repeat, cb, &timers[i], 0);
		if (!t)
		{
			perror("tmr_creat");
			return 1;
		}
		tmr_start(t);
	}
	
	while (com < sizeof timers / sizeof *timers)
		evt_wait();
	for (i = 0; i < sizeof timers / sizeof *timers; i++)
		if (timers[i].exp_cnt != timers[i].cnt)
		{
			printf("%s: count = %i, expected = %i\n", timers[i].name, timers[i].cnt, timers[i].exp_cnt);
			err = 1;
		}
	return err;
}
