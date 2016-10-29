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

#include <kern/console.h>
#include <kern/wingui.h>
#include <kern/event.h>
#include <kern/intr.h>
#include <kern/task.h>
#include <kern/lib.h>
#include <errno.h>
#include <event.h>

#include <kern/printk.h>

int win_redraw(int wd)
{
	struct win_desktop *d = curr->win_task.desktop;
	struct win_window *wp = &d->window[wd];
	struct event e;
	
	if (!wp->visible)
		return 0;
	
	memset(&e, 0, sizeof e);
	e.win.wd	= wd;
	e.win.type	= WIN_E_REDRAW;
	e.win.redraw_x	= 0;
	e.win.redraw_y	= 0;
	e.win.redraw_w	= d->window[wd].rect.w;
	e.win.redraw_h	= d->window[wd].rect.h;
	
	return win_event(d, &e);
}

static int _win_redraw_rect(int x, int y, int w, int h, struct win_window *start)
{
	struct win_desktop *d = curr->win_task.desktop;
	struct win_window *wp;
	struct event e;
	
	if (!d)
		panic("win_redraw_rect: !curr->win_task.desktop");
	
	memset(&e, 0, sizeof e);
	e.win.type = WIN_E_REDRAW;
	
	for (wp = start; wp; wp = list_next(&d->stack, wp))
	{
		if (!wp->task)
			continue;
		
		if (!wp->visible)
			continue;
		
		if (wp->rect.x >= x + w)
			continue;
		if (wp->rect.y >= y + h)
			continue;
		if (wp->rect.x + wp->rect.w <= x)
			continue;
		if (wp->rect.y + wp->rect.h <= y)
			continue;
		
		e.win.wd       = wp - d->window;
		e.win.redraw_x = x - wp->rect.x;
		e.win.redraw_y = y - wp->rect.y;
		e.win.redraw_w = w;
		e.win.redraw_h = h;
		
		if (e.win.redraw_x < 0)
		{
			e.win.redraw_w += e.win.redraw_x;
			e.win.redraw_x = 0;
		}
		if (e.win.redraw_y < 0)
		{
			e.win.redraw_h += e.win.redraw_y;
			e.win.redraw_y = 0;
		}
		
		if (e.win.redraw_x + e.win.redraw_w > wp->rect.w)
			e.win.redraw_w = wp->rect.w - e.win.redraw_x;
		
		if (e.win.redraw_y + e.win.redraw_h > wp->rect.h)
			e.win.redraw_h = wp->rect.h - e.win.redraw_y;
		
		win_event(d, &e);
	}
	return 0;
}

int win_advise_pos(int *x, int *y, int w, int h)
{
	struct win_desktop *d = curr->win_task.desktop;
	int x0, y0, x1, y1;
	
	if (!d)
		return ENODESKTOP;
	
	if (d->taskbar)
	{
		x0 = d->ws_rect.x;
		y0 = d->ws_rect.y;
		x1 = d->ws_rect.x + d->ws_rect.w;
		y1 = d->ws_rect.y + d->ws_rect.h;
	}
	else
	{
		x1 = d->display->width;
		y1 = d->display->height;
		x0 = y0 = 0;
	}
	
	if (d->focus >= 0 && d->window[d->focus].task == curr)
	{
		*x = d->window[d->focus].rect.x + 20;
		*y = d->window[d->focus].rect.y + 20;
		
		if (*x + w <= x1 && *y + h <= y1)
			return 0;
	}
	
	*x = d->last_auto_x + 20;
	*y = d->last_auto_y + 20;
	if ((*x + w > x1 && w < x1 - x0) || *x > x1)
		*x = x0;
	if ((*y + h > y1 && h < y1 - y0) || *y > y1)
		*y = y0;
	
	d->last_auto_x = *x;
	d->last_auto_y = *y;
	return 0;
}

