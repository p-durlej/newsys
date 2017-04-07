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

#include <kern/wingui.h>
#include <kern/printk.h>
#include <kern/task.h>
#include <errno.h>

struct win_request
{
	int (*proc)(struct win_request *rq);
	
	int			clip_x0;
	int			clip_y0;
	int			clip_x1;
	int			clip_y1;
	
	win_color		bg_color;
	win_color		color;
	const win_color *	bitmap;
	unsigned char *		text;
	unsigned char		ch;
	
	struct win_rect		rect;
	struct win_font *	font;
	struct win_display *	display;
	struct win_window *	window;
	
	int			src_x, src_y;
};

static int win_autoclip_iterate(struct win_window *w, struct win_request *rq)
{
	struct win_desktop *d = curr->win_task.desktop;
	struct win_request nrq;
	int err;
	
restart:
	if (!w)
		return rq->proc(rq);
	
	if (!w->visible)
	{
		w = list_prev(&d->stack, w);
		goto restart;
	}
	
	if (rq->clip_x0 >= w->rect.x + w->rect.w)
	{
		w = list_prev(&d->stack, w);
		goto restart;
	}
	
	if (rq->clip_y0 >= w->rect.y + w->rect.h)
	{
		w = list_prev(&d->stack, w);
		goto restart;
	}
	
	if (rq->clip_x1 <= w->rect.x)
	{
		w = list_prev(&d->stack, w);
		goto restart;
	}
	
	if (rq->clip_y1 <= w->rect.y)
	{
		w = list_prev(&d->stack, w);
		goto restart;
	}
	
	if (w->rect.y > rq->clip_y0)
	{
		nrq = *rq;
		nrq.clip_y1 = w->rect.y;
		err = win_autoclip_iterate(list_prev(&d->stack, w), &nrq);
		if (err)
			return err;
	}
	
	if (w->rect.y + w->rect.h < rq->clip_y1)
	{
		nrq = *rq;
		nrq.clip_y0 = w->rect.y + w->rect.h;
		err = win_autoclip_iterate(list_prev(&d->stack, w), &nrq);
		if (err)
			return err;
	}
	
	if (w->rect.x > rq->clip_x0)
	{
		nrq = *rq;
		nrq.clip_x1 = w->rect.x;
		if (nrq.clip_y0 < w->rect.y)
			nrq.clip_y0 = w->rect.y;
		if (nrq.clip_y1 > w->rect.y + w->rect.h)
			nrq.clip_y1 = w->rect.y + w->rect.h;
		err = win_autoclip_iterate(list_prev(&d->stack, w), &nrq);
		if (err)
			return err;
	}
	
	if (w->rect.x + w->rect.w < rq->clip_x1)
	{
		nrq = *rq;
		nrq.clip_x0 = w->rect.x + w->rect.w;
		if (nrq.clip_y0 < w->rect.y)
			nrq.clip_y0 = w->rect.y;
		if (nrq.clip_y1 > w->rect.y + w->rect.h)
			nrq.clip_y1 = w->rect.y + w->rect.h;
		err = win_autoclip_iterate(list_prev(&d->stack, w), &nrq);
		if (err)
			return err;
	}
	
	return 0;
}

static int win_autoclip(int wd, struct win_request *rq)
{
	struct win_desktop *d = curr->win_task.desktop;
	struct win_request nrq;
	struct win_window *w;
	int err;
	
	err = win_chkwd(wd);
	if (err)
		return err;
	
	w = &d->window[wd];
	
	if (!w->visible)
		return 0;
	
	nrq = *rq;
	nrq.window  = w;
	nrq.display = d->display;
	nrq.clip_x0 = w->clip.x;
	nrq.clip_y0 = w->clip.y;
	nrq.clip_x1 = w->clip.x + w->clip.w;
	nrq.clip_y1 = w->clip.y + w->clip.h;
	
	nrq.clip_x0 += w->rect.x;
	nrq.clip_y0 += w->rect.y;
	nrq.clip_x1 += w->rect.x;
	nrq.clip_y1 += w->rect.y;
	nrq.rect.x += w->x0;
	
	nrq.rect.y += w->y0;
	nrq.src_x  += w->x0;
	nrq.src_y  += w->y0;
	
	nrq.rect.x += w->rect.x;
	nrq.rect.y += w->rect.y;
	nrq.src_x  += w->rect.x;
	nrq.src_y  += w->rect.y;
	
	if (nrq.rect.w < 0)
		nrq.rect.w = 0;
	if (nrq.rect.h < 0)
		nrq.rect.h = 0;
	
	if (w->font == WIN_FONT_DEFAULT)
		nrq.font = &win_font[DEFAULT_FTD];
	else
		nrq.font = &win_font[w->font];
	
	if (nrq.font->data == NULL)
		return EINVAL;
	
	if (nrq.clip_x0 < 0)
		nrq.clip_x0 = 0;
	
	if (nrq.clip_y0 < 0)
		nrq.clip_y0 = 0;
	
	if (nrq.clip_x1 > d->display->width)
		nrq.clip_x1 = d->display->width;
	
	if (nrq.clip_y1 > d->display->height)
		nrq.clip_y1 = d->display->height;
	
	win_lock();
	
	err = win_autoclip_iterate(list_prev(&d->stack, w), &nrq);
	
	win_unlock();
	return err;
}

