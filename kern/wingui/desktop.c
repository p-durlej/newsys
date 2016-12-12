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
#include <kern/printk.h>
#include <kern/module.h>
#include <kern/wingui.h>
#include <kern/signal.h>
#include <kern/event.h>
#include <kern/task.h>
#include <kern/lib.h>
#include <signal.h>
#include <errno.h>
#include <event.h>
#include <list.h>

static void _win_defptr(struct win_display *disp, win_color *pixels, unsigned *mask);
static void win_reset_ptrs(struct win_desktop *d);

static char ptr_shape[PTR_WIDTH * PTR_HEIGHT] =
"*                                                               "
"**                                                              "
"*X*                                                             "
"*XX*                                                            "
"*XXX*                                                           "
"*XXXX*                                                          "
"*XXXXX*                                                         "
"*XXXXXX*                                                        "
"*XXXXXXX*                                                       "
"*XXXX*****                                                      "
"*XX*XX*                                                         "
"*X**XX*                                                         "
"** *XXX*                                                        "
"    *XX*                                                        "
"    *XX*                                                        "
"     **                                                         "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
"                                                                "
;

struct list win_desktops;

static void _win_defptr(struct win_display *disp, win_color *pixels, unsigned *mask)
{
	win_color white;
	win_color gray;
	win_color black;
	int i;
	
	disp->rgba2color(disp->data, 255, 255, 255, 255, &white);
	disp->rgba2color(disp->data, 127, 127, 127, 255, &gray);
	disp->rgba2color(disp->data, 0,   0,   0,   255, &black);
	
	for (i = 0; i < PTR_WIDTH * PTR_HEIGHT; i++)
		switch (ptr_shape[i])
		{
		case 'X':
			pixels[i] = white;
			mask[i]	  = 1;
			break;
		case '.':
			pixels[i] = gray;
			mask[i]	  = 1;
			break;
		case '*':
			pixels[i] = black;
			mask[i]	  = 1;
			break;
		default:
			pixels[i] = black;
			mask[i]	  = 0;
		}
}

int win_newdesktop(const char *name)
{
	struct win_desktop *d;
	int err;
	int i;
	
	if (strlen(name) >= sizeof d->name)
		return ENAMETOOLONG;
	
	if (curr->win_task.desktop)
		win_detach();
	
	err = kmalloc(&d, sizeof(struct win_desktop), "desktop");
	if (err)
		return err;
	
	memset(d, 0, sizeof(struct win_desktop));
	strcpy(d->name, name);
	d->refcnt = 1;
	d->focus = -1;
	
	d->display = &null_display;
	for (i = 0; i < sizeof d->input_md / sizeof *d->input_md; i++)
		d->input_md[i] = -1;
	d->currptr = &d->pointers[WIN_PTR_DEFAULT];
	list_app(&win_desktops, d);
	
	curr->win_task.inh_desktop = d;
	curr->win_task.desktop	   = d;
	
	for (i = 0; i < FONT_MAX; i++)
		d->font_map[i] = i;
	
	win_reset();
	
	return 0;
}

int win_killdesktop(sig_t signal)
{
	struct win_desktop *d = curr->win_task.desktop;
	int i;
	
	if (!d)
		return ENODESKTOP;
	
	for (i = 0; i < TASK_MAX; i++)
	{
		if (!task[i])
			continue;
		
		if (task[i] != curr && task[i]->win_task.desktop == d)
			signal_send(task[i], signal);
	}
	
	return 0;
}

int win_detach(void)
{
	struct win_desktop *d = curr->win_task.inh_desktop;
	int i;
	
	if (!d)
		return ENODESKTOP;
	
	win_clean();
	
	curr->win_task.inh_desktop = NULL;
	curr->win_task.desktop     = NULL;
	d->refcnt--;
	if (!d->refcnt)
	{
		if (d->display != &null_display)
			mod_unload(d->display_md);
		for (i = 0; i < sizeof d->input_md / sizeof *d->input_md; i++)
			if (d->input_md[i] >= 0)
				mod_unload(d->input_md[i]);
		list_rm(&win_desktops, d);
	}
	return 0;
}

int win_owner(uid_t uid)
{
	struct win_desktop *d = curr->win_task.inh_desktop;
	
	if (!d)
		return ENODESKTOP;
	
	d->owner_uid = uid;
	return 0;
}

int win_get_ptr_pos(int *x, int *y)
{
	struct win_desktop *d;
	int s;
	
	d = curr->win_task.desktop;
	if (!d)
		return ENODESKTOP;
	
	s = intr_dis();
	*x = d->ptr_x;
	*y = d->ptr_y;
	intr_res(s);
	return 0;
}

int win_desktop_size(int *width, int *height)
{
	struct win_desktop *d = curr->win_task.desktop;
	
	if (!d)
		return ENODESKTOP;
	
	*width = d->display->width;
	*height = d->display->height;
	return 0;
}

