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
#include <wingui_metrics.h>
#include <wingui_color.h>
#include <wingui_form.h>
#include <wingui.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define LIST_FRAME_WIDTH	(wm_get(WM_THIN_LINE) * 2)

static void list_xyitem(struct gadget *g, int *index, int x, int y);
static void list_ptr_down(struct gadget *g, int x, int y, int button);
static void list_ptr_up(struct gadget *g, int x, int y, int button);
static void list_ptr_move(struct gadget *g, int x, int y);
static void list_key_down(struct gadget *g, unsigned ch, unsigned shift);
static void list_remove(struct gadget *g);
static void list_draw_item_text(struct gadget *g, int index, int wd, int x, int y, int w, int h);
static void list_redraw_item(struct gadget *g, int wd, int index, int setclip);
static void list_update_vsbar(struct gadget *g);
static void list_redraw_frame(struct gadget *g, int wd);
static void list_redraw(struct gadget *g, int wd);
static void list_resize(struct gadget *g, int w, int h);
static void list_move(struct gadget *g, int x, int y);
static void list_focus(struct gadget *g);
static void list_freei(struct gadget *g);

#define abs(x)	((x) >= 0 ? (x) : -(x))

static void list_xyitem(struct gadget *g, int *index, int x, int y)
{
	if (g->list.flags & LIST_FRAME)
		y -= LIST_FRAME_WIDTH;
	
	*index = g->list.topmost_index + (y - 1) / g->list.item_height;
	if (*index >= g->list.item_count)
		*index = -1;
	if (*index < -1)
		*index = -1;
}

static void list_ptr_down(struct gadget *g, int x, int y, int button)
{
	int prev = g->list.item_index;
	int i;
	
	if (!g->read_only)
	{
		list_xyitem(g, &i, x, y);
		if (prev != i)
		{
			list_set_index(g, i);
			if (g->list.select)
				g->list.select(g, i, button);
		}
	}
}

static void list_ptr_up(struct gadget *g, int x, int y, int button)
{
	int dcd = wm_get(WM_DOUBLECLICK);
	int i;
	
	i = g->list.item_index;
	
	if (x >= 0 && abs(x - g->list.last_x) <= dcd &&
	    y >= 0 && abs(y - g->list.last_y) <= dcd &&
	    i >= 0 && g->list.request)
	{
		g->list.request(g, i, button);
		g->list.last_x = -1;
		g->list.last_y = -1;
	}
	else
	{
		g->list.last_x = x;
		g->list.last_y = y;
	}
}

static void list_key_down(struct gadget *g, unsigned ch, unsigned shift)
{
	if (g->read_only)
		return;
	switch (ch)
	{
	case '\n':
		if (g->list.item_index >= 0 && g->list.request)
			g->list.request(g, g->list.item_index, 0);
		break;
	case WIN_KEY_UP:
		if (g->list.item_index > 0)
		{
			if (g->list.item_index >= g->list.item_count)
				list_set_index(g, g->list.item_count - 1);
			else
				list_set_index(g, g->list.item_index - 1);
		}
		break;
	case WIN_KEY_DOWN:
		if (g->list.item_index < g->list.item_count - 1)
		{
			if (g->list.item_count < 0)
				list_set_index(g, 0);
			else
				list_set_index(g, g->list.item_index + 1);
		}
		break;
	}
}

static void list_remove(struct gadget *g)
{
	gadget_remove(g->list.vsbar);
	g->list.vsbar = NULL;
	list_freei(g);
}

static void list_draw_item_text(struct gadget *g, int index, int wd, int x, int y, int w, int h)
{
	int sbw = wm_get(WM_SCROLLBAR);
	win_color fg;
	win_color bg;
	
	if (g->list.item_index != index)
	{
		fg = wc_get(WC_FG);
		bg = wc_get(WC_BG);
	}
	else
	{
		if (g->form->focus == g)
		{
			fg = wc_get(WC_SEL_FG);
			bg = wc_get(WC_SEL_BG);
		}
		else
		{
			fg = wc_get(WC_NFSEL_FG);
			bg = wc_get(WC_NFSEL_BG);
		}
	}
	
	win_rect(wd, bg, x, y, w, g->list.item_height);
	win_text(wd, fg, x + 1, y, g->list.items[index]);
}