int win_creat(int *wd, int visible, int x, int y, int w, int h, void *proc, void *data)
{
	struct win_desktop *d = curr->win_task.desktop;
	struct win_window *wp = NULL;
	int err;
	int i;
	int s;
	
	if (!d)
		return ENODESKTOP;
	
	if (d->window_count >= WIN_MAX)
		return ENOMEM;
	
	for (i = 0; i < WIN_MAX; i++)
	{
		wp = &d->window[i];
		
		if (wp->task)
			continue;
		
		memset(wp, 0, sizeof *wp);
		wp->pointer = &d->pointers[WIN_PTR_DEFAULT];
		wp->task = curr;
		wp->proc = proc;
		wp->data = data;
		wp->visible = 0;
		if (!curr->euid && d->sec_locked)
			wp->layer = WIN_LAYER_SECURE;
		else
			wp->layer = WIN_LAYER_APP;
		wp->font = d->font_map[DEFAULT_FTD];
		
		s = intr_dis();
		list_app(&d->stack, wp);
		intr_res(s);
		
		if (d->taskbar)
		{
			struct event e;
			
			memset(&e, 0, sizeof e);
			e.win.type = WIN_E_CREAT;
			e.win.wd = i;
			e.type = E_WINGUI;
			e.proc = d->taskbar_proc;
			err = send_event(d->taskbar, &e);
			if (err)
				goto fail;
		}
		err = win_change(i, visible, x, y, w, h);
		if (err)
			goto fail;
		*wd = i;
		d->window_count++;
		return 0;
	}
	
	panic("win_creat: inconsistent window list");
fail:
	if (wp)
	{
		s = intr_dis();
		list_rm(&d->stack, wp);
		intr_res(s);
		memset(wp, 0, sizeof *wp);
	}
	return err;
}

int win_change(int wd, int visible, int x, int y, int w, int h)
{
	struct win_desktop *d = curr->win_task.desktop;
	struct win_window *wp1;
	struct win_window *wp;
	int full_redraw = 0;
	int err;
	int s;
	
	err = win_chkwd(wd);
	if (err)
		return err;
	
	if (w < 0 || h < 0)
	{
		w = 0;
		h = 0;
	}
	
	wp = &d->window[wd];
	
	if (d->taskbar && wp->visible != visible)
	{
		struct event e;
		
		memset(&e, 0, sizeof e);
		e.win.type = WIN_E_CHANGE;
		e.win.wd   = wd;
		e.type	   = E_WINGUI;
		e.proc	   = d->taskbar_proc;
		send_event(d->taskbar, &e);
	}
	
	if (wp->visible && !visible)
	{
		wp->visible = 0; /* no need to update wp->rect */
		_win_redraw_rect(wp->rect.x, wp->rect.y, wp->rect.w, wp->rect.h, list_next(&d->stack, wp));
		win_update_ptr(d);
		return 0;
	}
	
	if (!wp->visible && visible)
	{
		wp->visible = 1;
		wp->rect.x = x;
		wp->rect.y = y;
		wp->rect.w = w;
		wp->rect.h = h;
		
		wp->x0 = 0;
		wp->y0 = 0;
		wp->clip.x = 0;
		wp->clip.y = 0;
		wp->clip.w = w;
		wp->clip.h = h;
		
		win_update_ptr(d);
		return win_redraw(wd);
	}
	
	if (!visible) /* no need to update wp->rect */
		return 0;
	
	if (wp->rect.x != x || wp->rect.y != y ||
	    wp->rect.w != w || wp->rect.h != h)
	{
		_win_redraw_rect(wp->rect.x, wp->rect.y, wp->rect.w, wp->rect.h, list_next(&d->stack, wp));
		wp->x0 = 0;
		wp->y0 = 0;
		wp->clip.x = 0;
		wp->clip.y = 0;
		wp->clip.w = w;
		wp->clip.h = h;
		
		for (wp1 = list_prev(&d->stack, wp); wp1; wp1 = list_prev(&d->stack, wp1))
		{
			if (!wp1->visible)
				continue;
			if (wp1->rect.x + wp1->rect.w <= wp->rect.x)
				continue;
			if (wp1->rect.y + wp1->rect.h <= wp->rect.y)
				continue;
			if (wp1->rect.x >= wp->rect.x + wp->rect.w)
				continue;
			if (wp1->rect.y >= wp->rect.y + wp->rect.h)
				continue;
			full_redraw = 1;
			break;
		}
		
		for (wp1 = list_prev(&d->stack, wp); wp1; wp1 = list_prev(&d->stack, wp1))
		{
			if (!wp1->visible)
				continue;
			if (wp1->rect.x + wp1->rect.w <= x)
				continue;
			if (wp1->rect.y + wp1->rect.h <= y)
				continue;
			if (wp1->rect.x >= x + w)
				continue;
			if (wp1->rect.y >= y + h)
				continue;
			full_redraw = 1;
			break;
		}
		
		if (x + w > d->display->width ||
		    y + h > d->display->height ||
		    x < 0 ||
		    y < 0 ||
		    w < 0 ||
		    h < 0 ||
		    w != wp->rect.w ||
		    h != wp->rect.h ||
		    wp->rect.x < 0 ||
		    wp->rect.y < 0 ||
		    wp->rect.x + wp->rect.w > d->display->width ||
		    wp->rect.y + wp->rect.h > d->display->height)
			full_redraw = 1;
		
		if (d->display->flags & WIN_DF_SLOWCOPY)
			full_redraw = 1;
		
		if (full_redraw)
		{
			s = intr_dis();
			wp->rect.x = x;
			wp->rect.y = y;
			wp->rect.w = w;
			wp->rect.h = h;
			intr_res(s);
			
			win_update_ptr(d);
			return win_redraw(wd);
		}
		
		win_paint();
		win_lock();
		d->display->copy(d->display->data,
				 x, y, wp->rect.x, wp->rect.y, w, h);
		win_unlock();
		win_end_paint();
		
		s = intr_dis();
		wp->rect.x = x;
		wp->rect.y = y;
		intr_res(s);
		
		win_update_ptr(d);
		return 0;
	}
	
	return 0;
}