int win_ws_getrect(struct win_rect *r)
{
	struct win_desktop *d = curr->win_task.desktop;
	
	if (!d)
		return ENODESKTOP;
	if (!d->taskbar)
	{
		memset(r, 0, sizeof *r);
		r->x = 0;
		r->y = 0;
		r->w = d->display->width;
		r->h = d->display->height;
		return 0;
	}
	*r = d->ws_rect;
	return 0;
}

int win_ws_setrect(const struct win_rect *r)
{
	struct win_desktop *d = curr->win_task.desktop;
	
	if (!d)
		return ENODESKTOP;
	if (d->taskbar && d->taskbar != curr)
		return EPERM;
	d->ws_rect = *r;
	
	win_broadcast_resize(d);
	return 0;
}

int win_display(int md, struct win_display *display)
{
	struct win_desktop *d = curr->win_task.desktop;
	
	if (!d)
		return ENODESKTOP;
	if (d->display != &null_display)
		return EEXIST;
	
	con_disable();
	
	d->ptr_x   = display->width  / 2;
	d->ptr_y   = display->height / 2;
	d->display = display;
	
	win_lock();
	win_reset_ptrs(d);
	// win_defptr(display);
	display->moveptr(d->display->data, d->ptr_x, d->ptr_y);
	win_unlock();
	
	win_broadcast_setmode(d);
	return 0;
}

int win_input(int md)
{
	struct win_desktop *d = curr->win_task.desktop;
	int i;
	
	for (i = 0; i < sizeof d->input_md / sizeof *d->input_md; i++)
		if (d->input_md[i] < 0)
		{
			d->input_md[i] = md;
			return 0;
		}
	printk("win_input: too many input devices\n");
	return ENOMEM;
}

void win_uninstall_display(struct win_desktop *d, struct win_display *disp)
{
	if (!d)
		panic("win_uninstall_display: !d");
	if (d->display != disp)
		panic("win_uninstall_display: d->display != disp");
	d->display = &null_display;
}

void win_uninstall_input(struct win_desktop *d, int md)
{
	int i;
	
	if (!d)
		panic("win_uninstall_input: !d");
	
	for (i = 0; i < sizeof d->input_md / sizeof *d->input_md; i++)
		if (d->input_md[i] == md)
		{
			d->input_md[i] = -1;
			return;
		}
	
	panic("win_uninstall_input: module not found in list");
}

void win_update_ptr(struct win_desktop *d)
{
	struct win_pointer *p;
	int wd;
	int s;
	
	s = intr_dis();
	if (d->ptr_down)
		wd = d->ptr_down_wd;
	else
		win_xywd(d, &wd, d->ptr_x, d->ptr_y);
	if (wd >= 0)
		p = d->window[wd].pointer;
	else
		p = &d->pointers[WIN_PTR_DEFAULT];
	if (p != d->currptr)
	{
		d->display->moveptr(d->display->data, d->ptr_x - p->hx, d->ptr_y - p->hy);
		d->display->setptr(d->display->data, p->pixels, p->mask);
		d->currptr = p;
	}
	intr_res(s);
}

int win_modeinfo(int mode, struct win_modeinfo *buf)
{
	struct win_desktop *d = curr->win_task.desktop;
	int err;
	
	if (!d)
		return ENODESKTOP;
	
	memset(buf, 0, sizeof *buf);
	if (mode < 0)
		mode = d->display->mode;
	
	win_lock();
	err = d->display->modeinfo(d->display->data, mode, buf);
	win_unlock();
	return err;
}

int win_setmode(int mode, int refresh)
{
	struct win_desktop *d = curr->win_task.desktop;
	int err;
	
	if (!d)
		return ENODESKTOP;
	
	win_broadcast_setmode(d);
	
	win_lock();
	err = d->display->setmode(d->display->data, mode, refresh);
	win_reset_ptrs(d);
	win_unlock();
	if (err)
		return err;
	return 0;
}

int win_getmode(int *mode)
{
	struct win_desktop *d = curr->win_task.desktop;
	
	if (!d)
		return ENODESKTOP;
	
	*mode = d->display->mode;
	return 0;
}

