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
#include <wingui_form.h>
#include <wingui.h>
#include <errno.h>
#include <timer.h>
#include <time.h>

static const struct timeval hsbar_repeat = { 0, 100000 };
static const struct timeval hsbar_delay  = { 0, 250000 };

static void hsbar_make_rects(struct gadget *g)
{
	g->sbar.sbar_rect.x = g->rect.h;
	g->sbar.sbar_rect.y = 0;
	g->sbar.sbar_rect.w = g->rect.w - 2 * g->sbar.sbar_rect.x;
	g->sbar.sbar_rect.h = g->rect.h;
	
	g->sbar.sbox_rect.w = g->sbar.sbar_rect.h;
	g->sbar.sbox_rect.h = g->sbar.sbar_rect.h;
	
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
	
	g->sbar.sbox_rect.x = g->sbar.sbar_rect.w - g->sbar.sbox_rect.w;
	g->sbar.sbox_rect.x *= g->sbar.pos;
	g->sbar.sbox_rect.x /= (g->sbar.limit - 1);
	g->sbar.sbox_rect.x += g->sbar.sbar_rect.x;
	g->sbar.sbox_rect.y = 0;
}

static void hsbar_lfarrow(struct gadget *g, int wd)
{
	int w = g->sbar.sbar_rect.x;
	int h = g->rect.h;
	win_color hi1;
	win_color sh1;
	win_color hi2;
	win_color sh2;
	win_color bg;
	win_color fg;
	int x0, y0;
	int aw;
	int i;
	
	hi1 = wc_get(WC_HIGHLIGHT1);
	sh1 = wc_get(WC_SHADOW1);
	hi2 = wc_get(WC_HIGHLIGHT2);
	sh2 = wc_get(WC_SHADOW2);
	
	if (g->sbar.lfarrow_active)
	{
		fg = wc_get(WC_SEL_FG);
		bg = wc_get(WC_SEL_BG);
	}
	else
	{
		fg = wc_get(WC_WIN_FG);
		bg = wc_get(WC_WIN_BG);
	}
	
	win_rect(wd, bg, 0, 0, w, h);
	
	win_hline(wd, hi1, 0,	  0,     w);
	win_hline(wd, sh1, 0,	  h - 1, w);
	win_vline(wd, hi1, 0,	  0,     h);
	win_vline(wd, sh1, w - 1, 0,	 h);
	
	win_hline(wd, hi2, 1,	  1,     w - 2);
	win_hline(wd, sh2, 1,	  h - 2, w - 2);
	win_vline(wd, hi2, 1,	  1,     h - 2);
	win_vline(wd, sh2, w - 2, 1,	 h - 2);
	
	y0 = g->rect.h / 2;
	aw = w / 4;
	x0 = (w - aw) / 2;
	for (i = 0; i < aw; i++)
		win_vline(wd, fg, x0 + i, y0 - i, i * 2);
}

static void hsbar_rgarrow(struct gadget *g, int wd)
{
	int x = g->sbar.sbar_rect.x + g->sbar.sbar_rect.w;
	int w = g->sbar.sbar_rect.x;
	int h = g->rect.h;
	win_color hi1;
	win_color sh1;
	win_color hi2;
	win_color sh2;
	win_color bg;
	win_color fg;
	int x0, y0;
	int aw;
	int i;
	
	hi1 = wc_get(WC_HIGHLIGHT1);
	sh1 = wc_get(WC_SHADOW1);
	hi2 = wc_get(WC_HIGHLIGHT2);
	sh2 = wc_get(WC_SHADOW2);
	
	if (g->sbar.rgarrow_active)
	{
		fg = wc_get(WC_SEL_FG);
		bg = wc_get(WC_SEL_BG);
	}
	else
	{
		fg = wc_get(WC_WIN_FG);
		bg = wc_get(WC_WIN_BG);
	}
	
	win_rect(wd, bg, x, 0, w, h);
	
	win_hline(wd, hi1, x,	      0,     w);
	win_hline(wd, sh1, x,	      h - 1, w);
	win_vline(wd, hi1, x,	      0,     h);
	win_vline(wd, sh1, x + w - 1, 0,     h);
	
	win_hline(wd, hi2, x + 1,     1,     w - 2);
	win_hline(wd, sh2, x + 1,     h - 2, w - 2);
	win_vline(wd, hi2, x + 1,     1,     h - 2);
	win_vline(wd, sh2, x + w - 2, 1,     h - 2);
	
	y0 = g->rect.h / 2;
	aw = w / 4;
	x0 = x + (w + aw) / 2;
	for (i = 0; i < aw; i++)
		win_vline(wd, fg, x0 - i, y0 - i, i * 2);
}

