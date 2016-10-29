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

#include <priv/wingui_form.h>
#include <wingui_color.h>
#include <wingui_form.h>
#include <wingui.h>
#include <timer.h>
#include <errno.h>
#include <time.h>

static const struct timeval vsbar_repeat = { 0, 100000 };
static const struct timeval vsbar_delay  = { 0, 250000 };

static void vsbar_make_rects(struct gadget *g)
{
	g->sbar.sbar_rect.x = 0;
	g->sbar.sbar_rect.y = g->rect.w;
	g->sbar.sbar_rect.w = g->rect.w;
	g->sbar.sbar_rect.h = g->rect.h - 2 * g->sbar.sbar_rect.y;
	
	g->sbar.sbox_rect.w = g->sbar.sbar_rect.w;
	g->sbar.sbox_rect.h = g->sbar.sbar_rect.w;
	
	if (g->sbar.limit < 2)
	{
		g->sbar.sbox_rect.x = 0;
		g->sbar.sbox_rect.y = 0;
		g->sbar.sbox_rect.w = 0;
		g->sbar.sbox_rect.h = 0;
		return;
	}
	if (g->sbar.pos < 0 || g->sbar.pos >= g->sbar.limit)
	{
		g->sbar.sbox_rect.x = 0;
		g->sbar.sbox_rect.y = 0;
		g->sbar.sbox_rect.w = 0;
		g->sbar.sbox_rect.h = 0;
		return;
	}
	
	g->sbar.sbox_rect.x  =  g->sbar.sbar_rect.x;
	g->sbar.sbox_rect.y  =  g->sbar.sbar_rect.h - g->sbar.sbox_rect.h;
	g->sbar.sbox_rect.y *=  g->sbar.pos;
	g->sbar.sbox_rect.y /= (g->sbar.limit - 1);
	g->sbar.sbox_rect.y +=  g->sbar.sbar_rect.y;
}

static void vsbar_uparrow(struct gadget *g, int wd)
{
	int w = g->rect.w;
	int h = g->sbar.sbar_rect.y;
	win_color hi1;
	win_color sh1;
	win_color hi2;
	win_color sh2;
	win_color bg;
	win_color fg;
	int x0, y0;
	int ah;
	int i;
	
	hi1 = wc_get(WC_HIGHLIGHT1);
	sh1 = wc_get(WC_SHADOW1);
	hi2 = wc_get(WC_HIGHLIGHT2);
	sh2 = wc_get(WC_SHADOW2);
	
	if (g->sbar.uparrow_active)
	{
		fg = wc_get(WC_SEL_FG);
		bg = wc_get(WC_SEL_BG);
	}
	else
	{
		fg = wc_get(WC_WIN_FG);
		bg = wc_get(WC_WIN_BG);
	}
	
	form_draw_rect3d(wd, 0, 0, w, h, hi1, hi2, sh1, sh2, bg);
	
	x0 = g->rect.w / 2;
	ah = h / 4;
	y0 = (g->sbar.sbar_rect.y - ah) / 2;
	for (i = 0; i < ah; i++)
		win_hline(wd, fg, x0 - i, y0 + i, i * 2);
}

static void vsbar_dnarrow(struct gadget *g, int wd)
{
	int y = g->sbar.sbar_rect.y + g->sbar.sbar_rect.h;
	int w = g->rect.w;
	int h = g->sbar.sbar_rect.y;
	win_color hi1;
	win_color sh1;
	win_color hi2;
	win_color sh2;
	win_color bg;
	win_color fg;
	int x0, y0;
	int ah;
	int i;
	
	hi1 = wc_get(WC_HIGHLIGHT1);
	sh1 = wc_get(WC_SHADOW1);
	hi2 = wc_get(WC_HIGHLIGHT2);
	sh2 = wc_get(WC_SHADOW2);
	
	if (g->sbar.dnarrow_active)
	{
		fg = wc_get(WC_SEL_FG);
		bg = wc_get(WC_SEL_BG);
	}
	else
	{
		fg = wc_get(WC_WIN_FG);
		bg = wc_get(WC_WIN_BG);
	}
	
	form_draw_rect3d(wd, 0, y, w, h, hi1, hi2, sh1, sh2, bg);
	
	x0 = g->rect.w / 2;
	ah = h / 4;
	y0 = y + (g->sbar.sbar_rect.y + ah) / 2;
	for (i = 0; i < ah; i++)
		win_hline(wd, fg, x0 - i, y0 - i, i * 2);
}

static void vsbar_redraw(struct gadget *g, int wd)
{
	win_color sbox;
	win_color hi1;
	win_color sh1;
	win_color hi2;
	win_color sh2;
	win_color bg;
	win_color fg;
	int y0, y1;
	
	hi1  = wc_get(WC_HIGHLIGHT1);
	sh1  = wc_get(WC_SHADOW1);
	hi2  = wc_get(WC_HIGHLIGHT2);
	sh2  = wc_get(WC_SHADOW2);
	sbox = wc_get(WC_WIN_BG);
	fg   = wc_get(WC_FG);
	bg   = wc_get(WC_SBAR);
	
	if (g->sbar.sbox_active)
		sbox = wc_get(WC_SEL_BG);
	
	if (g->sbar.limit < 2)
		goto defunct;
	if (g->sbar.pos < 0 || g->sbar.pos >= g->sbar.limit)
		goto defunct;
	
	vsbar_make_rects(g);
	vsbar_uparrow(g, wd);
	vsbar_dnarrow(g, wd);
	
	y0 = g->rect.w;
	y1 = g->sbar.sbox_rect.y;
	win_rect(wd, bg, 0, y0, g->rect.w, y1 - y0);
	
	y0 = g->sbar.sbox_rect.y + g->sbar.sbox_rect.h;
	y1 = g->rect.h - g->rect.w;
	win_rect(wd, bg, 0, y0, g->rect.w, y1 - y0);
	
	form_draw_rect3d(wd, g->sbar.sbox_rect.x,
			     g->sbar.sbox_rect.y,
			     g->sbar.sbox_rect.w,
			     g->sbar.sbox_rect.h,
			     hi1, hi2, sh1, sh2, sbox);
	
	return;
defunct:
	win_rect(wd, bg, 0, 0, g->rect.w, g->rect.h);
}

