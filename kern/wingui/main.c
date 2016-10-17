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

#include <kern/shutdown.h>
#include <kern/wingui.h>
#include <kern/task.h>
#include <errno.h>

int win_chkwd(int wd)
{
	struct win_desktop *d = curr->win_task.desktop;
	
	if (!d)
		return ENODESKTOP;
	
	if (wd < 0 || wd >= WIN_MAX)
		return EINVAL;
	
	/* if (curr->uid && d->uid != curr->uid)
		return EPERM; */
	
	if (!d->window[wd].task)
		return EINVAL;
	
	if (/* !curr->win_task.xaccess && */ curr != d->window[wd].task)
		return EPERM;
	
	return 0;
}

void win_newtask(struct task *p)
{
	if (curr->win_task.inh_desktop)
	{
		p->win_task.inh_desktop = curr->win_task.inh_desktop;
		p->win_task.inh_desktop->refcnt++;
	}
}

void win_clean(void)
{
	struct win_desktop *d = curr->win_task.desktop;
	int i;
	
	if (!d)
		return;
	
	if (curr == d->taskbar)
	{
		win_broadcast_resize(d);
		d->taskbar = NULL;
	}
	
	while (curr->win_task.painting)
		win_end_paint();
	
	for (i = 0; i < WIN_MAX; i++)
		if (d->window[i].task == curr)
			win_close(i);
	
	curr->win_task.desktop = NULL;
}

static void win_shutdown(int type)
{
	struct win_desktop *d;
	
	for (d = list_first(&win_desktops); d; d = list_next(&win_desktops, d))
		d->display_locked++;
}

void win_init(void)
{
	list_init(&win_desktops, struct win_desktop, list_item);
	
	on_shutdown(win_shutdown);
}
