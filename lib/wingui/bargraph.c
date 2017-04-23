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

#include <priv/wingui_form.h>
#include <wingui_metrics.h>
#include <wingui_cgadget.h>
#include <wingui_color.h>
#include <wingui_form.h>
#include <wingui.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void bargraph_redraw(struct gadget *g, int wd)
{
	const char *minl, *maxl;
	win_color hi1, sh1;
	win_color hi2, sh2;
	win_color bg, fg;
	win_color bar;
	char buf[64];
	int tw0, th0;
	int tw1, th1;
	int m0, m1;
	int w = 0;
	int tl;
	
	tl = wm_get(WM_THIN_LINE);
	
	bg  = wc_get(WC_BG);
	fg  = wc_get(WC_FG);
	hi1 = wc_get(WC_HIGHLIGHT1);
	sh1 = wc_get(WC_SHADOW1);
	hi2 = wc_get(WC_HIGHLIGHT2);
	sh2 = wc_get(WC_SHADOW2);
	bar = fg;
	
	minl = g->bargraph.min_label;
	maxl = g->bargraph.max_label;
	
	if (!minl)
		minl = "0";
	
	if (!maxl)
	{
		sprintf(buf, "%i", g->bargraph.limit);
		maxl = buf;
	}
	
	win_text_size(WIN_FONT_DEFAULT, &tw0, &th0, minl);
	win_text_size(WIN_FONT_DEFAULT, &tw1, &th1, maxl);
	
	m0 = (tw0 ? 5 : 4) * tl;
	m1 = (tw1 ? 6 : 4) * tl;
	
	if (g->bargraph.limit > 0 && g->bargraph.value <= g->bargraph.limit)
	{
		w  = g->bargraph.value * (g->rect.w - tw1 - tw0 - m0 - m1);
		w /= g->bargraph.limit;
	}
	
	win_rect(wd,  bg,  0,		  0,		 g->rect.w, g->rect.h);
	win_rect(wd,  bar, tw0 + m0, 4 * tl, w, g->rect.h - 8 * tl);
	
	form_draw_frame3d(wd, 0, 0, g->rect.w, g->rect.h, sh1, sh2, hi1, hi2);
	
	win_text(wd, fg, 4 * tl,		   (g->rect.h - th0) / 2, minl);
	win_text(wd, fg, g->rect.w - tw1 - 4 * tl, (g->rect.h - th1) / 2, maxl);
}

struct gadget *bargraph_creat(struct form *f, int x, int y, int w, int h)
{
	struct gadget *g;
	
	g = gadget_creat(f, x, y, w, h);
	if (g == NULL)
		return NULL;
	
	g->redraw = bargraph_redraw;
	g->bargraph.limit = 100;
	return g;
}

void bargraph_set_limit(struct gadget *g, int limit)
{
	g->bargraph.limit = limit;
	gadget_redraw(g);
}

void bargraph_set_value(struct gadget *g, int value)
{
	g->bargraph.value = value;
	gadget_redraw(g);
}

void bargraph_set_labels(struct gadget *g, const char *min, const char *max)
{
	free(g->bargraph.min_label);
	free(g->bargraph.max_label);
	
	g->bargraph.min_label = min ? strdup(min) : NULL;
	g->bargraph.max_label = max ? strdup(max) : NULL;
}
