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

#include <kern/syscall.h>
#include <kern/wingui.h>
#include <kern/event.h>
#include <kern/task.h>
#include <kern/umem.h>
#include <kern/lib.h>
#include <errno.h>

int sys_win_newdesktop(const char *name)
{
	char lname[NAME_MAX + 1];
	int err;
	
	err = fustr(lname, name, sizeof lname);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = win_newdesktop(lname);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_killdesktop(sig_t signal)
{
	int err;
	
	err = win_killdesktop(signal);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_attach(void)
{
	if (!curr->win_task.inh_desktop)
	{
		uerr(ENODESKTOP);
		return -1;
	}
	curr->win_task.desktop = curr->win_task.inh_desktop;
	return 0;
}

int sys_win_detach(void)
{
	int err;
	
	err = win_detach();
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_owner(uid_t uid)
{
	int err;
	
	err = win_owner(uid);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_get_ptr_pos(int *x, int *y)
{
	int lx, ly;
	int err;
	
	err = win_get_ptr_pos(&lx, &ly);
	if (err)
		goto err;
	
	err = tucpy(x, &lx, sizeof lx);
	if (err)
		goto err;
	
	err = tucpy(y, &ly, sizeof ly);
	if (err)
		goto err;
	return 0;
err:
	uerr(err);
	return -1;
}

int sys_win_advise_pos(int *x, int *y, int w, int h)
{
	int lx, ly;
	int err;
	
	err = win_advise_pos(&lx, &ly, w, h);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = tucpy(x, &lx, sizeof lx);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = tucpy(y, &ly, sizeof ly);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys_win_creat(int *wd, int visible, int x, int y, int w, int h, void *proc, void *data)
{
	int err;
	int lwd;
	
	err = win_creat(&lwd, visible, x, y, w, h, proc, data);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = tucpy(wd, &lwd, sizeof lwd);
	if (err)
	{
		win_close(lwd);
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_change(int wd, int visible, int x, int y, int w, int h)
{
	int err;
	
	err = win_change(wd, visible, x, y, w, h);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_raise(int wd)
{
	int err;
	
	err = win_raise(wd);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_focus(int wd)
{
	int err;
	
	err = win_focus(wd);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_ufocus(int wd)
{
	int err;
	
	err = win_ufocus(wd);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_close(int wd)
{
	int err;
	
	err = win_close(wd);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_paint(void)
{
	int err;
	
	err = win_paint();
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_end_paint(void)
{
	int err;
	
	err = win_end_paint();
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_rgb2color(win_color *c, int r, int g, int b)
{
	win_color lc;
	int err;
	
	err = win_rgb2color(&lc, r, g, b);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = tucpy(c, &lc, sizeof lc);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_rgba2color(win_color *c, int r, int g, int b, int a)
{
	win_color lc;
	int err;
	
	err = win_rgba2color(&lc, r, g, b, a);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = tucpy(c, &lc, sizeof lc);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_clip(int wd, int x, int y, int w, int h, int x0, int y0)
{
	int err;
	
	err = win_clip(wd, x, y, w, h, x0, y0);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_pixel(int wd, win_color c, int x, int y)
{
	int err;
	
	err = win_pixel(wd, c, x, y);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_hline(int wd, win_color c, int x, int y, int l)
{
	int err;
	
	err = win_hline(wd, c, x, y, l);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_vline(int wd, win_color c, int x, int y, int l)
{
	int err;
	
	err = win_vline(wd, c, x, y, l);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_rect(int wd, win_color c, int x, int y, int w, int h)
{
	int err;
	
	err = win_rect(wd, c, x, y, w, h);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_copy(int wd, int dx, int dy, int sx, int sy, int w, int h)
{
	int err;
	
	err = win_copy(wd, dx, dy, sx, sy, w, h);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;	
}

int sys_win_text(int wd, win_color c, int x, int y, const char *text)
{
	char ltext[4096];
	int err;
	
	err = fustr(ltext, text, sizeof ltext);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = win_text(wd, c, x, y, ltext);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_btext(int wd, win_color bg, win_color fg, int x, int y, const char *text)
{
	char ltext[4096];
	int err;
	
	err = fustr(ltext, text, sizeof ltext);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = win_btext(wd, bg, fg, x, y, ltext);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_text_size(int wd, int *width, int *height, const char *text)
{
	int err;
	
	if ((err = usa(&text,	65536, ERANGE))			||
	    (err = uga(&width,	sizeof *width, UA_WRITE))	||
	    (err = uga(&height, sizeof *height, UA_WRITE)))
	{
		uerr(err);
		return -1;
	}
	
	err = win_text_size(wd, width, height, text);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_chr(int wd, win_color c, int x, int y, unsigned ch)
{
	int err;
	
	err = win_chr(wd, c, x, y, ch);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_bchr(int wd, win_color bg, win_color fg, int x, int y, unsigned ch)
{
	int err;
	
	err = win_bchr(wd, bg, fg, x, y, ch);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_chr_size(int ftd, int *width, int *height, unsigned ch)
{
	int err;
	
	if ((err = uga(&width,  sizeof *width,  UA_WRITE)) ||
	    (err = uga(&height, sizeof *height, UA_WRITE)))
		goto err;
	
	err = win_chr_size(ftd, width, height, ch);
	if (err)
		goto err;
	
	return 0;
err:
	uerr(err);
	return -1;
}

int sys_win_bitmap(int wd, const win_color *bmp, int x, int y, int w, int h)
{
	unsigned bsize = sizeof(win_color) * w * h;
	int err;
	
	if (!w || !h)
	{
		uerr(EINVAL);
		return -1;
	}
	
	if (bsize / w / h != sizeof(win_color))
	{
		uerr(EINVAL);
		return -1;
	}
	
	err = urchk(bmp, bsize);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	if (fault())
	{
		uerr(EFAULT);
		return -1;
	}
	err = win_bitmap(wd, bmp, x, y, w, h);
	fault_fini();
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_bconv(win_color *c, const struct win_rgba *rgba, unsigned count)
{
	void (*rgba2color)(void *dd, int r, int g, int b, int a, win_color *c);
	struct win_desktop *d = curr->win_task.desktop;
	void *dd = d->display->data;
	int err;
	
	if (!d)
	{
		uerr(ENODESKTOP);
		return -1;
	}
	rgba2color = d->display->rgba2color;
	
	if ((sizeof(win_color) * count) / sizeof(win_color) != count)
	{
		uerr(EFAULT);
		return -1;
	}
	
	if ((sizeof(struct win_rgba) * count) / sizeof(struct win_rgba) != count)
	{
		uerr(EFAULT);
		return -1;
	}
	
	err = uga(&c, sizeof(win_color) * count, UA_WRITE | UA_INIT);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = uga(&rgba, sizeof *rgba * count, UA_READ | UA_INIT);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	while (count--)
	{
		if (!rgba->a)
			*c = d->display->transparent;
		else
			rgba2color(dd, rgba->r, rgba->g, rgba->b, rgba->a, c);
		
		rgba++;
		c++;
	}
	
	return 0;
}

int sys_win_desktop_size(int *width, int *height)
{
	int err;
	int w;
	int h;
	
	err = win_desktop_size(&w, &h);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = tucpy(width, &w, sizeof w);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = tucpy(height, &h, sizeof h);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_ws_getrect(struct win_rect *r)
{
	struct win_rect lr;
	int err;
	
	err = win_ws_getrect(&lr);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = tucpy(r, &lr, sizeof lr);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys_win_ws_setrect(const struct win_rect *r)
{
	int err;
	
	err = uga(&r, sizeof *r, UA_READ);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = win_ws_setrect(r);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys_win_rect_preview(int enable, int x, int y, int w, int h)
{
	int err;
	
	err = win_rect_preview(enable, x, y, w, h);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_load_font(int *ftd, const char *name, const char *pathname)
{
	int err;
	int d;
	
	err = usa(&name, FONT_NAME_MAX, ERANGE);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = usa(&pathname, PATH_MAX - 1, ERANGE);
	if (err)
	{
		uerr(err);
		return err;
	}
	
	err = win_load_font(&d, name, pathname);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = tucpy(ftd, &d, sizeof d);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_find_font(int *ftd, const char *name)
{
	int err;
	int d;
	
	err = usa(&name, FONT_NAME_MAX, ERANGE);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = win_find_font(&d, name);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = tucpy(ftd, &d, sizeof d);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_set_font(int wd, int ftd)
{
	int err;
	
	err = win_set_font(wd, ftd);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_dispflags(void)
{
	struct win_desktop *d = curr->win_task.desktop;
	
	if (!d)
	{
		uerr(ENODESKTOP);
		return -1;
	}
	
	return d->display->flags;
}

int sys_win_modeinfo(int i, struct win_modeinfo *mi)
{
	int err;
	
	err = uga(&mi, sizeof *mi, UA_WRITE);
	if (err)
		goto err;
	
	err = win_modeinfo(i, mi);
	if (err)
		goto err;
	
	return 0;
err:
	uerr(err);
	return -1;
}

int sys_win_setmode(int i, int refresh)
{
	int err;
	
	err = win_setmode(i, refresh);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys_win_getmode(int *i)
{
	int err;
	int li;
	
	err = win_getmode(&li);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = tucpy(i, &li, sizeof li);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_get_title(int wd, char *buf, int max)
{
	char *p;
	int err;
	
	if (!curr->win_task.desktop)
	{
		uerr(ENODESKTOP);
		return -1;
	}
	
	if (wd < 0 || wd >= WIN_MAX || !curr->win_task.desktop->window[wd].task)
	{
		uerr(EINVAL);
		return -1;
	}
	
	p = curr->win_task.desktop->window[wd].title;
	if (strlen(p) >= max)
	{
		uerr(EINVAL);
		return -1;
	}
	
	err = tucpy(buf, p, strlen(p) + 1);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_win_set_title(int wd, const char *title)
{
	struct win_desktop *d = curr->win_task.desktop;
	int err;
	
	err = win_chkwd(wd);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = usa(&title, WIN_TITLE_MAX, ERANGE);
	if (err)
	{
		uerr(err);
		return -1;
	}
	strcpy(curr->win_task.desktop->window[wd].title, title);
	
	if (d->taskbar)
	{
		struct event e;
		
		memset(&e, 0, sizeof e);
		e.win.type = WIN_E_TITLE;
		e.win.wd   = wd;
		e.type	   = E_WINGUI;
		e.proc	   = d->taskbar_proc;
		send_event(d->taskbar, &e);
	}
	return 0;
}

int sys_win_is_visible(int wd, int *visible)
{
	int err;
	int lv;
	
	err = win_is_visible(wd, &lv);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = tucpy(visible, &lv, sizeof lv);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys_win_redraw_all()
{
	int err;
	
	err = win_redraw_all();
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys_win_setcte(win_color c, int r, int g, int b)
{
	int err;
	
	err = win_setcte(c, r, g, b);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys_win_getcte(win_color c, int *r, int *g, int *b)
{
	int err;
	
	err = uga(&r, sizeof *r, UA_WRITE);
	if (err)
		goto err;
	
	err = uga(&g, sizeof *r, UA_WRITE);
	if (err)
		goto err;
	
	err = uga(&b, sizeof *r, UA_WRITE);
	if (err)
		goto err;
	
	err = win_getcte(c, r, g, b);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
err:
	uerr(err);
	return -1;
}

int sys_win_update(void)
{
	int err;
	
	err = win_update();
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys_win_reset(void)
{
	int err;
	
	err = win_reset();
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys_win_insert_ptr(int ptr, const win_color *pixels, const unsigned *mask,
		       int w, int h, int x, int y)
{
	int err;
	
	if (w < 0 || w > PTR_WIDTH)
	{
		uerr(EINVAL);
		return -1;
	}
	
	if (h < 0 || h > PTR_HEIGHT)
	{
		uerr(EINVAL);
		return -1;
	}
	
	err = uaa(&pixels, sizeof *pixels, w * h, UA_READ);
	if (err)
		goto err;
	
	err = uaa(&mask, sizeof *mask, w * h, UA_READ);
	if (err)
		goto err;
	
	err = win_insert_ptr(ptr, pixels, mask, w, h, x, y);
	if (err)
		goto err;
	
	return 0;
err:
	uerr(err);
	return -1;
}

int sys_win_set_ptr_speed(const struct win_ptrspeed *speed)
{
	int err;
	
	err = uga(&speed, sizeof *speed, UA_READ);
	if (err)
		goto err;
	
	err = win_set_ptr_speed(speed);
	if (err)
		goto err;
	
	return 0;
err:
	uerr(err);
	return -1;
}

int sys_win_get_ptr_speed(struct win_ptrspeed *speed)
{
	int err;
	
	err = uga(&speed, sizeof *speed, UA_WRITE);
	if (err)
		goto err;
	
	err = win_get_ptr_speed(speed);
	if (err)
		goto err;
	
	return 0;
err:
	uerr(err);
	return -1;
}

int sys_win_map_buttons(int *tab, int cnt)
{
	int err;
	
	if (cnt <= 0 || cnt > WIN_BTN_MAX)
	{
		uerr(EINVAL);
		return -1;
	}
	
	err = uaa(&tab, sizeof *tab, WIN_BTN_MAX, UA_READ);
	if (err)
		goto err;
	
	err = win_map_buttons(tab, cnt);
	if (err)
		goto err;
	
	return 0;
err:
	uerr(err);
	return -1;
}

int sys_win_taskbar(void *proc)
{
	struct win_desktop *d = curr->win_task.desktop;
	struct event e;
	
	if (!d)
	{
		uerr(ENODESKTOP);
		return -1;
	}
	if (d->taskbar)
	{
		uerr(EEXIST);
		return -1;
	}
	d->taskbar_proc	= proc;
	d->taskbar	= curr;
	
	memset(&e, 0, sizeof e);
	e.win.type	= WIN_E_SWITCH_TO;
	e.win.wd	= d->focus;
	e.type		= E_WINGUI;
	e.proc		= d->taskbar_proc;
	send_event(d->taskbar, &e);
	return 0;
}

int sys_win_chk_taskbar(void)
{
	struct win_desktop *d = curr->win_task.desktop;
	
	if (!d)
	{
		uerr(ENODESKTOP);
		return -1;
	}
	if (d->taskbar == NULL)
	{
		uerr(ESRCH);
		return -1;
	}
	return 0;
}

int sys_win_sec_unlock(void)
{
	int err;
	
	err = win_sec_unlock();
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys_win_setlayer(int wd, int layer)
{
	int err;
	
	err = win_setlayer(wd, layer);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys_win_setptr(int wd, int ptr)
{
	int err;
	
	err = win_setptr(wd, ptr);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys_win_deflayer(int *layer)
{
	int err;
	
	err = uga(&layer, sizeof *layer, UA_WRITE);
	if (err)
		goto err;
	
	err = win_deflayer(layer);
	if (err)
		goto err;
	
	return 0;
err:
	uerr(err);
	return -1;
}

int sys_win_soft_keydown(unsigned ch)
{
	int err;
	
	err = win_soft_keydown(ch);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys_win_soft_keyup(unsigned ch)
{
	int err;
	
	err = win_soft_keyup(ch);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys_win_blink(int wd, int ena)
{
	int err;
	
	err = win_blink(wd, ena);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys_win_dragdrop(const void *data, size_t size)
{
	struct win_desktop *d = curr->win_task.desktop;
	int err;
	
	if (!d)
	{
		uerr(ENODESKTOP);
		return -1;
	}
	
	err = win_chkwd(d->ptr_down_wd);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	if (size > sizeof d->dd_buf)
	{
		uerr(ENOMEM);
		return -1;
	}
	
	if (d->dragdrop)
	{
		uerr(EBUSY);
		return -1;
	}
	
	err = uga(&data, size, UA_READ);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	memcpy(d->dd_buf, data, size);
	d->dd_len	= size;
	d->dragdrop	= 1;
	d->dd_serial++;
	
	return 0;
}

int sys_win_gdrop(void *buf, size_t bufsz, int *serial)
{
	struct win_desktop *d = curr->win_task.desktop;
	int err;
	
	if (!d)
	{
		uerr(ENODESKTOP);
		return -1;
	}
	
	if (bufsz < sizeof d->dd_len)
	{
		uerr(ENOMEM);
		return -1;
	}
	
	err = uga(&buf, bufsz, UA_WRITE);
	if (err)
		goto err;
	
	err = tucpy(serial, &d->dd_serial, sizeof d->dd_serial);
	if (err)
		goto err;
	
	memcpy(buf, d->dd_buf, d->dd_len);
	return 0;
err:
	uerr(err);
	return -1;
}
