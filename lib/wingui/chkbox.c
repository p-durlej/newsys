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
#include <string.h>
#include <stdlib.h>

static void chkbox_remove(struct gadget *g)
{
	free(g->text);
}

static void chkbox_redraw(struct gadget *g, int wd)
{
	win_color text_fg;
	win_color text_bg;
	win_color win_bg;
	win_color sh1;
	win_color sh2;
	win_color hi1;
	win_color hi2;
	win_color sel;
	win_color bg;
	int font_w;
	int font_h;
	
	win_text_size(WIN_FONT_DEFAULT, &font_w, &font_h, "X");
	
	win_bg	= wc_get(WC_WIN_BG);
	bg	= wc_get(WC_BG);
	sel	= wc_get(WC_SEL_BG);
	hi1	= wc_get(WC_HIGHLIGHT1);
	hi2	= wc_get(WC_HIGHLIGHT2);
	sh1	= wc_get(WC_SHADOW1);
	sh2	= wc_get(WC_SHADOW2);
	
	if (g->form->focus == g)
	{
		text_fg = wc_get(WC_SEL_FG);
		text_bg = wc_get(WC_SEL_BG);
	}
	else
	{
		text_fg = wc_get(WC_WIN_FG);
		text_bg = wc_get(WC_WIN_BG);
	}
	
	win_rect(wd, text_bg, 0, 0, g->rect.w, g->rect.h);
	win_text(wd, text_fg, font_h + 2, 0, g->text);
	win_vline(wd, win_bg, font_h, 0, font_h);
	
	win_rect(wd, bg, 1, 1, font_h - 2, font_h - 2);
	win_hline(wd, sh2, 0, 0, font_h);
	win_hline(wd, hi2, 0, font_h-1, font_h);
	win_vline(wd, sh2, 0, 0, font_h);
	win_vline(wd, hi2, font_h - 1, 0, font_h);
	
	win_hline(wd, sh1, 1, 1, font_h - 2);
	win_hline(wd, hi1, 1, font_h - 2, font_h - 2);
	win_vline(wd, sh1, 1, 1, font_h - 2);
	win_vline(wd, hi1, font_h - 2, 1, font_h - 2);
	
	if (g->chkbox.state)
		win_rect(wd, sel, 3, 3, font_h - 6, font_h - 6);
}

static void chkbox_ptr_down(struct gadget *g, int x, int y, int button)
{
	g->chkbox.ptr_down++;
	if (g->chkbox.ptr_down == 1)
		gadget_redraw(g);
}

static void chkbox_ptr_up(struct gadget *g, int x, int y, int button)
{
	g->chkbox.ptr_down--;
	
	if (!g->chkbox.ptr_down && !g->read_only && x < g->rect.w && y < g->rect.h)
	{
		if (g->chkbox.group)
			chkbox_set_state(g, 1);
		else
			chkbox_set_state(g, !g->chkbox.state);
		gadget_redraw(g);
	}
}

static void chkbox_key_down(struct gadget *g, unsigned ch, unsigned shift)
{
	if (g->read_only)
		return;
	if (ch == ' ' || ch == '\n')
	{
		if (g->chkbox.group)
			chkbox_set_state(g, 1);
		else
			chkbox_set_state(g, !g->chkbox.state);
		gadget_redraw(g);
	}
}

static void chkbox_focus(struct gadget *g)
{
	gadget_redraw(g);
}

struct gadget *chkbox_creat(struct form *form, int x, int y, const char *text)
{
	struct gadget *g;
	int w, h;
	
	if (win_text_size(WIN_FONT_DEFAULT, &w, &h, text))
		return NULL;
	
	g = gadget_creat(form, x, y, w + h + 2, h);
	if (!g)
		return NULL;
	
	g->remove   = chkbox_remove;
	g->redraw   = chkbox_redraw;
	g->key_down = chkbox_key_down;
	g->ptr_down = chkbox_ptr_down;
	g->ptr_up   = chkbox_ptr_up;
	g->focus    = chkbox_focus;
	
	g->text = strdup(text);
	if (!g->text)
	{
		gadget_remove(g);
		return NULL;
	}
	g->want_focus = 1;
	
	gadget_redraw(g);
	return g;
}

void chkbox_set_state(struct gadget *g, int state)
{
	struct gadget *i;
	
	if (state && g->chkbox.group)
	{
		i = g->form->gadget;
		while (i)
		{
			if (i->redraw == chkbox_redraw && i->chkbox.group == g->chkbox.group)
				chkbox_set_state(i, 0);
			i = i->next;
		}
	}
	if (g->chkbox.state != state)
	{
		g->chkbox.state = state;
		gadget_redraw(g);
	}
}

int chkbox_get_state(struct gadget *g)
{
	return g->chkbox.state;
}

void chkbox_set_group(struct gadget *g, int group_nr)
{
	g->chkbox.group = group_nr;
}