static void win_invpix(int w, int h, int x, int y)
{
	struct win_desktop *d = curr->win_task.desktop;
	void *dd = d->display->data;
	win_color c;
	
	if (x >= w || y >= h)
		return;
	if (x < 0 || y < 0)
		return;
	
	d->display->getpix(dd, x, y, &c);
	d->display->invert(dd, &c);
	d->display->putpix(dd, x, y, c);
}

void win_rect_preview_invert(int ox, int oy, int w, int h)
{
	struct win_desktop *d = curr->win_task.desktop;
	int x0 = d->rect_preview.x - ox;
	int y0 = d->rect_preview.y - oy;
	int x1 = d->rect_preview.w + x0;
	int y1 = d->rect_preview.h + y0;
	int x, y;
	
	if (!d->rect_preview_ena)
		return;
	
	if (x1 > d->display->width)
		x1 = d->display->width;
	if (y1 > d->display->height)
		y1 = d->display->height;
	
	if (x0 >= x1 || y0 >= y1)
		return;
	
	win_lock();
	for (y = y0 + 1; y < y1 - 1; y += 2)
	{
		win_invpix(w, h, x0,	 y);
		win_invpix(w, h, x1 - 1, y);
	}
	for (x = x0; x < x1; x += 2)
	{
		win_invpix(w, h, x, y0);
		win_invpix(w, h, x, y1 - 1);
	}
	win_unlock();
}

int win_paint(void)
{
	struct win_desktop *d = curr->win_task.desktop;
	
	if (!d)
		return ENODESKTOP;
	
	if (curr->win_task.painting >= 256)
		return EINVAL;
	
	curr->win_task.painting++;
	d->painting++;
	
	if (d->painting == 1)
	{
		win_lock();
		d->display->hideptr(d->display->data);
		win_unlock();
		if (d->rect_preview_ena)
			win_rect_preview_invert(0, 0, d->display->width, d->display->height);
	}
	
	return 0;
}

int win_end_paint(void)
{
	struct win_desktop *d = curr->win_task.desktop;
	
	if (!d)
		return ENODESKTOP;
	
	if (!curr->win_task.painting)
		return EINVAL;
	
	if (d->painting == 1)
	{
		if (d->rect_preview_ena)
			win_rect_preview_invert(0, 0, d->display->width, d->display->height);
		win_lock();
		d->display->showptr(d->display->data);
		win_unlock();
	}
	
	curr->win_task.painting--;
	d->painting--;
	
	return 0;
}

int win_rgb2color(win_color *c, int r, int g, int b)
{
	struct win_desktop *d = curr->win_task.desktop;
	
	if (!d)
		return ENODESKTOP;
	
	d->display->rgba2color(d->display->data, r, g, b, 255, c);
	return 0;
}

int win_rgba2color(win_color *c, int r, int g, int b, int a)
{
	struct win_desktop *d = curr->win_task.desktop;
	
	if (!d)
		return ENODESKTOP;
	
	d->display->rgba2color(d->display->data, r, g, b, a, c);
	return 0;
}