static void hsbar_redraw(struct gadget *g, int wd)
{
	win_color sbox;
	win_color hi1;
	win_color sh1;
	win_color hi2;
	win_color sh2;
	win_color bg;
	win_color fg;
	int x0, x1;
	
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
	
	hsbar_make_rects(g);
	hsbar_lfarrow(g, wd);
	hsbar_rgarrow(g, wd);
	
	x0 = g->rect.h;
	x1 = g->sbar.sbox_rect.x;
	win_rect(wd, bg, x0, 0, x1 - x0, g->rect.h);
	
	x0 = g->sbar.sbox_rect.x + g->sbar.sbox_rect.w;
	x1 = g->rect.w - g->rect.h;
	win_rect(wd, bg, x0, 0, x1 - x0, g->rect.h);
	
	win_rect(wd, sbox, g->sbar.sbox_rect.x,
			   g->sbar.sbox_rect.y,
			   g->sbar.sbox_rect.w,
			   g->sbar.sbox_rect.h);
	
	win_vline(wd, hi1, g->sbar.sbox_rect.x,
			   g->sbar.sbox_rect.y,
			   g->sbar.sbox_rect.h);
	win_vline(wd, sh1, g->sbar.sbox_rect.x + g->sbar.sbox_rect.w - 1,
			   g->sbar.sbox_rect.y,
			   g->sbar.sbox_rect.h);
	win_hline(wd, hi1, g->sbar.sbox_rect.x,
			   g->sbar.sbox_rect.y,
			   g->sbar.sbox_rect.w);
	win_hline(wd, sh1, g->sbar.sbox_rect.x,
			   g->sbar.sbox_rect.y + g->sbar.sbox_rect.h - 1,
			   g->sbar.sbox_rect.w);
	
	win_vline(wd, hi2, g->sbar.sbox_rect.x + 1,
			   g->sbar.sbox_rect.y + 1,
			   g->sbar.sbox_rect.h - 2);
	win_vline(wd, sh2, g->sbar.sbox_rect.x + g->sbar.sbox_rect.w - 2,
			   g->sbar.sbox_rect.y + 1,
			   g->sbar.sbox_rect.h - 2);
	win_hline(wd, hi2, g->sbar.sbox_rect.x + 1,
			   g->sbar.sbox_rect.y + 1,
			   g->sbar.sbox_rect.w - 2);
	win_hline(wd, sh2, g->sbar.sbox_rect.x + 1,
			   g->sbar.sbox_rect.y + g->sbar.sbox_rect.h - 2,
			   g->sbar.sbox_rect.w - 2);
	return;
defunct:
	win_rect(wd, bg, 0, 0, g->rect.w, g->rect.h);
}

static void hsbar_ptr_move(struct gadget *g, int x, int y)
{
	int pos;
	
	if (g->sbar.sbox_active)
	{
		pos  = x - g->sbar.sbar_rect.x;
		pos *= g->sbar.limit;
		pos /= g->sbar.sbar_rect.w;
		
		if (pos < 0)
			pos = 0;
		if (pos >= g->sbar.limit)
			pos = g->sbar.limit - 1;
		
		hsbar_set_pos(g, pos);
	}
}

