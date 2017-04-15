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

#include <priv/wingui_theme.h>
#include <priv/libc.h>
#include <wingui_cgadget.h>
#include <wingui_metrics.h>
#include <wingui_color.h>
#include <wingui_form.h>
#include <wingui_menu.h>
#include <wingui.h>
#include <timer.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

static const struct timeval edbox_ctv = { 0, 333333L };

static void edbox_remove(struct gadget *g)
{
	free(g->text);
}

static void edbox_creset(struct gadget *g)
{
	tmr_reset(g->edbox.timer, &edbox_ctv, &edbox_ctv);
	tmr_start(g->edbox.timer);
	g->edbox.c_state = 1;
}

static void edbox_focus(struct gadget *g)
{
	if (g->form->focus == g)
		edbox_creset(g);
	else
		tmr_stop(g->edbox.timer);
	gadget_redraw(g);
}

static int edbox_lcindex(struct gadget *g, int *buf, int column, int line)
{
	char *p = g->text;
	int cnt = line;
	char *nl;
	int len;
	
	if (column < 0 || line < 0)
	{
		_set_errno(EINVAL);
		return -1;
	}
	
	while (cnt)
	{
		p = strchr(p, '\n');
		if (!p)
		{
			_set_errno(EINVAL);
			return -1;
		}
		
		cnt--;
		p++;
	}
	
	nl = strchr(p, '\n');
	if (nl)
		len = nl - p;
	else
		len = strlen(p);
	
	if (column > len)
	{
		_set_errno(EINVAL);
		return -1;
	}
	
	*buf = p - g->text + column;
	return 0;
}

static void edbox_line_ptr(struct gadget *g, int line, char **buf)
{
	int i;
	
	edbox_lcindex(g, &i, 0, line);
	
	*buf = g->text + i;
}

static void edbox_xylcindex(struct gadget *g, int x, int y, int *cp, int *lp, int *ip)
{
	int text_w;
	int chr_w;
	int chr_h;
	int len;
	char *p;
	int lc;
	int i;
	
	edbox_get_line_count(g, &lc);
	x += g->edbox.hpos;
	
	*lp = y / g->edbox.line_height + g->edbox.topmost_line;
	if (*lp >= lc)
		*lp = lc - 1;
	
	edbox_line_ptr(g, *lp, &p);
	len = strlen(p);
	
	text_w = 0;
	for (i = 0; i < len; i++)
	{
		win_chr_size(g->edbox.font, &chr_w, &chr_h, p[i]);
		text_w += chr_w;
		
		if (text_w >= x || p[i] == '\n')
			break;
	}
	
	*ip = p + i - g->text;
	*cp = i;
}

static void edbox_indexlc(struct gadget *g, int i, int *l, int *c)
{
	char *p = g->text;
	
	*l = 0;
	*c = 0;
	
	while (i--)
	{
		if (*p == '\n')
		{
			(*l)++;
			*c = 0;
		}
		else 
			(*c)++;
		p++;
	}
}