static void vsbar_ptr_move(struct gadget *g, int x, int y)
{
	int pos;
	
	if (g->sbar.sbox_active)
	{
		pos  = y - g->sbar.sbar_rect.y;
		pos *= g->sbar.limit;
		pos /= g->sbar.sbar_rect.h;
		
		if (pos < 0)
			pos = 0;
		if (pos >= g->sbar.limit)
			pos = g->sbar.limit - 1;
		
		vsbar_set_pos(g, pos);
	}
}

static void vsbar_ptr_down(struct gadget *g, int x, int y, int button)
{
	if (g->read_only)
		return;
	
	if (y < g->sbar.sbar_rect.y)
	{
		g->sbar.uparrow_active = 1;
		vsbar_set_pos(g, g->sbar.pos - g->sbar.step);
		tmr_reset(g->sbar.timer, &vsbar_delay, &vsbar_repeat);
		tmr_start(g->sbar.timer);
		gadget_redraw(g);
	}
	
	if (y >= g->sbar.sbar_rect.y + g->sbar.sbar_rect.h)
	{
		g->sbar.dnarrow_active = 1;
		vsbar_set_pos(g, g->sbar.pos + g->sbar.step);
		tmr_reset(g->sbar.timer, &vsbar_delay, &vsbar_repeat);
		tmr_start(g->sbar.timer);
		gadget_redraw(g);
	}
	
	if (y >= g->sbar.sbox_rect.y &&
	    y <  g->sbar.sbox_rect.y + g->sbar.sbox_rect.h)
		g->sbar.sbox_active = 1;
}

static void vsbar_ptr_up(struct gadget *g, int x, int y, int button)
{
	if (g->sbar.sbox_active)
	{
		g->sbar.sbox_active = 0;
		gadget_redraw(g);
	}
	
	if (g->sbar.uparrow_active)
	{
		g->sbar.uparrow_active = 0;
		tmr_stop(g->sbar.timer);
		gadget_redraw(g);
	}
	
	if (g->sbar.dnarrow_active)
	{
		g->sbar.dnarrow_active = 0;
		tmr_stop(g->sbar.timer);
		gadget_redraw(g);
	}
}

static void vsbar_timer_cb(void *cx)
{
	struct gadget *g = cx;
	
	if (g->sbar.uparrow_active)
		vsbar_set_pos(g, g->sbar.pos - g->sbar.step);
	if (g->sbar.dnarrow_active)
		vsbar_set_pos(g, g->sbar.pos + g->sbar.step);
}

static void vsbar_remove(struct gadget *g)
{
	tmr_free(g->sbar.timer);
}

struct gadget *vsbar_creat(struct form *form, int x, int y, int w, int h)
{
	struct gadget *g;
	int se;
	
	g = gadget_creat(form, x, y, w, h);
	if (!g)
		return NULL;
	
	g->sbar.timer = tmr_creat(&vsbar_delay, &vsbar_repeat, vsbar_timer_cb, g, 0);
	if (!g->sbar.timer)
	{
		se = _get_errno();
		gadget_remove(g);
		_set_errno(se);
		return NULL;
	}
	g->sbar.step = 1;
	
	g->remove   = vsbar_remove;
	g->redraw   = vsbar_redraw;
	g->ptr_move = vsbar_ptr_move;
	g->ptr_down = vsbar_ptr_down;
	g->ptr_up   = vsbar_ptr_up;

	vsbar_make_rects(g);
	gadget_redraw(g);
	return g;
}

int vsbar_set_step(struct gadget *g, int step)
{
	if (step < 1)
	{
		_set_errno(EINVAL);
		return -1;
	}
	g->sbar.step = step;
	return 0;
}

int vsbar_set_limit(struct gadget *g, int limit)
{
	int prev = g->sbar.limit;
	
	if (limit < 0)
		g->sbar.limit = 0;
	else
		g->sbar.limit = limit;
	
	if (prev != g->sbar.limit)
	{
		vsbar_make_rects(g);
		gadget_redraw(g);
	}
	
	return 0;
}

int vsbar_set_pos(struct gadget *g, int pos)
{
	int prev = g->sbar.pos;
	
	g->sbar.pos = pos;
	if (g->sbar.pos >= g->sbar.limit)
		g->sbar.pos = g->sbar.limit - 1;
	if (g->sbar.pos < 0)
		g->sbar.pos = 0;
	
	if (prev != g->sbar.pos)
	{
		vsbar_make_rects(g);
		gadget_redraw(g);
		if (g->sbar.move)
			g->sbar.move(g, g->sbar.pos);
	}
	
	return 0;
}

int vsbar_get_pos(struct gadget *g)
{
	return g->sbar.pos;
}

int vsbar_on_move(struct gadget *g, sbar_move_cb *proc)
{
	g->sbar.move = proc;
	return 0;
}
