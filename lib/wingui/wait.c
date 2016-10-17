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

#include <wingui_color.h>
#include <signal.h>
#include <wingui.h>
#include <errno.h>
#include <event.h>

static void *win_signal_handler[NSIG + 1];

static void handle_win_signal(struct event *e)
{
	void (*proc)(int nr) = win_signal_handler[e->sig];
	
	proc(e->sig);
}

int win_signal(int nr, void *proc)
{
	if (nr < 1 || nr > NSIG)
	{
		_set_errno(EINVAL);
		return -1;
	}
	
	if (proc == SIG_IGN || proc == SIG_DFL)
	{
		if (signal(nr, proc) == (void *)-1)
			return -1;
		return 0;
	}
	
	win_signal_handler[nr] = proc;
	evt_signal(nr, handle_win_signal);
	return 0;
}

int win_event_count(void)
{
	return evt_count();
}

int win_wait(void)
{
	evt_wait();
	return 0;
}

int win_idle(void)
{
	evt_idle();
	return 0;
}
