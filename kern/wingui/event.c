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

#include <kern/printk.h>
#include <kern/wingui.h>
#include <kern/signal.h>
#include <kern/event.h>
#include <kern/clock.h>
#include <kern/task.h>
#include <kern/intr.h>
#include <kern/lib.h>
#include <kern/hw.h>
#include <errno.h>
#include <event.h>
#include <list.h>

int win_event(struct win_desktop *d, struct event *e)
{
	struct task *t = d->window[e->win.wd].task;
	
	e->proc = d->window[e->win.wd].proc;
	e->data = d->window[e->win.wd].data;
	e->type = E_WINGUI;
	return send_event(t, e);
}

static void win_sak(struct win_desktop *d)
{
	struct event e;
	
	d->sec_locked = 1;
	
	memset(&e, 0, sizeof e);
	e.win.type = WIN_E_SEC_LOCK;
	e.type	   = E_WINGUI;
	win_bcast(d, &e);
}

int win_keydown(struct win_desktop *d, unsigned ch)
{
#define SAK_SHIFT	(WIN_SHIFT_CTRL|WIN_SHIFT_ALT)
	struct event e;
	
	switch (ch)
	{
	case WIN_KEY_SHIFT:
		d->key_shift |= WIN_SHIFT_SHIFT;
		break;
	case WIN_KEY_CTRL:
		d->key_shift |= WIN_SHIFT_CTRL;
		break;
	case WIN_KEY_ALT:
		d->key_shift |= WIN_SHIFT_ALT;
		break;
	default:
		;
	}
	
	if ((d->key_shift & SAK_SHIFT) == SAK_SHIFT && (ch == WIN_KEY_DEL || ch == '\n'))
	{
		win_sak(d);
		return 0;
	}
	
	if (d->taskbar)
	{
		if ((d->key_shift & WIN_SHIFT_ALT) && ch == '\t')
		{
			memset(&e, 0, sizeof e);
			e.type	    = E_WINGUI;
			e.proc	    = d->taskbar_proc;
			e.win.type  = WIN_E_KEY_DOWN;
			e.win.wd    = -1;
			e.win.ch    = ch;
			e.win.shift = d->key_shift;
			return send_event(d->taskbar, &e);
		}
		if ((d->key_shift & WIN_SHIFT_CTRL) && ch == 0x1b)
		{
			memset(&e, 0, sizeof e);
			e.type	    = E_WINGUI;
			e.proc	    = d->taskbar_proc;
			e.win.type  = WIN_E_KEY_DOWN;
			e.win.wd    = -1;
			e.win.ch    = ch;
			e.win.shift = d->key_shift;
			return send_event(d->taskbar, &e);
		}
	}
	
	if (d->focus < 0)
		return 0;
	
	memset(&e, 0, sizeof e);
	e.win.wd    = d->focus;
	e.win.type  = WIN_E_KEY_DOWN;
	e.win.ch    = ch;
	e.win.shift = d->key_shift;
	return win_event(d, &e);
#undef SAK_SHIFT
}

int win_keyup(struct win_desktop *d, unsigned ch)
{
	struct event e;
	
	switch (ch)
	{
	case WIN_KEY_SHIFT:
		d->key_shift &= ~WIN_SHIFT_SHIFT;
		break;
	case WIN_KEY_CTRL:
		d->key_shift &= ~WIN_SHIFT_CTRL;
		break;
	case WIN_KEY_ALT:
		d->key_shift &= ~WIN_SHIFT_ALT;
		break;
	default:
		;
	}
	
	if (d->focus < 0)
		return 0;
	
	memset(&e, 0, sizeof e);
	e.win.wd = d->focus;
	e.win.type = WIN_E_KEY_UP;
	e.win.ch = ch;
	e.win.shift = d->key_shift;
	return win_event(d, &e);
}

void win_xywd(struct win_desktop *d, int *wd, int x, int y)
{
	struct win_window *w;
	
	for (w = list_first(&d->stack); w; w = list_next(&d->stack, w))
	{
		if (!w->visible)
			continue;
		
		if (x < w->rect.x)
			continue;
		if (y < w->rect.y)
			continue;
		if (x >= w->rect.x + w->rect.w)
			continue;
		if (y >= w->rect.y + w->rect.h)
			continue;
		
		*wd = w - d->window;
		return;
	}
	
	*wd = -1;
	return;
}