static void list_redraw_item(struct gadget *g, int wd, int index, int setclip)
{
	int sbw = wm_get(WM_SCROLLBAR);
	int x, y, w, h;
	
	if (index < 0 || index >= g->list.item_count)
		return;
	if (index < g->list.topmost_index)
		return;
	if (!g->visible)
		return;
	
	x = 0;
	y = (index - g->list.topmost_index) * g->list.item_height;
	w = g->rect.w;
	h = g->rect.h - y;
	
	if (y > g->rect.h)
		return;
	
	if (setclip)
	{
		win_clip(wd, g->rect.x, g->rect.y, g->rect.w, g->rect.h,
			     g->rect.x, g->rect.y);
		win_set_font(wd, g->font);
	}
	
	win_paint();
	if (g->list.flags & LIST_FRAME)
	{
		y += LIST_FRAME_WIDTH;
		x += LIST_FRAME_WIDTH;
		w -= LIST_FRAME_WIDTH * 2;
		h -= LIST_FRAME_WIDTH * 2;
	}
	if (g->list.flags & LIST_VSBAR)
		w -= sbw;
	
	if (h > g->list.item_height)
		h = g->list.item_height;
	
	gadget_clip(g, x, y, w, h, 0, 0);
	g->list.drawitem(g, index, wd, x, y, w, g->list.item_height);
	
	win_end_paint();
}

static void list_update_vsbar(struct gadget *g)
{
	int l;
	int h;
	
	h = g->rect.h;
	if (g->list.flags & LIST_FRAME)
		h -= LIST_FRAME_WIDTH * 2;
	l = g->list.item_count - h / g->list.item_height + 1;
	
	if (l <= g->list.topmost_index)
		l = g->list.topmost_index + 1;
	vsbar_set_limit(g->list.vsbar, l);
	vsbar_set_pos(g->list.vsbar, g->list.topmost_index);
}

static void list_redraw_frame(struct gadget *g, int wd)
{
	win_color hi1;
	win_color sh1;
	win_color hi2;
	win_color sh2;
	
	if (g->list.flags & LIST_FRAME)
	{
		hi1 = wc_get(WC_HIGHLIGHT1);
		sh1 = wc_get(WC_SHADOW1);
		hi2 = wc_get(WC_HIGHLIGHT2);
		sh2 = wc_get(WC_SHADOW2);
		
		form_draw_frame3d(wd, 0, 0, g->rect.w, g->rect.h, sh2, sh1, hi2, hi1);
	}
}

static void list_redraw(struct gadget *g, int wd)
{
	int sbw = wm_get(WM_SCROLLBAR);
	win_color hi;
	win_color sh;
	win_color bg;
	int x, y, w, h;
	int count;
	int ih;
	int i;
	
	hi = wc_get(WC_HIGHLIGHT1);
	sh = wc_get(WC_SHADOW1);
	bg = wc_get(WC_BG);
	
	count = (g->rect.h + g->list.item_height - 1) / g->list.item_height;
	if (count > g->list.item_count - g->list.topmost_index)
		count = g->list.item_count - g->list.topmost_index;
	
	x = 0;
	y = 0;
	w = g->rect.w;
	h = g->rect.h;
	
	if (g->list.flags & LIST_FRAME)
	{
		x += LIST_FRAME_WIDTH;
		y += LIST_FRAME_WIDTH;
		w -= LIST_FRAME_WIDTH * 2;
		h -= LIST_FRAME_WIDTH * 2;
	}
	if (g->list.flags & LIST_VSBAR)
		w -= sbw;
	ih = count * g->list.item_height;
	
	win_paint();
	if (ih < h)
		win_rect(wd, bg, x, y + ih, w, h - ih);
	
	list_redraw_frame(g, wd);
	for (i = 0; i < count; i++)
		list_redraw_item(g, wd, g->list.topmost_index + i, 0);
	win_end_paint();
}