static void edbox_redraw_line(struct gadget *g, int wd, int line)
{
	int y = (line - g->edbox.topmost_line) * g->edbox.line_height;
	int x = -g->edbox.hpos;
	int sw = g->edbox.shadow_w;
	int tl = wm_get(WM_THIN_LINE);
	win_color crsr;
	win_color s_bg;
	win_color s_fg;
	win_color bg;
	win_color fg;
	int crsr_x = -1;
	int chr_w;
	int chr_h;
	char *p;
	int i;
	
	if (!g->visible)
		return;
	
	edbox_lcindex(g, &i, 0, line);
	p = g->text + i;
	
	s_fg = wc_get(WC_SEL_FG);
	s_bg = wc_get(WC_SEL_BG);
	crsr = wc_get(WC_CURSOR);
	
	if (line == g->edbox.curr_line && g->form->focus == g)
	{
		fg = wc_get(WC_CUR_FG);
		bg = wc_get(WC_CUR_BG);
	}
	else
	{
		fg = wc_get(WC_FG);
		bg = wc_get(WC_BG);
	}
	
	win_paint();
	win_clip(wd, g->rect.x + sw, g->rect.y + sw, g->rect.w - 2 * sw, g->rect.h - 2 * sw,
		     g->rect.x + sw, g->rect.y + sw);
	
	win_set_font(wd, g->edbox.font);
	while (*p && *p != '\n')
	{
		if (i >= g->edbox.sel_start && i < g->edbox.sel_end)
			win_bchr(wd, s_bg, s_fg, x, y, *p);
		else
			win_bchr(wd, bg, fg, x, y, *p);
		
		if (i == g->edbox.curr)
			crsr_x = x;
		
		win_chr_size(g->edbox.font, &chr_w, &chr_h, *p);
		
		x += chr_w;
		i++;
		p++;
	}
	win_rect(wd, bg, x, y, g->rect.w - x, g->edbox.line_height);
	
	if (i == g->edbox.curr)
		crsr_x = x;
	
	if (g->form->focus == g && g->form->focused && g->edbox.c_state && crsr_x >= 0)
		win_rect(wd, crsr, crsr_x, y, 2 * tl, g->edbox.line_height);
	
	win_end_paint();
}

static void edbox_redraw(struct gadget *g, int wd)
{
	const struct form_theme *th = form_th_get();
	win_color bg;
	int vlc;
	int lc;
	int i;
	
	bg = wc_get(WC_BG);
	
	edbox_get_line_count(g, &lc);
	
	vlc = g->rect.h / g->edbox.line_height + 1;
	if (vlc > lc - g->edbox.topmost_line)
		vlc = lc - g->edbox.topmost_line;
	
	win_rect(wd, bg, 0,
			 vlc * g->edbox.line_height,
			 g->rect.w,
			 g->rect.h - vlc * g->edbox.line_height);
	
	if (th->d_edboxframe)
		th->d_edboxframe(wd, 0, 0, g->rect.w, g->rect.h, g->edbox.shadow_w, g == g->form->focus);
	else
	{
		int sw = g->edbox.shadow_w;
		
		win_rect(wd, bg, 0, 0, g->rect.w, sw);
		win_rect(wd, bg, 0, g->rect.h - sw, g->rect.w, sw);
		win_rect(wd, bg, 0, sw, sw, g->rect.h - 2 * sw);
		win_rect(wd, bg, g->rect.w - sw, sw, sw, g->rect.h - 2 * sw);
	}
	
	for (i = 0; i < vlc; i++)
		edbox_redraw_line(g, wd, i + g->edbox.topmost_line);
}

static void edbox_ptr_down(struct gadget *g, int x, int y, int button)
{
	int l, c, i;
	
	if (button)
	{
		if (g->menu)
			menu_popup(g->menu,
				   g->rect.x + g->form->win_rect.x + x,
				   g->rect.y + g->form->win_rect.y + y);
		return;
	}
	
	g->edbox.ptr_down++;
	if (g->edbox.ptr_down == 1)
	{
		edbox_xylcindex(g, x, y, &c, &l, &i);
		
		g->edbox.newsel_start = i;
		edbox_sel_set(g, i, i);
		edbox_goto(g, c, l);
	}
}

static void edbox_ptr_up(struct gadget *g, int x, int y, int button)
{
	int l, c, i;
	
	if (button)
		return;
	
	g->edbox.ptr_down--;
	if (!g->edbox.ptr_down)
	{
		edbox_xylcindex(g, x, y, &c, &l, &i);
		
		edbox_sel_set(g, g->edbox.newsel_start, i);
		edbox_goto(g, c, l);
	}
}

static void edbox_ptr_move(struct gadget *g, int x, int y)
{
	int l, c, i;
	
	if (g->edbox.ptr_down)
	{
		edbox_xylcindex(g, x, y, &c, &l, &i);
		
		edbox_sel_set(g, g->edbox.newsel_start, i);
		edbox_goto(g, c, l);
	}
}

