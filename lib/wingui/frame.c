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
#include <wingui_color.h>
#include <wingui_form.h>
#include <string.h>
#include <stdlib.h>

static void frame_redraw(struct gadget *g, int wd)
{
	win_color h1 = wc_get(WC_HIGHLIGHT1);
	win_color s1 = wc_get(WC_SHADOW1);
	win_color fg = wc_get(WC_WIN_FG);
	win_color bg = wc_get(WC_WIN_BG);
	int w, h;
	
	if (g->text)
		win_text_size(WIN_FONT_DEFAULT, &w, &h, g->text);
	else
		w = h = 0;
	h &= ~1;
	
	form_draw_frame3d(wd, h / 2, h / 2, g->rect.w - h, g->rect.h - h, s1, h1, h1, s1);
	
	if (g->text)
		win_btext(wd, bg, fg, h, 0, g->text);
}

struct gadget *frame_creat(struct form *f, int x, int y, int w, int h, const char *text)
{
	struct gadget *g;
	char *t = NULL;
	
	if (text && !(t = strdup(text)))
		return NULL;
	
	g = gadget_creat(f, x, y, w, h);
	if (!g)
	{
		free(t);
		return NULL;
	}
	g->redraw = frame_redraw;
	g->no_bg  = 1;
	g->text   = t;
	gadget_redraw(g);
	return g;
}

void frame_set_text(struct gadget *g, const char *text)
{
	char *s = NULL;
	
	if (text)
		s = strdup(text);
	
	free(g->text);
	g->text = s;
	
	gadget_redraw(g);
}
