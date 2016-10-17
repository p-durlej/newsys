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

#include <wingui_metrics.h>
#include <wingui_bitmap.h>
#include <wingui_color.h>
#include <wingui_form.h>
#include <wingui_menu.h>
#include <wingui.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define DEFAULT_ICON	"/lib/icons/file.pnm"

#define TEXT_BELOW_ICON	1
#define DOUBLE_CLICK	0

static void icbox_remove(struct gadget *g);
static int  icbox_defsizes(struct gadget *g);
static int  icbox_usizes(struct gadget *g);
static void icbox_irect(struct gadget *g, int i, struct win_rect *r);
static void icbox_xyitem(struct gadget *g, int *buf, int x, int y);
static void icbox_redraw_item_cb(struct gadget *g, int i, int wd, int x, int y, int w, int h, int sel_only);
static void icbox_redraw_item(struct gadget *g, int wd, int i, int text_only, int setclip);
static void icbox_size_item_cb(struct gadget *g, int i, int *w, int *h);
static void icbox_redraw_bg(struct gadget *g, int wd, int x, int y, int w, int h);
static void icbox_redraw(struct gadget *g, int wd);
static int  icbox_afisize(struct gadget *g, struct icbox_item *item);
static void icbox_resize(struct gadget *g, int w, int h);
static void icbox_key_down(struct gadget *g, unsigned ch, unsigned shift);
static void icbox_ptr_move(struct gadget *g, int x, int y);
static void icbox_ptr_down(struct gadget *g, int x, int y, int button);
static void icbox_ptr_up(struct gadget *g, int x, int y, int button);
static void icbox_do_defops(struct gadget *g);

static void icbox_remove(struct gadget *g)
{
	int i;
	
	for (i = 0; i < g->icbox.item_count; i++)
	{
		bmp_free(g->icbox.items[i].icon);
		free(g->icbox.items[i].dd_data);
		free(g->icbox.items[i].text);
	}
	free(g->icbox.items);
}

static int icbox_defsizes(struct gadget *g)
{
	int tw, th;
	int resid;
	int ncol;
	int w, h;
	int mw;
	int icw, ich;
	int iw, ih;
	
	icw = g->icbox.icon_width;
	ich = g->icbox.icon_height;
	
	g->icbox.item_height = 1;
	g->icbox.item_width  = 1;
	return 0;
}

static int icbox_usizes(struct gadget *g)
{
	int iw, ih;
	int ncol;
	int w, h;
	int mw;
	
	w  = g->rect.w;
	h  = g->rect.h;
	ih = g->icbox.item_height;
	iw = g->icbox.item_width;
	ncol = w / iw;
	if (ncol)
		iw += w % iw / ncol;
	if (g->icbox.item_width != iw)
	{
		g->icbox.item_height = ih;
		g->icbox.item_width  = iw;
		return 1;
	}
	return 0;
}

static void icbox_irect(struct gadget *g, int i, struct win_rect *r)
{
	int ipl = g->rect.w / g->icbox.item_width;
	
	if (!ipl)
		ipl = 1;
	
	r->x = (i % ipl) * g->icbox.item_width;
	r->y = (i / ipl) * g->icbox.item_height;
	r->w = g->icbox.item_width;
	r->h = g->icbox.item_height;
	
	r->x -= g->icbox.scroll_x;
	r->y -= g->icbox.scroll_y;
}

static void icbox_xyitem(struct gadget *g, int *buf, int x, int y)
{
	int line;
	int col;
	int ipl;
	int i;
	
	ipl = g->rect.w / g->icbox.item_width;
	if (!ipl)
		ipl = 1;
	
	line = (y + g->icbox.scroll_y) / g->icbox.item_height;
	col  = (x + g->icbox.scroll_x) / g->icbox.item_width;
	i = col + line * ipl;
	
	if (col >= ipl)
		i = -1;
	if (i < -1)
		i = -1;
	if (i >= g->icbox.item_count)
		i = -1;
	
	*buf = i;
}