int win_clip(int wd, int x, int y, int w, int h, int x0, int y0)
{
	struct win_window *win;
	int err;
	
	err = win_chkwd(wd);
	if (err)
		return err;
	
	win = &curr->win_task.desktop->window[wd];
	
	win->x0 = x0;
	win->y0 = y0;
	win->clip.x = x;
	win->clip.y = y;
	win->clip.w = w;
	win->clip.h = h;
	
	if (x < 0)
	{
		win->clip.w += win->clip.x;
		win->clip.x = 0;
	}
	
	if (y < 0)
	{
		win->clip.h += win->clip.y;
		win->clip.y = 0;
	}
	
	if (win->clip.w < 0)
		win->clip.w = 0;
	
	if (win->clip.h < 0)
		win->clip.h = 0;
	
	if (win->clip.x + w > win->rect.w)
		win->clip.w = win->rect.w - win->clip.x;
	
	if (win->clip.y + h > win->rect.h)
		win->clip.h = win->rect.h - win->clip.y;
	
	return 0;
}

static int win_autoclip_pixel(struct win_request *rq)
{
	if (rq->clip_x0 > rq->rect.x)
		return 0;
	
	if (rq->clip_y0 > rq->rect.y)
		return 0;
	
	if (rq->clip_x1 <= rq->rect.x)
		return 0;
	
	if (rq->clip_y1 <= rq->rect.y)
		return 0;
	
	rq->display->putpix(rq->display->data, rq->rect.x, rq->rect.y, rq->color);
	return 0;
}

static int win_autoclip_hline(struct win_request *rq)
{
	int x0 = rq->rect.x;
	int y0 = rq->rect.y;
	int x1 = rq->rect.x + rq->rect.w;
	
	if (y0 < rq->clip_y0)
		return 0;
	
	if (y0 >= rq->clip_y1)
		return 0;
	
	if (rq->clip_x0 > x0)
		x0 = rq->clip_x0;
	
	if (rq->clip_x1 < x1)
		x1 = rq->clip_x1;
	
	if (x1 < x0)
		return 0;
	
	rq->display->hline(rq->display->data, x0, y0, x1 - x0, rq->color);
	return 0;
}

static int win_autoclip_vline(struct win_request *rq)
{
	int x0 = rq->rect.x;
	int y0 = rq->rect.y;
	int y1 = rq->rect.y + rq->rect.h;
	
	if (x0 < rq->clip_x0)
		return 0;
	
	if (x0 >= rq->clip_x1)
		return 0;
	
	if (rq->clip_y0 > y0)
		y0 = rq->clip_y0;
	
	if (rq->clip_y1 < y1)
		y1 = rq->clip_y1;
	
	if (y1 < y0)
		return 0;
	
	rq->display->vline(rq->display->data, x0, y0, y1 - y0, rq->color);
	return 0;
}

static int win_autoclip_rect(struct win_request *rq)
{
	int x0 = rq->rect.x;
	int y0 = rq->rect.y;
	int x1 = rq->rect.x + rq->rect.w;
	int y1 = rq->rect.y + rq->rect.h;
	
	if (rq->clip_x0 > x0)
		x0 = rq->clip_x0;
	
	if (rq->clip_y0 > y0)
		y0 = rq->clip_y0;
	
	if (rq->clip_x1 < x1)
		x1 = rq->clip_x1;
	
	if (rq->clip_y1 < y1)
		y1 = rq->clip_y1;
	
	if (x1 < x0 || y1 < y0)
		return 0;
	
	rq->display->rect(rq->display->data,
			x0, y0, x1 - x0, y1 - y0, rq->color);
	return 0;
}

static int win_autoclip_copy(struct win_request *rq)
{
	int x0 = rq->rect.x;
	int y0 = rq->rect.y;
	int x1 = rq->rect.x + rq->rect.w;
	int y1 = rq->rect.y + rq->rect.h;
	
	if (rq->clip_x0 > x0)
		return EPERM;
	
	if (rq->clip_y0 > y0)
		return EPERM;
	
	if (rq->clip_x1 < x1)
		return EPERM;
	
	if (rq->clip_y1 < y1)
		return EPERM;
	
	if (x1 < x0 || y1 < y0)
		return 0;
	
	rq->display->copy(rq->display->data, x0, y0, rq->src_x, rq->src_y, rq->rect.w, rq->rect.h);
	return 0;
}