static void list_resize(struct gadget *g, int w, int h)
{
	int sbw = wm_get(WM_SCROLLBAR);
	int sbx, sby;
	
	if (g->list.flags & LIST_FRAME)
	{
		sbx  = g->rect.x + g->rect.w - sbw - LIST_FRAME_WIDTH;
		sby  = g->rect.y + LIST_FRAME_WIDTH;
		sbx -= g->form->workspace_rect.x;
		sby -= g->form->workspace_rect.y;
		
		gadget_resize(g->list.vsbar, sbw, h - LIST_FRAME_WIDTH * 2);
		gadget_move(g->list.vsbar, sbx, sby);
	}
	else
	{
		sbx  = g->rect.x + g->rect.w - sbw;
		sby  = g->rect.y;
		sbx -= g->form->workspace_rect.x;
		sby -= g->form->workspace_rect.y;
		
		gadget_resize(g->list.vsbar, sbw, h);
		gadget_move(g->list.vsbar, sbx, sby);
	}
	
	list_update_vsbar(g);
}

static void list_move(struct gadget *g, int x, int y)
{
	int sbw = wm_get(WM_SCROLLBAR);
	int sbx, sby;
	
	if (g->list.flags & LIST_FRAME)
	{
		sbx  = g->rect.x + g->rect.w - sbw - LIST_FRAME_WIDTH;
		sby  = g->rect.y + LIST_FRAME_WIDTH;
		sbx -= g->form->workspace_rect.x;
		sby -= g->form->workspace_rect.y;
		
		gadget_move(g->list.vsbar, sbx, sby);
	}
	else
	{
		sbx  = g->rect.x + g->rect.w - sbw;
		sby  = g->rect.y;
		sbx -= g->form->workspace_rect.x;
		sby -= g->form->workspace_rect.y;
		
		gadget_move(g->list.vsbar, sbx, sby);
	}
}

static void list_set_font(struct gadget *g, int ftd)
{
	int text_w, text_h;
	
	if (win_text_size(ftd, &text_w, &text_h, "X"))
		return;
	g->list.item_height = text_h;
}

static void list_focus(struct gadget *g)
{
	list_redraw_item(g, g->form->wd, g->list.item_index, 1);
}

static void list_vsbar_move(struct gadget *g, int newpos)
{
	list_scroll(g->p_data, newpos);
}

static void list_freei(struct gadget *g)
{
	const char **e;
	const char **p;
	
	if (g->list.drawitem == list_draw_item_text)
	{
		p = g->list.items;
		e = p + g->list.item_count;
		for (; p < e; p++)
			free((void *)*p);
	}
	
	free(g->list.items);
	g->list.items = NULL;
	g->list.item_count = 0;
}

struct gadget *list_creat(struct form *form, int x, int y, int w, int h)
{
	struct gadget *sb;
	struct gadget *g;
	int text_w;
	int text_h;
	int sbw;
	int se;
	
	if (win_text_size(WIN_FONT_DEFAULT, &text_w, &text_h, "X"))
		return NULL;
	sbw = wm_get(WM_SCROLLBAR);
	
	g = gadget_creat(form, x, y, w, h);
	if (!g)
		return NULL;
	
	g->list.item_height	= text_h;
	g->list.drawitem	= list_draw_item_text;
	g->list.last_x		= -1;
	g->list.last_y		= -1;
	g->want_focus		= 1;
	g->remove		= list_remove;
	g->redraw		= list_redraw;
	g->resize		= list_resize;
	g->move			= list_move;
	g->set_font		= list_set_font;
	g->ptr_down		= list_ptr_down;
	g->ptr_up		= list_ptr_up;
	g->key_down		= list_key_down;
	g->focus		= list_focus;
	gadget_redraw(g);
	
	sb = vsbar_creat(form, x + w - sbw, y, sbw, h);
	if (!sb)
	{
		se = _get_errno();
		gadget_remove(g);
		_set_errno(se);
		return NULL;
	}
	vsbar_on_move(sb, list_vsbar_move);
	gadget_hide(sb);
	sb->is_internal = 1;
	sb->p_data	= g;
	g->list.vsbar	= sb;
	return g;
}

void list_set_flags(struct gadget *g, int flags)
{
	int sbw = wm_get(WM_SCROLLBAR);
	int sbx, sby;
	
	if (flags & LIST_VSBAR)
		gadget_show(g->list.vsbar);
	else
		gadget_hide(g->list.vsbar);
	
	if (flags & LIST_FRAME)
	{
		sbx  = g->rect.x + g->rect.w - sbw - LIST_FRAME_WIDTH;
		sby  = g->rect.y + LIST_FRAME_WIDTH;
		sbx -= g->form->workspace_rect.x;
		sby -= g->form->workspace_rect.y;
		
		gadget_resize(g->list.vsbar, sbw, g->rect.h - LIST_FRAME_WIDTH * 2);
		gadget_move(g->list.vsbar, sbx, sby);
	}
	else
	{
		sbx  = g->rect.x + g->rect.w - sbw;
		sby  = g->rect.y;
		sbx -= g->form->workspace_rect.x;
		sby -= g->form->workspace_rect.y;
		
		gadget_resize(g->list.vsbar, sbw, g->rect.h);
		gadget_move(g->list.vsbar, sbx, sby);
	}
	
	g->list.flags = flags;
	gadget_redraw(g);
}