static void edbox_inschr(struct gadget *g, unsigned ch)
{
	char *ntext;
	int len;
	
	if (g->edbox.sel_start != g->edbox.sel_end)
		edbox_sel_delete(g);
	
	len = strlen(g->text);
	ntext = realloc(g->text, len + 2);
	if (!ntext)
		return;
	
	memmove(ntext + g->edbox.curr + 1, ntext + g->edbox.curr, len - g->edbox.curr + 1);
	ntext[g->edbox.curr] = ch;
	g->text = ntext;
	
	edbox_creset(g);
	edbox_goto_index(g, g->edbox.curr + 1);
	if (g->edbox.edit)
		g->edbox.edit(g);
}

static void edbox_rmchr(struct gadget *g)
{
	char *ntext;
	int len;
	
	if (!g->edbox.curr)
		return;
	
	if (g->edbox.sel_start != g->edbox.sel_end)
	{
		g->edbox.sel_start = 0;
		g->edbox.sel_end = 0;
		gadget_redraw(g);
	}
	
	len = strlen(g->text);
	
	memmove(g->text + g->edbox.curr - 1,
		g->text + g->edbox.curr,
		len - g->edbox.curr + 1);
	
	ntext = realloc(g->text, len);
	if (ntext)
		g->text = ntext;
	edbox_creset(g);
	
	if (!g->edbox.curr_column)
		edbox_goto_index(g, g->edbox.curr - 1);
	else
	{
		g->edbox.curr--;
		g->edbox.curr_column--;
		edbox_redraw_line(g, g->form->wd, g->edbox.curr_line);
	}
	
	if (g->edbox.edit)
		g->edbox.edit(g);
}

static void edbox_key_down(struct gadget *g, unsigned ch, unsigned shift)
{
	int len;
	int i;
	
	switch (ch)
	{
	case WIN_KEY_UP:
		edbox_goto(g, 0, g->edbox.curr_line - 1);
		
		if (shift & WIN_SHIFT_SHIFT)
			edbox_sel_set(g, g->edbox.newsel_start, g->edbox.curr);
		else
			edbox_sel_set(g, 0, 0);
		
		edbox_creset(g);
		break;
	case WIN_KEY_DOWN:
		edbox_goto(g, 0, g->edbox.curr_line + 1);
		
		if (shift & WIN_SHIFT_SHIFT)
			edbox_sel_set(g, g->edbox.newsel_start, g->edbox.curr);
		else
			edbox_sel_set(g, 0, 0);
		
		edbox_creset(g);
		break;
	case WIN_KEY_LEFT:
		edbox_goto_index(g, g->edbox.curr - 1);
		
		if (shift & WIN_SHIFT_SHIFT)
			edbox_sel_set(g, g->edbox.newsel_start, g->edbox.curr);
		else
			edbox_sel_set(g, 0, 0);
		
		edbox_creset(g);
		break;
	case WIN_KEY_RIGHT:
		edbox_goto_index(g, g->edbox.curr + 1);
		
		if (shift & WIN_SHIFT_SHIFT)
			edbox_sel_set(g, g->edbox.newsel_start, g->edbox.curr);
		else
			edbox_sel_set(g, 0, 0);
		
		edbox_creset(g);
		break;
	case '\b':
		if (g->read_only)
			break;
		if (g->edbox.sel_start != g->edbox.sel_end)
			edbox_sel_delete(g);
		else
			edbox_rmchr(g);
		
		edbox_creset(g);
		break;
	case WIN_KEY_PGUP:
		break;
	case WIN_KEY_PGDN:
		break;
	case WIN_KEY_HOME:
		i = g->edbox.curr - 1;
		if (i < 0)
			break;
		while (i >= 0 && g->text[i] != '\n')
			i--;
		edbox_goto_index(g, i + 1);
		
		if (shift & WIN_SHIFT_SHIFT)
			edbox_sel_set(g, g->edbox.newsel_start, g->edbox.curr);
		else
			edbox_sel_set(g, 0, 0);
		
		edbox_creset(g);
		break;
	case WIN_KEY_END:
		len = strlen(g->text);
		i = g->edbox.curr;
		while (i < len && g->text[i] != '\n')
			i++;
		edbox_goto_index(g, i);
		
		if (shift & WIN_SHIFT_SHIFT)
			edbox_sel_set(g, g->edbox.newsel_start, g->edbox.curr);
		else
			edbox_sel_set(g, 0, 0);
		
		edbox_creset(g);
		break;
	case WIN_KEY_INS:
		break;
	case WIN_KEY_DEL:
		if (g->read_only)
			break;
		if (g->edbox.sel_start != g->edbox.sel_end)
			edbox_sel_delete(g);
		else if (g->edbox.curr < strlen(g->text))
		{
			edbox_goto_index(g, g->edbox.curr + 1);
			edbox_rmchr(g);
		}
		edbox_creset(g);
		break;
	case WIN_KEY_SHIFT:
		g->edbox.newsel_start = g->edbox.curr;
		break;
	default:
		if (g->read_only)
			break;
		if (isprint(ch) || ch == '\n')
			edbox_inschr(g, ch);
	}
}