static int win_rfont(struct win_font *font, int x, int y)
{
	return (font->bitmap[x / 8 + y * font->linelen] >> (x & 7)) & 1;
}

static int win_autoclip_glyph(struct win_request *rq, int x0, int y0, struct win_glyph *gl, struct win_font *font)
{
	void *dd = rq->display->data;
	int i = 0;
	int x;
	int y;
	
	for (y = 0; y < gl->height; y++)
		for (x = 0; x < gl->width; x++, i++)
		{
			if (x + x0 < rq->clip_x0)
				continue;
			if (y + y0 < rq->clip_y0)
				continue;
			if (x + x0 >= rq->clip_x1)
				continue;
			if (y + y0 >= rq->clip_y1)
				continue;
			
			if (win_rfont(font, gl->pos + x, y))
				rq->display->putpix(dd, x + x0, y + y0, rq->color);
		}
	return 0;
}

static void win_bmp_hline(struct win_display *d, win_color bg, win_color fg, int x, int y, int w, const uint8_t *data, int off)
{
	const uint8_t *p = data + (off >> 3);
	void *dd = d->data;
	uint8_t b;
	int nb;
	
	if (d->bmp_hline)
	{
		d->bmp_hline(dd, x, y, w, data, off, bg, fg);
		return;
	}
	
	b  = *p++ >> (off & 7);
	nb = 8 - (off & 7);
	for (; w; x++, w--)
	{
		if (b & 1)
			d->putpix(dd, x, y, fg);
		else
			d->putpix(dd, x, y, bg);
		
		if (!--nb)
		{
			b  = *p++;
			nb = 8;
		}
		else
			b >>= 1;
	}
}

static int win_autoclip_bglyph_fast(struct win_request *rq, int x0, int y0, struct win_glyph *gl, struct win_font *font)
{
	win_color bg = rq->bg_color;
	win_color fg = rq->color;
	int y;
	
	for (y = 0; y < gl->height; y++)
		win_bmp_hline(rq->display, bg, fg, x0, y0 + y, gl->width, font->bitmap + y * font->linelen, gl->pos);
	
	return 0;
}

static int win_autoclip_bglyph(struct win_request *rq, int x0, int y0, struct win_glyph *gl, struct win_font *font)
{
	void *dd = rq->display->data;
	int i = 0;
	int x;
	int y;
	
	if (x0 >= rq->clip_x0 && y0 >= rq->clip_y0 && x0 + gl->width <= rq->clip_x1 && y0 + gl->height <= rq->clip_y1)
		return win_autoclip_bglyph_fast(rq, x0, y0, gl, font);
	
	for (y = 0; y < gl->height; y++)
		for (x = 0; x < gl->width; x++, i++)
		{
			if (x + x0 < rq->clip_x0)
				goto next;
			if (y + y0 < rq->clip_y0)
				goto next;
			if (x + x0 >= rq->clip_x1)
				goto next;
			if (y + y0 >= rq->clip_y1)
				goto next;
			
			if (win_rfont(font, gl->pos + x, y))
				rq->display->putpix(dd, x + x0, y + y0, rq->color);
			else
				rq->display->putpix(dd, x + x0, y + y0, rq->bg_color);
next:
			;
		}
	return 0;
}

static int win_autoclip_text(struct win_request *rq)
{
	unsigned char *p = rq->text;
	struct win_glyph *gl;
	int x = rq->rect.x;
	int y = rq->rect.y;
	
	while (*p)
	{
		switch (*p)
		{
		case '\n':
			x  = rq->rect.x;
			y += rq->font->glyph[0]->height;
			break;
		case '\r':
			break;
		default:
			gl = rq->font->glyph[*p];
			
			win_autoclip_glyph(rq, x, y, gl, rq->font);
			x += gl->width;
		}
		
		p++;
	}
	
	return 0;
}

static int win_autoclip_btext(struct win_request *rq)
{
	unsigned char *p = rq->text;
	struct win_glyph *gl;
	int x = rq->rect.x;
	int y = rq->rect.y;
	
	while (*p)
	{
		switch (*p)
		{
			case '\n':
				x  = rq->rect.x;
				y += rq->font->glyph[0]->height;
				break;
			case '\r':
				break;
			default:
				gl = rq->font->glyph[*p];
				
				win_autoclip_bglyph(rq, x, y, gl, rq->font);
				x += gl->width;
		}
		
		p++;
	}
	
	return 0;
}