static void icbox_redraw_item_cb(struct gadget *g, int i, int wd, int x, int y, int w, int h, int sel_only)
{
	struct icbox_item *im = &g->icbox.items[i];
	win_color text_bg;
	win_color text;
	win_color bg;
	int text_x;
	int text_y;
	int text_w;
	int text_h;
	
	win_text_size(WIN_FONT_DEFAULT, &text_w, &text_h, im->text);
#if TEXT_BELOW_ICON
	text_x = x + (w - text_w) / 2;
	text_y = y + g->icbox.icon_height + 1;
#else
	text_x = x + g->icbox.icon_width + 1;
	text_y = y + 16 - text_h / 2;
#endif
	
	if (i == g->icbox.item_index)
	{
		text_bg	= wc_get(WC_SEL_BG);
		text	= wc_get(WC_SEL_FG);
	}
	else
	{
		text_bg	= wc_get(WC_BG);
		text	= wc_get(WC_FG);
	}
	bg = wc_get(WC_BG);
	
	if (!sel_only)
	{
#if TEXT_BELOW_ICON
		if (im->icon != NULL)
			bmp_draw(wd, im->icon, x + (w - g->icbox.icon_width) / 2, y + 1);
	}
	win_rect(wd, text_bg, text_x - 2, text_y - 1, text_w + 3, text_h + 2);
	win_text(wd, text, text_x, text_y, im->text);
#else
		if (im->icon != NULL)
			bmp_draw(wd, im->icon, x, y);
	}
	win_btext(wd, text_bg, text, text_x, text_y, im->text);
#endif
}

static void icbox_redraw_item(struct gadget *g, int wd, int i, int text_only, int setclip)
{
	struct icbox_item *im = &g->icbox.items[i];
	struct win_rect r;
	
	if (g->delayed_redraw || g->form->redraw_expected)
		return;	
	if (i < 0 || i >= g->icbox.item_count)
		return;
	if (!g->visible)
		return;
	
	if (setclip)
		win_clip(wd, g->rect.x, g->rect.y, g->rect.w, g->rect.h,
			     g->rect.x, g->rect.y);
	
	icbox_irect(g, i, &r);
	if (r.x + r.w < 0)
		return;
	if (r.y + r.h < 0)
		return;
	if (r.x >= g->rect.w)
		return;
	if (r.y >= g->rect.h)
		return;
	
	win_paint();
	g->icbox.drawitem(g, i, wd, r.x, r.y, r.w, r.h, text_only);
	win_end_paint();
}

static void icbox_size_item_cb(struct gadget *g, int i, int *w, int *h)
{
	struct icbox_item *im = &g->icbox.items[i];
	int tw, th;
	
	win_text_size(WIN_FONT_DEFAULT, &tw, &th, im->text);
	if (tw < g->icbox.icon_width)
		tw = g->icbox.icon_width;
	*w = tw + 5;
	*h = th + g->icbox.icon_height + 3;
}

static void icbox_redraw_bg(struct gadget *g, int wd, int x, int y, int w, int h)
{	
	win_rect(wd, wc_get(WC_BG), x, y, w, h);
}

static void icbox_redraw(struct gadget *g, int wd)
{
	int i;
	
	g->icbox.drawbg(g, wd, 0, 0, g->rect.w, g->rect.h);
	for (i = 0; i < g->icbox.item_count; i++)
		icbox_redraw_item(g, wd, i, 0, 0);
}

static int icbox_afisize(struct gadget *g, struct icbox_item *item)
{
	int iw, ih;
	int r = 0;
	
	g->icbox.sizeitem(g, item - g->icbox.items, &iw, &ih);
	if (iw > g->icbox.item_width)
	{
		g->icbox.item_width = iw;
		r = 1;
	}
	if (ih > g->icbox.item_height)
	{
		g->icbox.item_height = ih;
		r = 1;
	}
	return r;
}

static void icbox_resize(struct gadget *g, int w, int h)
{
	int i;
	
	icbox_defsizes(g);
	for (i = 0; i < g->icbox.item_count; i++)
		icbox_afisize(g, &g->icbox.items[i]);
	icbox_usizes(g);
}

