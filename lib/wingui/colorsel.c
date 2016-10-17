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
#include <wingui_dlg.h>
#include <stdlib.h>

static void colorsel_popup(struct gadget *g)
{
	if (g->read_only)
		return;
	
	dlg_color(g->form, "Choose color", &g->colorsel.color);
	gadget_redraw(g);
	if (g->colorsel.change)
		g->colorsel.change(g, &g->colorsel.color);
}

static void colorsel_redraw(struct gadget *g, int wd)
{
	win_color hi1, hi2;
	win_color sh1, sh2;
	win_color bg;
	win_color c;
	int w, h;
	
	hi1 = wc_get(WC_HIGHLIGHT1);
	hi2 = wc_get(WC_HIGHLIGHT2);
	sh1 = wc_get(WC_SHADOW1);
	sh2 = wc_get(WC_SHADOW2);
	bg  = wc_get(WC_BG);
	
	w = g->rect.w;
	h = g->rect.h;
	
	win_rgb2color(&c, g->colorsel.color.r,
			  g->colorsel.color.g,
			  g->colorsel.color.b);
	win_rect(wd, bg, 0, 0, w, h);
	win_hline(wd, sh1, 0, 0, w);
	win_hline(wd, sh2, 1, 1, w - 2);
	win_vline(wd, sh1, 0, 0, h);
	win_vline(wd, sh2, 1, 1, h - 2);
	win_hline(wd, hi1, 0, h - 1, w);
	win_hline(wd, hi2, 1, h - 2, w - 2);
	win_vline(wd, hi1, w - 1, 0, h);
	win_vline(wd, hi2, w - 2, 1, h - 2);
	win_rect(wd, c, 4, 4, g->rect.w - 8, g->rect.h - 8);
}

static void colorsel_key_down(struct gadget *g, unsigned ch, unsigned shift)
{
	if (ch == '\n')
		colorsel_popup(g);
}

static void colorsel_ptr_up(struct gadget *g, int x, int y, int button)
{
	if (x < 0 || x >= g->rect.w)
		return;
	if (y < 0 || y >= g->rect.h)
		return;
	colorsel_popup(g);
}

struct gadget *colorsel_creat(struct form *f, int x, int y, int w, int h)
{
	struct gadget *g;
	
	g = gadget_creat(f, x, y, w, h);
	g->want_focus = 1;
	g->key_down = colorsel_key_down;
	g->ptr_up   = colorsel_ptr_up;
	g->redraw   = colorsel_redraw;
	gadget_redraw(g);
	return g;
}

void colorsel_on_change(struct gadget *g, colorsel_change_cb *proc)
{
	g->colorsel.change = proc;
}

void colorsel_set(struct gadget *g, const struct win_rgba *buf)
{
	g->colorsel.color = *buf;
	gadget_redraw(g);
}

void colorsel_get(struct gadget *g, struct win_rgba *buf)
{
	*buf = g->colorsel.color;
}