int win_raise(int wd)
{
	struct win_desktop *d = curr->win_task.desktop;
	struct win_window *wpp;
	struct win_window *wp1;
	struct win_window *wp0;
	struct event e;
	int x0, y0;
	int x1, y1;
	int rs = 0;
	int s;
	
	if (!d)
		return ENODESKTOP;
	
	if (wd < 0 || wd >= WIN_MAX || !d->window[wd].task)
		return EINVAL;
	
	wp0 = &d->window[wd];
	
	wpp = list_first(&d->stack);
	while (wpp->layer < wp0->layer)
		wpp = list_next(&d->stack, wpp);
	if (wpp == wp0)
		return 0;
	
	memset(&e, 0, sizeof e);
	e.win.wd   = wd;
	e.win.type = WIN_E_RAISE;
	win_event(d, &e);
	
	if (wp0->visible)
	{
		for (wp1 = list_prev(&d->stack, wp0); wp1; wp1 = list_prev(&d->stack, wp1))
		{
			if (!wp1->visible)
				continue;
			if (wp1->rect.x + wp1->rect.w <= wp0->rect.x)
				continue;
			if (wp1->rect.y + wp1->rect.h <= wp0->rect.y)
				continue;
			if (wp1->rect.x >= wp0->rect.x + wp0->rect.w)
				continue;
			if (wp1->rect.y >= wp0->rect.y + wp0->rect.h)
				continue;
			if (!rs || wp1->rect.x < x0)
				x0 = wp1->rect.x;
			if (!rs || wp1->rect.y < y0)
				y0 = wp1->rect.y;
			if (!rs || wp1->rect.x + wp1->rect.w > x1)
				x1 = wp1->rect.x + wp1->rect.w;
			if (!rs || wp1->rect.y + wp1->rect.h > y1)
				y1 = wp1->rect.y + wp1->rect.h;
			rs = 1;
		}
		x0 -= wp0->rect.x;
		x1 -= wp0->rect.x;
		y0 -= wp0->rect.y;
		y1 -= wp0->rect.y;
		if (x0 < 0)
			x0 = 0;
		if (y0 < 0)
			y0 = 0;
		if (x1 > wp0->rect.w)
			x1 = wp0->rect.w;
		if (y1 > wp0->rect.h)
			y1 = wp0->rect.h;
	}
	
	s = intr_dis();
	list_rm(&d->stack, wp0);
	list_ib(&d->stack, wpp, wp0);
	intr_res(s);
	
	if (!wp0->visible)
		return 0;
	
	if (rs)
	{
		memset(&e, 0, sizeof e);
		e.win.wd	= wd;
		e.win.type	= WIN_E_REDRAW;
		e.win.redraw_x	= x0;
		e.win.redraw_y	= y0;
		e.win.redraw_w	= x1 - x0;
		e.win.redraw_h	= y1 - y0;
		return win_event(d, &e);
	}
	return 0;
}

int win_focus(int wd)
{
	struct win_desktop *d = curr->win_task.desktop;
	struct event e;
	int prev;
	
	if (!d)
		return ENODESKTOP;
	if (d->sec_locked && curr->euid)
		return EPERM;
	
	if (wd < 0 || wd >= WIN_MAX || !d->window[wd].task)
		return EINVAL;
	
	prev = d->focus;
	if (prev == wd)
		return 0;
	
	memset(&e, 0, sizeof e);
	if (prev >= 0)
	{
		e.win.wd   = prev;
		e.win.type = WIN_E_SWITCH_FROM;
		win_event(d, &e);
	}
	e.win.wd   = wd;
	e.win.type = WIN_E_SWITCH_TO;
	win_event(d, &e);
	d->focus = wd;
	
	if (curr->win_task.desktop->taskbar)
	{
		struct event e;
		
		memset(&e, 0, sizeof e);
		e.win.type = WIN_E_SWITCH_TO;
		e.win.wd   = wd;
		e.type     = E_WINGUI;
		e.proc     = d->taskbar_proc;
		send_event(d->taskbar, &e);
	}
	return 0;
}

