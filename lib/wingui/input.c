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
#include <wingui_cgadget.h>
#include <wingui_metrics.h>
#include <wingui_color.h>
#include <wingui_form.h>
#include <wingui.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <timer.h>
#include <ctype.h>

static const struct timeval input_ctv = { 0, 333333L };

static void input_creset(struct gadget *g)
{
	tmr_reset(g->input.timer, &input_ctv, &input_ctv);
	tmr_start(g->input.timer);
	g->input.c_state = 1;
}

static void input_redraw_cursor(struct gadget *g, int wd)
{
	win_color crsr = wc_get(WC_CURSOR);
	win_color bg = wc_get(WC_BG);
	win_color fg = wc_get(WC_FG);
	int text_w;
	int text_h;
	char ch;
	int tl;
	int y;
	
	tl = wm_get(WM_THIN_LINE);
	
	if (g->input.flags & INPUT_PASSWORD)
	{
		win_chr_size(g->font, &text_w, &text_h, 'X');
		text_w = 0;
	}
	else
	{
		ch = g->text[g->input.curr];
		g->text[g->input.curr] = 0;
		win_text_size(g->font, &text_w, &text_h, g->text);
		g->text[g->input.curr] = ch;
	}
	y = (g->rect.h - text_h) / 2;
	
	win_clip(wd,
		 g->rect.x, g->rect.y, g->rect.w, g->rect.h,
		 g->rect.x, g->rect.y);
	win_paint();
	
	if (!g->input.c_state)
		win_rect(wd, bg, y + text_w, y, 2 * tl, text_h);
	if (ch && (g->input.flags & INPUT_PASSWORD) == 0)
		win_chr(wd, fg, y + text_w, y, ch);
	if (g->input.c_state)
		win_rect(wd, crsr, y + text_w, y, 2 * tl, text_h);
	win_end_paint();
}

static void input_redraw(struct gadget *g, int wd)
{
	win_color crsr = wc_get(WC_CURSOR);
	win_color hi1 = wc_get(WC_HIGHLIGHT1);
	win_color hi2 = wc_get(WC_HIGHLIGHT2);
	win_color sh1 = wc_get(WC_SHADOW1);
	win_color sh2 = wc_get(WC_SHADOW2);
	win_color bg = wc_get(WC_BG);
	win_color fg = wc_get(WC_FG);
	int tl = wm_get(WM_THIN_LINE);
	int text_w;
	int text_h;
	int cur_x = 0;
	char ch;
	int y;
	
	ch = g->text[g->input.curr];
	g->text[g->input.curr] = 0;
	win_text_size(g->font, &text_w, &text_h, g->text);
	g->text[g->input.curr] = ch;
	
	cur_x = y = (g->rect.h - text_h) / 2;
	
	win_rect(wd, bg, 0, 0, g->rect.w, g->rect.h);
	if (!(g->input.flags & INPUT_PASSWORD))
	{
		win_text(wd, fg, y, y, g->text);
		cur_x = y + text_w;
	}
	
	if (g == g->form->focus && g->form->focused && g->input.c_state)
		win_rect(wd, crsr, cur_x, y, 2 * tl, text_h);
	
	if (g->input.flags & INPUT_FRAME)
		form_draw_frame3d(wd, 0, 0, g->rect.w, g->rect.h, sh1, sh2, hi1, hi2);
}

static void input_remove(struct gadget *g)
{
	tmr_free(g->input.timer);
	free(g->text);
}

static void input_focus(struct gadget *g)
{
	if (g->form->focus == g)
		input_creset(g);
	else
		tmr_stop(g->input.timer);
	gadget_redraw(g);
}

static void input_ptr_down(struct gadget *g, int x, int y, int button)
{
	char *p = g->text;
	int cw, ch;
	int w;
	
	win_chr_size(g->font, &cw, &ch, 'X');
	w = (g->rect.h - ch) / 2;
	
	if (g->input.flags & INPUT_PASSWORD)
		return;
	
	while (*p)
	{
		win_chr_size(g->font, &cw, &ch, *p);
		w += cw;
		if (w >= x)
			break;
		p++;
	}
	g->input.curr = p - g->text;
	gadget_redraw(g);
}