static void icbox_key_down(struct gadget *g, unsigned ch, unsigned shift)
{
	int ipl = g->rect.w / g->icbox.item_width;
	
	if (!ipl)
		ipl = 1;
	
	switch (ch)
	{
	case WIN_KEY_UP:
		if (g->icbox.item_index - ipl >= 0)
			icbox_set_index(g, g->icbox.item_index - ipl);
		break;
	case WIN_KEY_DOWN:
		if (g->icbox.item_index == -1)
			icbox_set_index(g, 0);
		else
		if (g->icbox.item_index + ipl < g->icbox.item_count)
			icbox_set_index(g, g->icbox.item_index + ipl);
		break;
	case WIN_KEY_LEFT:
		if (g->icbox.item_index > 0)
			icbox_set_index(g, g->icbox.item_index - 1);
		break;
	case WIN_KEY_RIGHT:
		if (g->icbox.item_index + 1 < g->icbox.item_count)
			icbox_set_index(g, g->icbox.item_index + 1);
		break;
	case '\n':
		if (g->icbox.item_index >= 0 && g->icbox.request)
			g->icbox.request(g, g->icbox.item_index, 0);
		break;
	}
}

static void icbox_ptr_move(struct gadget *g, int x, int y)
{
	int i = g->icbox.item_index;
	struct icbox_item *it;
	int dx, dy;
	int dcd;
	
	if (!g->ptr_down_cnt)
		return;
	
	dx  = abs(g->form->ptr_down_x - g->rect.x - x);
	dy  = abs(g->form->ptr_down_y - g->rect.y - y);
	dcd = wm_get(WM_DOUBLECLICK);
	
	if (dx < dcd || dy < dcd)
		return;
	
	if (g->icbox.dragdrop)
		return;
	g->icbox.dragdrop = 1;
	
	if (i < 0)
		return;
	
	it = &g->icbox.items[i];
	if (it->dd_data == NULL)
		return;
	
	if (!gadget_dragdrop(g, it->dd_data, it->dd_len))
		gadget_setptr(g, WIN_PTR_DRAG);
}

static void icbox_ptr_down(struct gadget *g, int x, int y, int button)
{
	int i;
	
	icbox_xyitem(g, &i, x, y);
	icbox_set_index(g, i);
}

static void icbox_ptr_up(struct gadget *g, int x, int y, int button)
{
	static int px = -1;
	static int py = -1;
	
#if DOUBLE_CLICK
	int dcd;
#endif
	int i;
	
	if (!g->ptr_down_cnt && g->icbox.dragdrop)
	{
		gadget_setptr(g, WIN_PTR_ARROW);
		g->icbox.dragdrop = 0;
		return;
	}
	
	icbox_xyitem(g, &i, x, y);
	
	if (g->icbox.item_index != i)
	{
		icbox_set_index(g, -1);
		px = -1;
		py = -1;
		return;
	}
	
#if DOUBLE_CLICK
	dcd = wm_get(WM_DOUBLECLICK);
	
	if (!button)
	{
		if (px != -1 && abs(x - px) <= dcd && abs(y - py) <= dcd)
		{
			px = -1;
			py = -1;
			
			if (i >= 0 && g->icbox.request)
				g->icbox.request(g, i, button);
		}
		else
		{
			px = x;
			py = y;
		}
	}
#else
	if (!button && i >= 0 && g->icbox.request && !g->icbox.dragdrop)
		g->icbox.request(g, i, button);
#endif
}

static void icbox_do_defops(struct gadget *g)
{
	if (g->icbox.def_scroll)
	{
		g->icbox.def_scroll = 0;
		icbox_scroll_now(g, g->icbox.def_scroll_x, g->icbox.def_scroll_y);
	}
}

struct gadget *icbox_creat(struct form *f, int x, int y, int w, int h)
{
	struct gadget *g;
	
	g = gadget_creat(f, x, y, w, h);
	if (!g)
		return NULL;
	
	g->icbox.drawitem    = icbox_redraw_item_cb;
	g->icbox.sizeitem    = icbox_size_item_cb;
	g->icbox.drawbg	     = icbox_redraw_bg;
	g->icbox.icon_width  = 32;
	g->icbox.icon_height = 32;
	