static void edbox_timer(void *cx)
{
	struct gadget *g = cx;
	
	if (!g->visible)
		return;
	
	g->edbox.c_state = !g->edbox.c_state;
	
	win_paint();
	win_clip(g->form->wd, g->rect.x, g->rect.y, g->rect.w, g->rect.h,
			      g->rect.x, g->rect.y);
	edbox_redraw_line(g, g->form->wd, g->edbox.curr_line);
	win_end_paint();
}

struct gadget *edbox_creat(struct form *form, int x, int y, int w, int h)
{
	const struct form_theme *th = form_th_get();
	struct gadget *g;
	int text_w;
	int text_h;
	char *text;
	int se;
	
	text = strdup("");
	if (!text)
		return NULL;
	
	g = gadget_creat(form, x, y, w, h);
	if (!g)
	{
		free(text);
		return NULL;
	}
	
	g->edbox.timer = tmr_creat(&edbox_ctv, &edbox_ctv, edbox_timer, g, 0);
	if (g->edbox.timer == NULL)
	{
		se = _get_errno();
		gadget_remove(g);
		_set_errno(se);
		return NULL;
	}
	
	g->text = text;
	
	win_text_size(WIN_FONT_DEFAULT, &text_w, &text_h, "X");
	g->edbox.font = WIN_FONT_DEFAULT;
	g->edbox.line_height = text_h;
	
	g->want_focus = 1;
	g->ptr	      = WIN_PTR_TEXT;
	
	g->remove   = edbox_remove;
	g->focus    = edbox_focus;
	g->redraw   = edbox_redraw;
	g->ptr_down = edbox_ptr_down;
	g->ptr_up   = edbox_ptr_up;
	g->ptr_move = edbox_ptr_move;
	g->key_down = edbox_key_down;
	
	g->edbox.shadow_w = (th != NULL) ? th->edbox_shadow_w : 0;
	
	gadget_redraw(g);
	return g;
}

void edbox_set_flags(struct gadget *g, int flags)
{
	g->edbox.flags = flags;
	gadget_redraw(g);
}

void edbox_set_font(struct gadget *g, int ftd)
{
	int text_w;
	int text_h;
	
	win_text_size(ftd, &text_w, &text_h, "X");
	
	g->edbox.font		= ftd;
	g->edbox.line_height	= text_h;
	
	gadget_redraw(g);
}

int edbox_set_text(struct gadget *g, const char *text)
{
	char *ntext = strdup(text);
	
	if (!ntext)
		return -1;
	
	free(g->text);
	g->text = ntext;
	
	g->edbox.curr		= 0;
	g->edbox.curr_line	= 0;
	g->edbox.curr_column	= 0;
	g->edbox.topmost_line	= 0;
	g->edbox.sel_start	= 0;
	g->edbox.sel_end	= 0;
	
	gadget_redraw(g);
	return 0;
}