static void hsbar_ptr_down(struct gadget *g, int x, int y, int button)
{
	if (g->read_only)
		return;
	
	if (x < g->sbar.sbar_rect.x)
	{
		hsbar_set_pos(g, g->sbar.pos - g->sbar.step);
		tmr_reset(g->sbar.timer, &hsbar_delay, &hsbar_repeat);
		tmr_start(g->sbar.timer);
		g->sbar.lfarrow_active = 1;
		gadget_redraw(g);
	}
	
	if (x >= g->sbar.sbar_rect.x + g->sbar.sbar_rect.w)
	{
		hsbar_set_pos(g, g->sbar.pos + g->sbar.step);
		tmr_reset(g->sbar.timer, &hsbar_delay, &hsbar_repeat);
		tmr_start(g->sbar.timer);
		g->sbar.rgarrow_active = 1;
		gadget_redraw(g);
	}
	
	if (x >= g->sbar.sbox_rect.x &&
	    x <  g->sbar.sbox_rect.x + g->sbar.sbox_rect.w)
		g->sbar.sbox_active = 1;
}

static void hsbar_ptr_up(struct gadget *g, int x, int y, int button)
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

static void hsbar_timer_cb(void *cx)
{
	struct gadget *g = cx;
	
	if (g->sbar.uparrow_active)
		hsbar_set_pos(g, g->sbar.pos - g->sbar.step);
	if (g->sbar.dnarrow_active)
		hsbar_set_pos(g, g->sbar.pos + g->sbar.step);
}

static void hsbar_remove(struct gadget *g)
{
	tmr_free(g->sbar.timer);
}

struct gadget *hsbar_creat(struct form *form, int x, int y, int w, int h)
{
	struct gadget *g;
	int se;
	
	g = gadget_creat(form, x, y, w, h);
	if (!g)
		return NULL;
	
	g->sbar.timer = tmr_creat(&hsbar_delay, &hsbar_repeat, hsbar_timer_cb, g, 0);
	if (!g->sbar.timer)
	{
		se = _get_errno();
		gadget_remove(g);
		_set_errno(se);
		return NULL;
	}
	g->sbar.step = 1;
	
	g->remove   = hsbar_remove;
	g->redraw   = hsbar_redraw;
	g->ptr_move = hsbar_ptr_move;
	g->ptr_down = hsbar_ptr_down;
	g->ptr_up   = hsbar_ptr_up;

	hsbar_make_rects(g);
	gadget_redraw(g);
	return g;
}

int hsbar_set_step(struct gadget *g, int step)
{
	if (step < 1)
	{
		_set_errno(EINVAL);
		return -1;
	}
	g->sbar.step = step;
	return 0;
}

int hsbar_set_limit(struct gadget *g, int limit)
{
	int prev = g->sbar.limit;
	
	if (limit < 0)
		g->sbar.limit = 0;
	else
		g->sbar.limit = limit;
	
	if (prev != g->sbar.limit)
	{
		hsbar_make_rects(g);
		gadget_redraw(g);
	}
	
	return 0;
}

int hsbar_set_pos(struct gadget *g, int pos)
{
	int prev = g->sbar.pos;
	
	g->sbar.pos = pos;
	if (g->sbar.pos >= g->sbar.limit)
		g->sbar.pos = g->sbar.limit - 1;
	if (g->sbar.pos < 0)
		g->sbar.pos = 0;
	
	if (prev != g->sbar.pos)
	{
		hsbar_make_rects(g);
		gadget_redraw(g);
		if (g->sbar.move)
			g->sbar.move(g, g->sbar.pos);
	}
	
	return 0;
}

int hsbar_get_pos(struct gadget *g)
{
	return g->sbar.pos;
}

int hsbar_on_move(struct gadget *g, sbar_move_cb *proc)
{
	g->sbar.move = proc;
	return 0;
}
