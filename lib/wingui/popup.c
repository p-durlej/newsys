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

#include <wingui_metrics.h>
#include <wingui_color.h>
#include <wingui_form.h>
#include <wingui_menu.h>
#include <wingui_bell.h>
#include <stdlib.h>
#include <errno.h>

static void popup_focus(struct gadget *g)
{
}

static void popup_remove(struct gadget *g)
{
	free(g->popup.items);
}

static void popup_redraw(struct gadget *g, int wd)
{
	win_color hi1 = wc_get(WC_HIGHLIGHT1);
	win_color sh1 = wc_get(WC_SHADOW1);
	win_color bg = wc_get(WC_WIN_BG);
	win_color fg = wc_get(WC_WIN_FG);
	int i = g->popup.item_index;
	int tl;
	
	tl = wm_get(WM_THIN_LINE);
	
	win_rect(wd, bg, 0, 0, g->rect.w, g->rect.h);
	if (g->popup.pressed)
		form_draw_frame3d(wd, 0, 0, g->rect.w, g->rect.h, sh1, bg, hi1, bg);
	
	if (i >= 0 && i < g->popup.item_count)
		win_text(wd, fg, g->rect.h, 2 * tl, g->popup.items[i]);
	
	for (i = 0; i < 4; i++)
		win_hline(wd, fg, g->rect.h / 2 + i - 4, g->rect.h / 2 + i - 2, (4 - i) * 2);
}

static void popup_menu_action(struct menu_item *mi)
{
	struct gadget *g = mi->p_data;
	int i = mi->l_data;
	
	popup_set_index(g, i);
}

static void popup_ptr_down(struct gadget *g, int x, int y, int button)
{
	g->popup.pressed++;
	if (g->popup.pressed == 1)
		gadget_redraw(g);
}

static void popup_ptr_up(struct gadget *g, int x, int y, int button)
{
	struct menu_item *mi;
	struct menu *m;
	int i;
	
	g->popup.pressed--;
	gadget_redraw(g);
	if (!g->popup.pressed && x < g->rect.w && y < g->rect.h)
	{
		m = menu_creat();
		if (!m)
		{
			wb(WB_ERROR);
			return;
		}
		for (i = 0; i < g->popup.item_count; i++)
		{
			mi = menu_newitem(m, g->popup.items[i], popup_menu_action);
			if (!mi)
			{
				menu_free(m);
				wb(WB_ERROR);
				return;
			}
			mi->p_data = g;
			mi->l_data = i;
		}
		menu_popup_l(m,
			     g->form->win_rect.x + g->rect.x,
			     g->form->win_rect.y + g->rect.y + g->rect.h,
			     g->form->layer);
		menu_free(m);
	}
}

struct gadget *popup_creat(struct form *form, int x, int y)
{
	struct gadget *g;
	int w, h;
	int tl;
	
	tl = wm_get(WM_THIN_LINE);
	
	win_text_size(WIN_FONT_DEFAULT, &w, &h, "X");
	
	g = gadget_creat(form, x, y, h + 4 * tl, h + 4 * tl);
	if (!g)
		return NULL;
	
	g->popup.item_index = -1;
	g->popup.item_count = 0;
	g->popup.items	    = 0;
	g->popup.pressed    = 0;
	
	g->redraw   = popup_redraw;
	g->focus    = popup_focus;
	g->remove   = popup_remove;
	g->ptr_down = popup_ptr_down;
	g->ptr_up   = popup_ptr_up;
	
	gadget_redraw(g);
	return g;
}

int popup_newitem(struct gadget *g, const char *text)
{
	const char **p;
	
	p = realloc(g->popup.items, sizeof(char *) * (g->popup.item_count + 1));
	if (!p)
		return -1;
	p[g->popup.item_count++] = text;
	g->popup.items = p;
	return 0;
}

int popup_clear(struct gadget *g)
{
	g->popup.item_count = 0;
	free(g->popup.items);
	return 0;
}

int popup_get_index(struct gadget *g)
{
	return g->popup.item_index;
}

int popup_set_index(struct gadget *g, int index)
{
	int w, h;
	int tl;
	
	if (index < 0 || index >= g->popup.item_count)
	{
		_set_errno(EINVAL);
		return -1;
	}
	
	if (index == g->popup.item_index)
		return 0;
	
	tl = wm_get(WM_THIN_LINE);
	
	win_text_size(WIN_FONT_DEFAULT, &w, &h, g->popup.items[index]);
	w += h + 6 * tl;
	h += 4 * tl;
	
	g->popup.item_index = index;
	gadget_resize(g, w, h);
	gadget_redraw(g);
	if (g->popup.select)
		g->popup.select(g, index);
	
	return 0;
}

const char *popup_get_text(struct gadget *g, int index)
{
	return g->popup.items[index];
}

int popup_on_select(struct gadget *g, popup_select_cb *proc)
{
	g->popup.select = proc;
	return 0;
}