static int win_autoclip_chr(struct win_request *rq)
{
	win_autoclip_glyph(rq, rq->rect.x, rq->rect.y,
		rq->font->glyph[rq->ch], rq->font);
	return 0;
}

static int win_autoclip_bchr(struct win_request *rq)
{
	win_autoclip_bglyph(rq, rq->rect.x, rq->rect.y,
		rq->font->glyph[rq->ch], rq->font);
	return 0;
}

static int win_autoclip_bitmap(struct win_request *rq)
{
	win_color tr = rq->display->transparent;
	void *dd = rq->display->data;
	int x0 = rq->rect.x;
	int y0 = rq->rect.y;
	int x1 = rq->rect.x + rq->rect.w;
	int y1 = rq->rect.y + rq->rect.h;
	int x;
	int y;
	int i;
	
	i = 0;
	for (y = y0; y < y1; y++)
		for (x = x0; x < x1; x++, i++)
			if (x >= rq->clip_x0 && x <  rq->clip_x1 &&
			    y >= rq->clip_y0 && y <  rq->clip_y1 &&
			    rq->bitmap[i] != tr)
				rq->display->putpix(dd, x, y, rq->bitmap[i]);
	
	return 0;
}

int win_pixel(int wd, win_color c, int x, int y)
{
	struct win_request rq;
	
	rq.proc   = win_autoclip_pixel;
	rq.rect.x = x;
	rq.rect.y = y;
	rq.rect.w = 1;
	rq.rect.h = 1;
	rq.color  = c;
	
	return win_autoclip(wd, &rq);
}

int win_hline(int wd, win_color c, int x, int y, int l)
{
	struct win_request rq;
	
	rq.proc   = win_autoclip_hline;
	rq.rect.x = x;
	rq.rect.y = y;
	rq.rect.w = l;
	rq.rect.h = 1;
	rq.color  = c;
	
	return win_autoclip(wd, &rq);
}

int win_vline(int wd, win_color c, int x, int y, int l)
{
	struct win_request rq;
	
	rq.proc   = win_autoclip_vline;
	rq.rect.x = x;
	rq.rect.y = y;
	rq.rect.w = 1;
	rq.rect.h = l;
	rq.color  = c;
	
	return win_autoclip(wd, &rq);
}

int win_rect(int wd, win_color c, int x, int y, int w, int h)
{
	struct win_request rq;
	
	rq.proc   = win_autoclip_rect;
	rq.rect.x = x;
	rq.rect.y = y;
	rq.rect.w = w;
	rq.rect.h = h;
	rq.color  = c;
	
	return win_autoclip(wd, &rq);
}

int win_copy(int wd, int dx, int dy, int sx, int sy, int w, int h)
{
	struct win_request rq;
	struct event e;
	struct win_window *wp;
	int err;
	
	rq.proc	  = win_autoclip_copy;
	rq.rect.x = dx;
	rq.rect.y = dy;
	rq.rect.w = w;
	rq.rect.h = h;
	rq.src_x  = sx;
	rq.src_y  = sy;
	
	if (win_autoclip(wd, &rq))
	{
		err = win_chkwd(wd);
		if (err)
			return err;
		wp = &curr->win_task.desktop->window[wd];
		
		e.type	       = E_WINGUI;
		e.win.type     = WIN_E_REDRAW;
		e.win.wd       = wd;
		e.win.redraw_x = dx;
		e.win.redraw_y = dy;
		e.win.redraw_w = w;
		e.win.redraw_h = h;
		win_event(curr->win_task.desktop, &e);
	}
	return 0;
}

int win_text(int wd, win_color c, int x, int y, const char *text)
{
	struct win_request rq;
	
	rq.proc	  = win_autoclip_text;
	rq.text	  = (unsigned char *)text;
	rq.rect.x = x;
	rq.rect.y = y;
	rq.rect.w = 0;
	rq.rect.h = 0;
	rq.color  = c;
	
	return win_autoclip(wd, &rq);
}