	g->want_focus	= 1;
	g->remove	= icbox_remove;
	g->redraw	= icbox_redraw;
	g->resize	= icbox_resize;
	g->do_defops	= icbox_do_defops;
	
	g->key_down	= icbox_key_down;
	g->ptr_move	= icbox_ptr_move;
	g->ptr_down	= icbox_ptr_down;
	g->ptr_up	= icbox_ptr_up;
	
	icbox_defsizes(g);
	return g;
}

int icbox_set_icon_size(struct gadget *g, int w, int h)
{
	if (w <= 0 || h <= 0)
	{
		_set_errno(EINVAL);
		return -1;
	}
	
	g->icbox.icon_width  = w;
	g->icbox.icon_height = h;
	return 0;
}

int icbox_newitem(struct gadget *g, const char *text, const char *icname)
{
	struct icbox_item *ni;
	struct bitmap *nicon;
	struct bitmap *icon;
	int i;
	
	ni = realloc(g->icbox.items, (g->icbox.item_count + 1) * sizeof *ni);
	if (!ni)
		return -1;
	g->icbox.items = ni;
	
	if (icname != NULL)
	{
		if (icon = bmp_load(icname), !icon)
			if (icon = bmp_load(DEFAULT_ICON), !icon)
				return -1;
		nicon = bmp_scale(icon, g->icbox.icon_width, g->icbox.icon_height);
		if (nicon)
			bmp_free(icon);
		else
			nicon = icon;
		if (bmp_conv(nicon))
		{
			bmp_free(nicon);
			return -1;
		}
	}
	else
		nicon = NULL;
	
	ni[g->icbox.item_count].text	= strdup(text);
	ni[g->icbox.item_count].icon	= nicon;
	ni[g->icbox.item_count].dd_data	= NULL;
	ni[g->icbox.item_count].dd_len	= 0;
	
	if (!ni[g->icbox.item_count].text)
	{
		bmp_free(nicon);
		return -1;
	}
	g->icbox.item_count++;
	
	if (icbox_afisize(g, &ni[g->icbox.item_count - 1]))
	{
		icbox_usizes(g);
		gadget_redraw(g);
	}
	else
		icbox_redraw_item(g, g->form->wd, g->icbox.item_count - 1, 0, 1);
	return 0;
}

void icbox_clear(struct gadget *g)
{
	int i;
	
	for (i = 0; i < g->icbox.item_count; i++)
	{
		bmp_free(g->icbox.items[i].icon);
		free(g->icbox.items[i].dd_data);
		free(g->icbox.items[i].text);
	}
	free(g->icbox.items);
	g->icbox.items = NULL;
	
	g->icbox.item_index = -1;
	g->icbox.item_count = 0;
	
	g->icbox.scroll_x = 0;
	g->icbox.scroll_y = 0;
	
	icbox_defsizes(g);
	gadget_redraw(g);
}

int icbox_get_index(struct gadget *g)
{
	return g->icbox.item_index;
}

int icbox_set_index(struct gadget *g, int index)
{
	int prev = g->icbox.item_index;
	struct win_rect r;
	int sx = 0;
	int sy = 0;
	
	if (index < -1 || index >= g->icbox.item_count)
	{
		_set_errno(EINVAL);
		return -1;
	}
	
	if (prev == index)
		return 0;
	
	g->icbox.item_index = index;
	
	if (index >= 0)
	{
		icbox_irect(g, index, &r);
		if (r.x + r.w > g->rect.w)
			sx = r.x + r.w - g->rect.w;
		if (r.y + r.h > g->rect.h)
			sy = r.y + r.h - g->rect.h;
		if (r.x < 0)
			sx = r.x;
		if (r.y < 0)
			sy = r.y;
		
		icbox_redraw_item(g, g->form->wd, prev,  1, 1);
		icbox_redraw_item(g, g->form->wd, index, 1, 1);
		if (sx || sy)
			icbox_scroll(g, g->icbox.scroll_x + sx,
					g->icbox.scroll_y + sy);
	}
	else
		icbox_redraw_item(g, g->form->wd, prev, 1, 1);
	
	return 0;
}