int win_ufocus(int wd)
{
	struct win_desktop *d = curr->win_task.desktop;
	struct event e;
	int err;
	
	err = win_chkwd(wd);
	if (err)
		return err;
	
	if (!d)
		return ENODESKTOP;
	
	memset(&e, 0, sizeof e);
	e.win.wd   = wd;
	e.win.type = WIN_E_SWITCH_FROM;
	win_event(d, &e);
	d->focus = -1;
	
	if (curr->win_task.desktop->taskbar)
	{
		struct event e;
		
		memset(&e, 0, sizeof e);
		e.win.type = WIN_E_SWITCH_TO;
		e.win.wd   = -1;
		e.type	   = E_WINGUI;
		e.proc	   = d->taskbar_proc;
		send_event(d->taskbar, &e);
	}
	return 0;
}

int win_close(int wd)
{
	struct win_desktop *d = curr->win_task.desktop;
	struct win_window *nw;
	struct win_window *w;
	struct win_rect rect;
	struct win_task *wt;
	int visible;
	int err;
	int n;
	int s;
	
	err = win_chkwd(wd);
	if (err)
		return err;
	
	w  = &d->window[wd];
	wt = &w->task->win_task;
	nw = w;
	do
		nw = list_next(&d->stack, nw);
	while (nw != NULL && !nw->visible);
	
	visible = w->visible;
	rect	= w->rect;
	
	if (d->focus == wd)
	{
		d->focus = -1;
		if (nw)
			win_focus(nw - d->window);
	}
	
	if (d->ptr_down_wd == wd)
		d->ptr_down_wd = -1;
	
	s = intr_dis();
	list_rm(&d->stack, w);
	d->window_count--;
	
	for (n = 0; n < EVT_MAX; n++)
		if (curr->event[n].type == E_WINGUI && curr->event[n].win.wd == wd)
			curr->event[n].type = 0;
	memset(w, 0, sizeof *w);
	intr_res(s);
	
	if (visible)
		_win_redraw_rect(rect.x, rect.y, rect.w, rect.h, nw);
	
	if (d->taskbar)
	{
		struct event e;
		
		memset(&e, 0, sizeof e);
		e.win.type = WIN_E_CLOSE;
		e.win.wd   = wd;
		e.type	   = E_WINGUI;
		e.proc	   = d->taskbar_proc;
		send_event(d->taskbar, &e);
	}
	
	win_update_ptr(d);
	return 0;
}

int win_is_visible(int wd, int *visible)
{
	if (!curr->win_task.desktop)
		return ENODESKTOP;
	
	if (wd < 0 || wd >= WIN_MAX || !curr->win_task.desktop->window[wd].task)
		return EINVAL;
	
	*visible = curr->win_task.desktop->window[wd].visible;
	return 0;
}

int win_redraw_all(void)
{
	int i;
	
	if (!curr->win_task.desktop)
		return ENODESKTOP;
	
	for (i = 0; i < WIN_MAX; i++)
		if (curr->win_task.desktop->window[i].task)
			win_redraw(i);
	
	return 0;
}

int win_setlayer(int wd, int layer)
{
	int err;
	
	err = win_chkwd(wd);
	if (err)
		return err;
	if (curr->euid && layer < 0)
		return EPERM;
	curr->win_task.desktop->window[wd].layer = layer;
	return 0;
}

int win_setptr(int wd, int ptr)
{
	struct win_pointer *p;
	struct win_desktop *d;
	int err;
	
	err = win_chkwd(wd);
	if (err)
		return err;
	if (ptr < 0 || ptr >= WIN_PTR_MAX)
		return EINVAL;
	d = curr->win_task.desktop;
	p = &d->pointers[ptr];
	d->window[wd].pointer = p;
	win_update_ptr(d);
	return 0;
}

int win_deflayer(int *layer)
{
	struct win_desktop *d;
	
	d = curr->win_task.desktop;
	if (!d)
		return ENODESKTOP;
	
	if (!curr->euid && d->sec_locked)
		*layer = WIN_LAYER_SECURE;
	else
		*layer = WIN_LAYER_APP;
	return 0;
}
