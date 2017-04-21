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

#include <wingui_cgadget.h>
#include <wingui_color.h>
#include <wingui_form.h>
#include <wingui.h>
#include <string.h>
#include <stdlib.h>

static void label_redraw(struct gadget *g, int wd)
{
	win_color fg;
	win_color bg;
	
	fg = wc_get(WC_WIN_FG);
	bg = wc_get(WC_WIN_BG);
	
	if (!g->no_bg)
		win_rect(wd, bg, 0, 0, g->rect.w, g->rect.h);
	win_text(wd, fg, 0, 0, g->text);
}

static void label_remove(struct gadget *g)
{
	free(g->text);
}

struct gadget *label_creat(struct form *form, int x, int y, const char *text)
{
	struct gadget *g;
	char *dtext;
	int w, h;
	
	dtext = strdup(text);
	if (!dtext)
		return NULL;
	
	if (win_text_size(WIN_FONT_DEFAULT, &w, &h, text))
		return NULL;
	
	g = gadget_creat(form, x, y, w, h);
	if (!g)
		return NULL;
	
	g->remove = label_remove;
	g->redraw = label_redraw;
	g->text	  = dtext;
	gadget_redraw(g);
	return g;
}

int label_set_text(struct gadget *g, const char *text)
{
	char *dtext;
	int w, h;
	
	dtext = strdup(text);
	if (!dtext)
		return -1;
	
	free(g->text);
	g->text = dtext;
	
	if (win_text_size(WIN_FONT_DEFAULT, &w, &h, dtext))
		return -1;
	
	if (w == g->rect.w && h == g->rect.h)
	{
		gadget_redraw(g);
		return 0;
	}
	gadget_resize(g, w, h);
	return 0;
}