int edbox_get_text(struct gadget *g, char *buf, int maxlen)
{
	if (!maxlen)
		__libc_panic("edbox_get_text: !maxlen");
	
	if (strlen(g->text) + 1 <= maxlen)
		strcpy(buf, g->text);
	else
	{
		memcpy(buf, g->text, maxlen - 1);
		buf[maxlen - 1] = 0;
	}
	
	return 0;
}

int edbox_get_text_ptr(struct gadget *g, char **text)
{
	*text = g->text;
	return 0;
}

int edbox_get_text_len(struct gadget *g, int *len)
{
	*len = strlen(g->text);
	return 0;
}

int edbox_get_line_count(struct gadget *g, int *count)
{
	char *p = g->text;
	
	*count = 0;
	while (p)
	{
		p = strchr(p, '\n');
		if (p)
			p++;
		
		(*count)++;
	}
	
	return 0;
}

int edbox_get_text_width(struct gadget *g, int *width)
{
	int tw, th;
	int w, h;
	int cw;
	char *p;
	
	if (win_text_size(g->edbox.font, &w, &h, g->text))
	{
		if (_get_errno() == ERANGE)
		{
			win_chr_size(g->edbox.font, &w, &h, 0);
			tw = 0;
			th = h;
			cw = 0;
			
			for (p = g->text; *p; p++)
			{
				if (*p == '\n')
				{
					if (cw > tw)
						tw = cw;
					th += h;
					cw  = 0;
					continue;
				}
				
				win_chr_size(g->edbox.font, &w, &h, *p);
				cw += w;
			}
			if (cw > tw)
				tw = cw;
			
			*width = tw + 4;
			return 0;
		}
		return -1;
	}
	*width = w + 4;
	return 0;
}

int edbox_vscroll(struct gadget *g, int topmost_line)
{
	int lc;
	
	if (g->edbox.topmost_line == topmost_line)
		return 0;
	
	if (topmost_line < 0)
	{
		_set_errno(EINVAL);
		return -1;
	}
	
	edbox_get_line_count(g, &lc);
	if (topmost_line >= lc)
	{
		_set_errno(EINVAL);
		return -1;
	}
	
	g->edbox.topmost_line = topmost_line;
	if (g->edbox.vscroll)
		g->edbox.vscroll(g, topmost_line);
	
	gadget_redraw(g);
	return 0;
}

int edbox_hscroll(struct gadget *g, int hpix)
{
	if (g->edbox.hpos == hpix)
		return 0;
	
	if (hpix < 0)
	{
		_set_errno(EINVAL);
		return -1;
	}
	
	g->edbox.hpos = hpix;
	if (g->edbox.hscroll != NULL)
		g->edbox.hscroll(g, hpix);
	
	gadget_redraw(g);
	return 0;
}

int edbox_goto(struct gadget *g, int column, int line)
{
	int i;
	
	if (edbox_lcindex(g, &i, column, line))
		return -1;
	
	return edbox_goto_index(g, i);
}

int edbox_goto_index(struct gadget *g, int index)
{
	char *text = g->text;
	int len = strlen(text);
	int lstart;
	int cx, cw, ch;
	int vlc;
	int pl;
	int i;
	
	if (index < 0 || index > len)
	{
		_set_errno(EINVAL);
		return -1;
	}
	
	pl = g->edbox.curr_line;
	
	g->edbox.curr_column = 0;
	g->edbox.curr_line = 0;
	g->edbox.curr = index;
	
	lstart = 0;
	for (i = 0; i < index; i++)
		if (text[i] == '\n')
		{
			g->edbox.curr_column = 0;
			g->edbox.curr_line++;
			lstart = i + 1;
		}
		else
			g->edbox.curr_column++;
	
	for (cx = 0, i = lstart; i < index; i++)
	{
		win_chr_size(g->edbox.font, &cw, &ch, g->text[i]);
		cx += cw;
	}
	
	if (g->edbox.curr_line < g->edbox.topmost_line)
		edbox_vscroll(g, g->edbox.curr_line);
	else
	{
		vlc = g->rect.h / g->edbox.line_height;
		
		if (g->edbox.curr_line - g->edbox.topmost_line >= vlc)
			edbox_vscroll(g, g->edbox.curr_line - vlc + 1);
	}
	
	if (cx < g->edbox.hpos)
		edbox_hscroll(g, cx);
	else if (cx >= g->rect.w + g->edbox.hpos)
		edbox_hscroll(g, cx - g->rect.w);
	
	if (pl != g->edbox.curr_line)
		gadget_redraw(g);
	else
		edbox_redraw_line(g, g->form->wd, pl);
	
	return 0;
}