int win_insert_ptr(int ptr, const win_color *pixels, const unsigned *mask, int w, int h, int hx, int hy)
{
	static win_color pbuf[PTR_WIDTH * PTR_HEIGHT];
	static unsigned  mbuf[PTR_WIDTH * PTR_HEIGHT];
	
	struct win_desktop *d = curr->win_task.desktop;
	struct win_pointer *p;
	int x, y;
	
	if (!d)
		return ENODESKTOP;
	
	if (ptr < 0 || ptr >= WIN_PTR_MAX)
		return EINVAL;
	
	if ((unsigned)w > PTR_WIDTH || (unsigned)h > PTR_HEIGHT)
		return EINVAL;
	
	memset(pbuf, 0, sizeof pbuf);
	memset(mbuf, 0, sizeof mbuf);
	
	for (y = 0; y < h; y++)
		for (x = 0; x < w; x++)
		{
			pbuf[x + y * PTR_WIDTH] = pixels[x + y * w];
			mbuf[x + y * PTR_WIDTH] = mask[x + y * w];
		}
	
	p = &d->pointers[ptr];
	memcpy(p->pixels, pbuf, sizeof p->pixels);
	memcpy(p->mask, mbuf, sizeof p->mask);
	p->hx = hx;
	p->hy = hy;
	
	win_lock();
	if (d->currptr == p)
	{
		d->display->moveptr(d->display->data, d->ptr_x - hx, d->ptr_y - hy);
		d->display->setptr(d->display->data, pbuf, mbuf);
	}
	win_unlock();
	return 0;
}

int win_set_ptr_speed(const struct win_ptrspeed *speed)
{
	if (!curr->win_task.desktop)
		return ENODESKTOP;
	
	if (!speed->nor_div || !speed->acc_div)
		return EINVAL;
	
	curr->win_task.desktop->ptr_speed = *speed;
	return 0;
}

int win_get_ptr_speed(struct win_ptrspeed *speed)
{
	if (!curr->win_task.desktop)
		return ENODESKTOP;
	
	*speed = curr->win_task.desktop->ptr_speed;
	return 0;
}

int win_map_buttons(const int *tab, int cnt)
{
	int i;
	
	if (!curr->win_task.desktop)
		return ENODESKTOP;
	
	memcpy(curr->win_task.desktop->buttons, tab, sizeof *tab * cnt);
	for (i = cnt; i < WIN_BTN_MAX; i++)
		curr->win_task.desktop->buttons[i] = i;
	return 0;
}

static void win_reset_ptrs(struct win_desktop *d)
{
	int i;
	
	memset(&d->pointers, 0, sizeof d->pointers);
#if WIN_PTR_MAX > 1
	_win_defptr(d->display, d->pointers[1].pixels, d->pointers[1].mask);
	for (i = 2; i < WIN_PTR_MAX; i++)
		d->pointers[i] = d->pointers[1];
	d->display->setptr(d->display->data, d->currptr->pixels, d->currptr->mask);
	d->display->moveptr(d->display->data, d->ptr_x, d->ptr_y);
#endif
}

int win_reset(void)
{
	struct win_desktop *d = curr->win_task.desktop;
	int i;
	
	if (!d)
		return ENODESKTOP;
	
	for (i = 0; i < sizeof d->buttons / sizeof *d->buttons; i++)
		d->buttons[i] = i;
	
	d->ptr_speed.acc_threshold = 0;
	d->ptr_speed.acc_mul = 1;
	d->ptr_speed.acc_div = 1;
	d->ptr_speed.nor_mul = 1;
	d->ptr_speed.nor_div = 1;
	win_lock();
	win_reset_ptrs(d);
	win_unlock();
	return 0;
}

void win_lock(void)
{
	struct win_desktop *d = curr->win_task.desktop;
	
	if (!d)
		panic("win_lock: !curr->win_task.desktop");
	
	d->display_locked++;
}

void win_unlock(void)
{
	struct win_desktop *d = curr->win_task.desktop;
	int s;
	
	if (!d)
		panic("win_lock: !curr->win_task.desktop");
	
	s = intr_dis();
	d->display_locked--;
	if (d->ptr_moved && !d->display_locked)
	{
		d->ptr_moved = 0;
		d->display->moveptr(d->display->data, d->ptr_x - d->currptr->hx, d->ptr_y - d->currptr->hy);
		win_update_ptr(d);
	}
	intr_res(s);
}

struct win_desktop *win_getdesktop(void)
{
	return curr->win_task.desktop;
}

void win_broadcast_resize(struct win_desktop *d)
{
	struct event e;
	
	memset(&e, 0, sizeof e);
	e.win.type = WIN_E_RESIZE;
	e.win.wd   = -1;
	e.type	   = E_WINGUI;
	win_bcast(d, &e);
}

void win_broadcast_setmode(struct win_desktop *d)
{
	struct event e;
	int i;
	
	win_broadcast_resize(d);
	
	memset(&e, 0, sizeof e);
	e.win.type = WIN_E_SETMODE;
	e.win.wd   = -1;
	e.type	   = E_WINGUI;
	win_bcast(d, &e);
	
	for (i = 0; i < WIN_MAX; i++)
		if (d->window[i].task)
			win_redraw(i);
}

int win_buffer(int ena)
{
	struct win_desktop *d = curr->win_task.desktop;
	
	if (d == NULL)
		return ENODESKTOP;
	
	return ena ? ENODEV : 0;
}