int list_newitem(struct gadget *g, const char *text)
{
	const char **newlist;
	const char *dtext;
	
	newlist = realloc(g->list.items, (g->list.item_count + 1) * sizeof *newlist);
	if (!newlist)
		return -1;
	
	if (g->list.drawitem == list_draw_item_text)
	{
		dtext = strdup(text);
		if (!dtext)
			return -1;
	}
	else
		dtext = text;
	
	newlist[g->list.item_count++] = dtext;
	g->list.items = newlist;
	
	win_clip(g->form->wd, g->rect.x, g->rect.y, g->rect.w, g->rect.h,
			      g->rect.x, g->rect.y);
	list_redraw_item(g, g->form->wd, g->list.item_count - 1, 1);
	list_update_vsbar(g);
	return 0;
}

void list_clear(struct gadget *g)
{
	list_freei(g);
	if (!(g->list.flags & LIST_DONT_CLEAR_POS))
	{
		g->list.item_index    = -1;
		g->list.topmost_index = 0;
	}
	list_update_vsbar(g);
	gadget_redraw(g);
}

int list_scroll(struct gadget *g, int topmost_index)
{
	if (topmost_index < 0)
	{
		_set_errno(EINVAL);
		return -1;
	}
	
	if (g->list.topmost_index != topmost_index)
	{
		g->list.topmost_index = topmost_index;
		
		list_update_vsbar(g);
		gadget_redraw(g);
		if (g->list.scroll)
			g->list.scroll(g, topmost_index);
	}
	return 0;
}

int list_get_index(struct gadget *g)
{
	return g->list.item_index;
}

int list_set_index(struct gadget *g, int index)
{
	int prev = g->list.item_index;
	int max;
	
	if (index >= g->list.item_count)
	{
		_set_errno(EINVAL);
		return -1;
	}
	
	if (g->list.flags & LIST_FRAME)
		max = (g->rect.h - LIST_FRAME_WIDTH) / g->list.item_height;
	else
		max = g->rect.h / g->list.item_height;
	
	if (index >= 0 && index < g->list.topmost_index)
		list_scroll(g, index);
	
	if (index >= max + g->list.topmost_index)
		list_scroll(g, index - max + 1);
	
	if (prev == index)
		return 0;
	
	g->list.item_index = index;
	list_redraw_item(g, g->form->wd, prev,	1);
	list_redraw_item(g, g->form->wd, index, 1);
	
	if (g->list.select)
		g->list.select(g, index, 0);
	return 0;
}

const char *list_get_text(struct gadget *g, int index)
{
	if (index < 0 || index >= g->list.item_count)
	{
		_set_errno(EINVAL);
		return NULL;
	}
	
	return g->list.items[index];
}

int list_on_request(struct gadget *g, list_request_cb *proc)
{
	g->list.request = proc;
	return 0;
}

int list_on_select(struct gadget *g, list_select_cb *proc)
{
	g->list.select = proc;
	return 0;
}

int list_on_scroll(struct gadget *g, list_scroll_cb *proc)
{
	g->list.scroll = proc;
	return 0;
}

int list_set_draw_cb(struct gadget *g, list_drawitem_cb *proc)
{
	if (!proc)
		g->list.drawitem = list_draw_item_text;
	else
		g->list.drawitem = proc;
	return 0;
}

int list_cmp(const void *s1, const void *s2)
{
	return strcmp(*(char **)s1, *(char **)s2);
}

void list_sort(struct gadget *g)
{
	if (g->list.item_count < 2)
		return;
	
	qsort(g->list.items, g->list.item_count, sizeof *g->list.items, list_cmp);
	g->list.topmost_index = 0;
	g->list.item_index = -1;
	gadget_redraw(g);
}