const struct bitmap *icbox_get_icon(struct gadget *g, int index)
{
	if (index < 0 || index >= g->icbox.item_count)
	{
		_set_errno(EINVAL);
		return NULL;
	}
	
	return g->icbox.items[index].icon;
}

const char *icbox_get_text(struct gadget *g, int index)
{
	if (index < 0 || index >= g->icbox.item_count)
	{
		_set_errno(EINVAL);
		return NULL;
	}
	
	return g->icbox.items[index].text;
}

int icbox_get_scroll_range(struct gadget *g, int *x, int *y)
{
	int ipl;
	int lc;
	
	ipl = g->rect.w / g->icbox.item_width;
	if (!ipl)
		ipl = 1;
	
	lc = (g->icbox.item_count - 1) / ipl + 1;
	
	*y = g->icbox.item_height * lc - g->rect.h;
	*x = 0;
	
	if (*y < 0)
		*y = 0;
	return 0;
}

int icbox_scroll_now(struct gadget *g, int x, int y)
{
	int dx, dy;
	int w, h;
	
	dx = x - g->icbox.scroll_x;
	dy = y - g->icbox.scroll_y;
	
	if (!dx && !dy)
		return 0;
	
	g->icbox.scroll_x = x;
	g->icbox.scroll_y = y;
	
	if (abs(dy) < g->rect.h && !dx)
	{
		w = g->rect.w;
		h = g->rect.h;
		
		win_paint();
		gadget_clip(g, 0, 0, w, h, 0, 0);
		if (dy > 0)
		{
			win_copy(g->form->wd, 0, 0, 0, dy, w, h - dy);
			gadget_clip(g, 0, h - dy, w, dy, 0, 0);
		}
		else
		{
			win_copy(g->form->wd, 0, -dy, 0, 0, w, h + dy);
			gadget_clip(g, 0, 0, w, -dy, 0, 0);
		}
		icbox_redraw(g, g->form->wd);
		win_end_paint();
		goto fini;
	}
	
	gadget_redraw(g);
fini:
	if (g->icbox.scroll)
		g->icbox.scroll(g, x, y);
	return 0;
}

int icbox_scroll(struct gadget *g, int x, int y)
{
	g->icbox.def_scroll_x = x;
	g->icbox.def_scroll_y = y;
	g->icbox.def_scroll   = 1;
	gadget_defer(g);
	return 0;
}

int icbox_on_request(struct gadget *g, icbox_request_cb *proc)
{
	g->icbox.request = proc;
	return 0;
}

int icbox_on_select(struct gadget *g, icbox_select_cb *proc)
{
	g->icbox.select = proc;
	return 0;
}

int icbox_on_scroll(struct gadget *g, icbox_scroll_cb *proc)
{
	g->icbox.scroll = proc;
	return 0;
}

void icbox_set_bg_cb(struct gadget *g, icbox_drawbg_cb *proc)
{
	if (!proc)
		g->icbox.drawbg = icbox_redraw_bg;
	else
		g->icbox.drawbg = proc;
}

void icbox_set_item_cb(struct gadget *g, icbox_drawitem_cb *proc)
{
	if (!proc)
		g->icbox.drawitem = icbox_redraw_item_cb;
	else
		g->icbox.drawitem = proc;
}

void icbox_set_size_cb(struct gadget *g, icbox_sizeitem_cb *proc)
{
	if (!proc)
		g->icbox.sizeitem = icbox_size_item_cb;
	else
		g->icbox.sizeitem = proc;
}

int icbox_set_dd_data(struct gadget *g, int index, const void *data, size_t len)
{
	struct icbox_item *it;
	void *p;
	
	if (index < 0 || index >= g->icbox.item_count)
	{
		_set_errno(EINVAL);
		return NULL;
	}
	
	it = &g->icbox.items[index];
	
	free(it->dd_data);
	it->dd_data = NULL;
	
	p = malloc(len);
	if (p == NULL)
		return -1;
	memcpy(p, data, len);
	
	it->dd_data = p;
	it->dd_len  = len;
	return 0;
}