int edbox_on_vscroll(struct gadget *g, edbox_vscroll_cb *proc)
{
	g->edbox.vscroll = proc;
	return 0;
}

int edbox_on_hscroll(struct gadget *g, edbox_hscroll_cb *proc)
{
	g->edbox.hscroll = proc;
	return 0;
}

int edbox_on_edit(struct gadget *g, edbox_edit_cb *proc)
{
	g->edbox.edit = proc;
	return 0;
}

int edbox_insert(struct gadget *g, const char *text)
{
	int len = strlen(g->text);
	int ntl = strlen(text);
	char *ntext;
	
	if (g->edbox.sel_start != g->edbox.sel_end)
		edbox_sel_delete(g);
	
	ntext = realloc(g->text, len + ntl + 1);
	if (!ntext)
		return -1;
	
	g->text = ntext;
	
	memmove(ntext + ntl + g->edbox.curr, ntext + g->edbox.curr, len - g->edbox.curr + 1);
	memcpy(ntext + g->edbox.curr, text, ntl);
	
	g->edbox.sel_start = 0;
	g->edbox.sel_end = 0;
	edbox_creset(g);
	edbox_goto_index(g, g->edbox.curr + ntl);
	
	if (g->edbox.edit)
		g->edbox.edit(g);
	
	return 0;
}

int edbox_sel_set(struct gadget *g, int start, int end)
{
	int len = strlen(g->text);
	int start0_l;
	int start1_l;
	int end0_l;
	int end1_l;
	int pstart;
	int pend;
	int i;
	
	if (start < 0 || start > len || end < 0 || end > len)
	{
		_set_errno(EINVAL);
		return -1;
	}
	
	if (end < start)
	{
		int tmp;
		
		tmp   = end;
		end   = start;
		start = tmp;
	}
	
	pstart = g->edbox.sel_start;
	pend   = g->edbox.sel_end;
	
	edbox_indexlc(g, pstart, &start0_l, &i);
	edbox_indexlc(g, pend,	 &end0_l,   &i);
	edbox_indexlc(g, start,	 &start1_l, &i);
	edbox_indexlc(g, end,	 &end1_l,   &i);
	
	g->edbox.sel_start = start;
	g->edbox.sel_end   = end;
	
	if (pstart != pend)
	{
		if (start0_l != start1_l || end0_l != end1_l)
		{
			gadget_redraw(g);
			return 0;
		}
	}
	else
		if (start1_l != end1_l)
		{
			gadget_redraw(g);
			return 0;
		}
	
	if (pstart != start)
		edbox_redraw_line(g, g->form->wd, start1_l);
	
	if (pend != end)
		edbox_redraw_line(g, g->form->wd, end1_l);
	
	return 0;
}

int edbox_sel_get(struct gadget *g, int *start, int *end)
{
	*start	= g->edbox.sel_start;
	*end	= g->edbox.sel_end;
	return 0;
}

int edbox_sel_delete(struct gadget *g)
{
	int len = strlen(g->text);
	int ss = g->edbox.sel_start;
	int se = g->edbox.sel_end;
	char *text = g->text;
	
	if (ss == se)
		return 0;
	
	memmove(text + ss, text + se, len - se + 1);
	
	text = realloc(text, len - se + ss + 1);
	if (text)
		g->text = text;
	
	g->edbox.sel_start = 0;
	g->edbox.sel_end   = 0;
	edbox_creset(g);
	edbox_goto_index(g, ss);
	
	if (g->edbox.edit)
		g->edbox.edit(g);
	
	return 0;
}
