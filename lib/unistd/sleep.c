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

#include <sys/signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <timer.h>
#include <event.h>
#include <errno.h>
#include <time.h>

static struct timer *sleep_timer;
static int sleep_done;

static void sleep_cb(void *data)
{
	sleep_done = 1;
}

static void mktimer(void)
{
	struct timeval tv = { 999999, 0 };
	
	if (!sleep_timer)
		sleep_timer = tmr_creat(&tv, &tv, sleep_cb, NULL, 0);
}

int sleep(int sec)
{
	time_t ret;
	
	if (!sec)
		return 0;
	
	ret = time(NULL) + sec;
	usleep2(sec * 1000000, 1);
	ret -= time(NULL);
	
	if (ret <= 0)
		return 0;
	return ret;
}

int usleep2(useconds_t us, int intr)
{
	struct timeval tv, rep = { 999999, 0 };
	
	if (!us)
		return 0;
	
	mktimer();
	if (!sleep_timer)
		return -1;
	
	tv.tv_sec  = us / 1000000;
	tv.tv_usec = us % 1000000;
	
	tmr_reset(sleep_timer, &tv, &rep);
	sleep_done = 0;
	
	tmr_start(sleep_timer);
	do
		evt_wait();
	while (!sleep_done && !intr);
	tmr_stop(sleep_timer);
	
	if (!sleep_done)
	{
		_set_errno(EINTR);
		return -1;
	}
	return 0;
}

int usleep(useconds_t us)
{
	return usleep2(us, 1);
}