static void input_key_down(struct gadget *g, unsigned ch, unsigned shift)
{
	char *ntx;
	int len;
	int i;
	
	input_creset(g);
	switch (ch)
	{
	case WIN_KEY_HOME:
		if (g->input.curr)
		{
			g->input.curr = 0;
			gadget_redraw(g);
		}
		break;
	case WIN_KEY_END:
		g->input.curr = strlen(g->text);
		gadget_redraw(g);
		break;
	case WIN_KEY_RIGHT:
		if (g->input.curr < strlen(g->text))
		{
			g->input.curr++;
			gadget_redraw(g);
		}
		break;
	case WIN_KEY_LEFT:
		if (g->input.curr)
		{
			g->input.curr--;
			gadget_redraw(g);
		}
		break;
	case 'U' & 0x1f:
		if (g->read_only)
			break;
		g->input.curr = 0;
		ntx = realloc(g->text, 1);
		if (ntx)
			g->text = ntx;
		*g->text = 0;
		gadget_redraw(g);
		if (g->input.change)
			g->input.change(g);
		break;
	case WIN_KEY_DEL:
		if (g->read_only)
			break;
		
		len = strlen(g->text);
		i = g->input.curr;
		if (!i)
			break;
		
		memmove(g->text + i, g->text + i + 1, len - i + 1);
		ntx = realloc(g->text, len);
		if (ntx)
			g->text = ntx;
		gadget_redraw(g);
		if (g->input.change)
			g->input.change(g);
		break;
	case '\b':
		if (g->read_only)
			break;
		
		len = strlen(g->text);
		i = g->input.curr;
		if (!i)
			break;
		
		memmove(g->text + i - 1, g->text + i, len - i + 2);
		ntx = realloc(g->text, len);
		if (ntx)
			g->text = ntx;
		g->input.curr--;
		gadget_redraw(g);
		if (g->input.change)
			g->input.change(g);
		break;
	case '\n':
		break;
	default:
		if (!isprint(ch))
			break;
		if (g->read_only)
			break;
		
		len = strlen(g->text);
		i = g->input.curr;
		
		ntx = realloc(g->text, len + 2);
		if (!ntx)
			break;
		
		g->text = ntx;
		memmove(ntx + i + 1, ntx + i, len - i + 1);
		ntx[i] = ch;
		g->input.curr++;
		
		gadget_redraw(g);
		if (g->input.change)
			g->input.change(g);
		break;
	}
}

static void input_timer(void *cx)
{
	struct gadget *g = cx;
	
	g->input.c_state = !g->input.c_state;
	if (g->visible)
		input_redraw_cursor(g, g->form->wd);
}

struct gadget *input_creat(struct form *form, int x, int y, int w, int h)
{
	struct gadget *g;
	struct timer *t;
	char *text;
	
	text = strdup("");
	if (!text)
		return NULL;
	
	g = gadget_creat(form, x, y, w, h);
	if (!g)
	{
		free(text);
		return NULL;
	}
	
	t = tmr_creat(&input_ctv, &input_ctv, input_timer, g, 0);
	if (t == NULL)
	{
		gadget_remove(g);
		return NULL;
	}
	
	g->text		= text;
	g->input.timer	= t;
	g->ptr		= WIN_PTR_TEXT;
	
	g->redraw	= input_redraw;
	g->remove	= input_remove;
	g->focus	= input_focus;
	g->key_down	= input_key_down;
	g->ptr_down	= input_ptr_down;
	g->want_focus	= 1;
	
	gadget_redraw(g);
	return g;
}

void input_set_flags(struct gadget *g, int flags)
{
	g->input.flags = flags;
	gadget_redraw(g);
}

int input_set_text(struct gadget *g, const char *text)
{
	char *ntx;
	
	ntx = strdup(text);
	if (!ntx)
		return -1;
	
	free(g->text);
	g->text = ntx;
	g->input.curr = strlen(ntx);
	gadget_redraw(g);
	if (g->input.change)
		g->input.change(g);
	return 0;
}

int input_set_pos(struct gadget *g, int pos)
{
	size_t len;
	
	len = strlen(g->text);
	if (pos < 0)
	{
		_set_errno(EINVAL);
		return -1;
	}
	else if (pos > len)
	{
		_set_errno(EINVAL);
		return -1;
	}
	g->input.curr = pos;
	gadget_redraw(g);
	return 0;
}

void input_on_change(struct gadget *g, input_change_cb *proc)
{
	g->input.change = proc;
}