static void win_adjust_ptr(struct win_desktop *d)
{
	if (d->ptr_x < 0)
		d->ptr_x = 0;
	
	if (d->ptr_y < 0)
		d->ptr_y = 0;
	
	if (d->ptr_x >= d->display->width)
		d->ptr_x = d->display->width - 1;
	
	if (d->ptr_y >= d->display->height)
		d->ptr_y = d->display->height - 1;
}

int win_ptrmove(struct win_desktop *d, int x, int y)
{
	struct win_pointer *p = d->currptr;
	struct event e;
	int wd;
	int s;
	
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	if (x >= d->display->width)
		x = d->display->width - 1;
	if (y >= d->display->height)
		y = d->display->height - 1;
	
	s = intr_dis();
	
	wd = d->ptr_down_wd;
	
	d->ptr_x = x;
	d->ptr_y = y;
	win_adjust_ptr(d);
	
	win_lock1(d);
	intr_res(s);
	
	if (!d->ptr_down)
		win_xywd(d, &wd, x, y);
	
	if (d->display_locked == 1)
	{
		d->display->moveptr(d->display->data, x - p->hx, y - p->hy);
		win_update_ptr_ulk(d);
	}
	else
		d->ptr_moved = 1;
	
	win_unlock1(d);
	
	if (wd < 0)
		return 0;
	
	memset(&e, 0, sizeof e);
	e.win.wd    = wd;
	e.win.type  = WIN_E_PTR_MOVE;
	e.win.ptr_x = x - d->window[e.win.wd].rect.x;
	e.win.ptr_y = y - d->window[e.win.wd].rect.y;
	e.win.abs_x = x;
	e.win.abs_y = y;
	return win_event(d, &e);
}

#define abs(x)	((x) < 0 ? -(x) : (x))

int win_ptrmove_rel(struct win_desktop *d, int x, int y)
{
	if (abs(x) > d->ptr_speed.acc_threshold ||
	    abs(y) > d->ptr_speed.acc_threshold)
	{
		x *= d->ptr_speed.acc_mul;
		x /= d->ptr_speed.acc_div;
		y *= d->ptr_speed.acc_mul;
		y /= d->ptr_speed.acc_div;
	}
	else
	{
		x *= d->ptr_speed.nor_mul;
		x /= d->ptr_speed.nor_div;
		y *= d->ptr_speed.nor_mul;
		y /= d->ptr_speed.nor_div;
	}
	return win_ptrmove(d, d->ptr_x + x, d->ptr_y + y);
}

int win_ptrdown(struct win_desktop *d, int button)
{
	struct event e;
	int x, y;
	int s;
	
	memset(&e, 0, sizeof e);
	if ((d->key_shift & WIN_SHIFT_ALT) && !d->ptr_down)
		d->ptr_force_rmb = 1;
	if (d->ptr_force_rmb)
		button = 1;
	
	s = intr_dis();
	x = d->ptr_x;
	y = d->ptr_y;
	if (!d->ptr_down)
		win_xywd(d, &d->ptr_down_wd, x, y);
	d->ptr_down++;
	intr_res(s);
	
	if (d->ptr_down_wd < 0)
		return 0;
	
	e.win.wd = d->ptr_down_wd;
	e.win.type = WIN_E_PTR_DOWN;
	e.win.ptr_x = x - d->window[e.win.wd].rect.x;
	e.win.ptr_y = y - d->window[e.win.wd].rect.y;
	e.win.abs_x = x;
	e.win.abs_y = y;
	if (button >= 0 && button < WIN_BTN_MAX)
		e.win.ptr_button = d->buttons[button];
	else
		e.win.ptr_button = button;
	return win_event(d, &e);
}