int win_btext(int wd, win_color bg, win_color fg, int x, int y, const char *text)
{
	struct win_request rq;
	
	rq.proc		= win_autoclip_btext;
	rq.text		= (unsigned char *)text;
	rq.rect.x	= x;
	rq.rect.y	= y;
	rq.rect.w	= 0;
	rq.rect.h	= 0;
	rq.bg_color	= bg;
	rq.color	= fg;
	
	return win_autoclip(wd, &rq);
}

int win_text_size(int ftd, int *w, int *h, const char *text)
{
	unsigned char *p = (unsigned char *)text;
	struct win_font *ft;
	int curr_width = 0;
	int err;
	
	err = win_chk_ftd(ftd);
	if (err)
		return err;
	
	if (ftd == WIN_FONT_DEFAULT)
		ftd = DEFAULT_FTD;
	ftd = curr->win_task.desktop->font_map[ftd];
	ft = &win_font[ftd];
	
	*w = 0;
	*h = ft->glyph[0]->height;
	
	while (*p)
	{
		switch (*p)
		{
		case '\r':
			break;
		case '\n':
			curr_width = 0;
			*h += ft->glyph[0]->height;
			break;
		default:
			curr_width += ft->glyph[*p]->width;
			if (*w < curr_width)
				*w = curr_width;
		}
		
		p++;
	}
	
	return 0;
}

int win_chr(int wd, win_color c, int x, int y, unsigned ch)
{
	struct win_request rq;
	
	rq.proc = win_autoclip_chr;
	rq.ch = ch;
	rq.rect.x = x;
	rq.rect.y = y;
	rq.rect.w = 0;
	rq.rect.h = 0;
	rq.color  = c;
	
	return win_autoclip(wd, &rq);
}

int win_bchr(int wd, win_color bg, win_color fg, int x, int y, unsigned ch)
{
	struct win_request rq;

	rq.proc = win_autoclip_bchr;
	rq.ch = ch;
	rq.rect.x = x;
	rq.rect.y = y;
	rq.rect.w = 0;
	rq.rect.h = 0;
	rq.bg_color = bg;
	rq.color = fg;

	return win_autoclip(wd, &rq);
}

int win_chr_size(int ftd, int *w, int *h, unsigned ch)
{
	struct win_font *ft;
	int err;
	
	err = win_chk_ftd(ftd);
	if (err)
		return err;
	
	if (ftd == WIN_FONT_DEFAULT)
		ftd = DEFAULT_FTD;
	
	ftd = curr->win_task.desktop->font_map[ftd];
	ft = &win_font[ftd];
	
	*w = ft->glyph[ch & 0xff]->width;
	*h = ft->glyph[ch & 0xff]->height;
	
	return 0;
}

int win_bitmap(int wd, const win_color *bmp, int x, int y, int w, int h)
{
	struct win_request rq;
	
	rq.proc = win_autoclip_bitmap;
	rq.bitmap = bmp;
	rq.rect.x = x;
	rq.rect.y = y;
	rq.rect.w = w;
	rq.rect.h = h;
	
	return win_autoclip(wd, &rq);
}

int win_rect_preview(int enable, int x, int y, int w, int h)
{
	struct win_desktop *d = curr->win_task.desktop;
	
	if (!d)
		return ENODESKTOP;
	
	win_paint();
	
	d->rect_preview_ena = enable;
	d->rect_preview.x = x;
	d->rect_preview.y = y;
	d->rect_preview.w = w;
	d->rect_preview.h = h;
	
	win_end_paint();
	
	return 0;
}

int win_setcte(win_color c, int r, int g, int b)
{
	struct win_desktop *d = curr->win_task.desktop;
	
	if (!d)
		return ENODESKTOP;
	
	if (c <  d->display->user_cte)
		return EINVAL;
	if (c >= d->display->user_cte + d->display->user_cte_count)
		return EINVAL;
	
	win_lock();
	d->display->setcte(d->display->data, c, r, g, b);
	win_unlock();
	return 0;
}

int win_getcte(win_color c, int *r, int *g, int *b)
{
	struct win_desktop *d = curr->win_task.desktop;
	int a;
	
	if (!d)
		return ENODESKTOP;
	
	if (c < d->display->user_cte)
		return EINVAL;
	if (c >= d->display->user_cte + d->display->user_cte_count)
		return EINVAL;
	
	win_lock();
	d->display->color2rgba(d->display->data, r, g, b, &a, c);
	win_unlock();
	return 0;
}