int win_ptrup(struct win_desktop *d, int button)
{
	struct event e;
	int x, y;
	int s;
	
	if (d->ptr_force_rmb)
		button = 1;
	
	if (!d->ptr_down)
		panic("win_ptrup: !d->ptr_down");
	d->ptr_down--;
	
	if (!d->ptr_down)
		d->ptr_force_rmb = 0;
	
	s = intr_dis();
	x = d->ptr_x;
	y = d->ptr_y;
	intr_res(s);
	
	if (!d->ptr_down && d->dragdrop)
	{
		int wd;
		
		d->dragdrop = 0;
		
		win_xywd(d, &wd, x, y);
		if (wd >= 0)
		{
			memset(&e, 0, sizeof e);
			e.win.wd	= wd;
			e.win.type	= WIN_E_DROP;
			e.win.ptr_x	= x - d->window[e.win.wd].rect.x;
			e.win.ptr_y	= y - d->window[e.win.wd].rect.y;
			e.win.abs_x	= x;
			e.win.abs_y	= y;
			e.win.dd_serial	= d->dd_serial;
			
			win_event(d, &e);
		}
		
		if (d->ptr_down_wd >= 0)
		{
			memset(&e, 0, sizeof e);
			e.win.wd	= d->ptr_down_wd;
			e.win.type	= WIN_E_DRAG;
			e.win.ptr_x	= x - d->window[e.win.wd].rect.x;
			e.win.ptr_y	= y - d->window[e.win.wd].rect.y;
			e.win.abs_x	= x;
			e.win.abs_y	= y;
			e.win.dd_serial	= d->dd_serial;
			
			win_event(d, &e);
		}
	}
	
	if (d->ptr_down_wd < 0)
		return 0;
	
	memset(&e, 0, sizeof e);
	e.win.wd	= d->ptr_down_wd;
	e.win.type	= WIN_E_PTR_UP;
	e.win.ptr_x	= x - d->window[e.win.wd].rect.x;
	e.win.ptr_y	= y - d->window[e.win.wd].rect.y;
	e.win.abs_x	= x;
	e.win.abs_y	= y;
	if (button >= 0 && button < WIN_BTN_MAX)
		e.win.ptr_button = d->buttons[button];
	else
		e.win.ptr_button = button;
	return win_event(d, &e);
}

void win_bcast(struct win_desktop *d, struct event *e)
{
	int i;
	
	for (i = 0; i < TASK_MAX; i++)
		if (task[i] && task[i]->win_task.desktop == d)
			send_event(task[i], e);
}

int win_update(void)
{
	struct win_desktop *d = curr->win_task.desktop;
	struct event e;
	time_t t;
	
	if (!d)
		return ENODESKTOP;
	
	t = clock_time();
	if (d->last_update == t)
		return EAGAIN;
	d->last_update = t;
	
	memset(&e, 0, sizeof e);
	e.win.type = WIN_E_UPDATE;
	e.type	   = E_WINGUI;
	win_bcast(d, &e);
	return 0;
}

int win_sec_unlock(void)
{
	struct win_desktop *d = curr->win_task.desktop;
	struct event e;
	
	if (!d)
		return ENODESKTOP;
		
	d->sec_locked = 0;
	
	memset(&e, 0, sizeof e);
	e.win.type = WIN_E_SEC_UNLOCK;
	e.type	   = E_WINGUI;
	win_bcast(d, &e);
	
	return 0;
}

int win_soft_keydown(unsigned ch)
{
	struct win_desktop *d = curr->win_task.desktop;
	
	if (!d)
		return ENODESKTOP;
	return win_keydown(d, ch);
}

int win_soft_keyup(unsigned ch)
{
	struct win_desktop *d = curr->win_task.desktop;
	
	if (!d)
		return ENODESKTOP;
	return win_keyup(d, ch);
}

int win_blink(int wd, int ena)
{
	struct win_desktop *d = curr->win_task.desktop;
	int err;
	
	if (!d)
		return ENODESKTOP;
	err = win_chkwd(wd);
	if (err)
		return err;
	d->window[wd].blink_ena = ena;
	return 0;
}

void win_clock(void)
{
	struct win_desktop *d;
	struct event e;
	static int i;
	
	i++;
	i %= clock_hz() / 3;
	if (i)
		return;
	
	memset(&e, 0, sizeof e);
	e.win.type = WIN_E_BLINK;
	for (d = list_first(&win_desktops); d; d = list_next(&win_desktops, d))
		if (d->focus >= 0 && d->window[d->focus].blink_ena)
		{
			e.win.wd = d->focus;
			win_event(d, &e);
		}
}

int win_save_all(void)
{
	struct win_desktop *d = curr->win_task.desktop;
	struct event e;
	time_t t;
	
	if (!d)
		return ENODESKTOP;
	
	t = clock_time();
	if (d->last_update == t)
		return EAGAIN;
	d->last_update = t;
	
	memset(&e, 0, sizeof e);
	e.win.type = WIN_E_SAVE;
	e.type	   = E_WINGUI;
	win_bcast(d, &e);
	return 0;
}
