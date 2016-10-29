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

#include <config/defaults.h>
#include <priv/wingui_form_cfg.h>
#include <priv/wingui_form.h>
#include <priv/wingui_theme.h>
#include <priv/libc.h>
#include <kern/wingui.h>
#include <wingui_metrics.h>
#include <wingui_color.h>
#include <wingui_menu.h>
#include <wingui_form.h>
#include <wingui.h>
#include <confdb.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <event.h>
#include <list.h>

struct list	form_deferred;
struct list	form_list;
int		form_cfg_loaded;
struct form_cfg	form_cfg;

#define BTN_ST_NORMAL	0
#define BTN_ST_DEPRESD	1
#define BTN_ST_INACTIVE	2

#define CBTN_CLOSE	0
#define CBTN_ZOOM	1
#define CBTN_MINI	2

#define min(a, b)	((a) < (b) ? (a) : (b))
#define max(a, b)	((a) > (b) ? (a) : (b))

static void form_xygadget(struct form *f, struct gadget **buf, int x, int y);
static int  form_menu_xyitem(struct form *f, int x, int y);
static void form_draw_closebtn(struct form *f);
static void form_draw_zoombtn(struct form *f);
static void form_draw_minibtn(struct form *f);
static void form_draw_menubtn(struct form *f);
static void form_draw_frame(struct form *f);
static void form_draw_title(struct form *f);
static void form_draw_decoration_now(struct form *f, int set_clip);
static void form_draw_decoration(struct form *f);
static void form_draw_menu(struct form *f, int set_clip);
static void form_setup_rects(struct form *f, int w, int h);
static void form_menu_do_option(struct form *f);
static int  form_kbd_menu_event(struct event *e);
static int  form_menu_event(struct event *e);
static int  form_resize_event(struct event *e);
static int  form_close_event(struct event *e);
static void form_control_popup(struct form *f);
static int  form_control_menu_event(struct event *e);
static void form_minimize(struct form *f);
static void form_zoom(struct form *f);
static void form_cbtn_uppa(struct form *f, int cbtn, int x, int y);
static int  form_zoom_event(struct event *e);
static int  form_move_event(struct event *e);
static void form_update_ptr_now(struct form *f);
static void form_update_ptr(struct form *f);
static void form_update_pos(struct form *form);
static int  form_event(struct event *e);
static int  form_redraw_rect(struct form *f, int x, int y, int w, int h);
#define form_is_active(f)	(((f)->active || (f)->focused) && !(f)->child_dialog)
static void form_activate(struct form *f, int activate);
static void form_defer(struct form *f);
static void form_umini(struct form *form);

static void form_xygadget(struct form *f, struct gadget **buf, int x, int y)
{
	struct gadget *g;
	
	for (g = f->gadget; g; g = g->next)
	{
		if (x < g->rect.x)
			continue;
		
		if (y < g->rect.y)
			continue;
		
		if (x >= g->rect.x + g->rect.w)
			continue;
		
		if (y >= g->rect.y + g->rect.h)
			continue;
		
		*buf = g;
		return;
	}
	
	*buf = NULL;
	return;
}

static int form_menu_xyitem(struct form *f, int x, int y)
{
	struct menu *m = f->menu;
	struct menu_item *im;
	int i;
	
	for (i = 0; i < m->item_count; i++)
	{
		im = &m->item[i];
		
		if (x - f->menu_rect.x >= im->rect.x		  &&
		    x - f->menu_rect.x <  im->rect.x + im->rect.w &&
		    y - f->menu_rect.y >= im->rect.y		  &&
		    y - f->menu_rect.y <  im->rect.y + im->rect.h)
			return i;
	}
	return -1;
}

void form_draw_rect3d(int wd, int x, int y, int w, int h,
		      win_color hi1, win_color hi2,
		      win_color sh1, win_color sh2,
		      win_color bg)
{
	int tl = wm_get(WM_THIN_LINE);
	
	win_rect(wd, hi1, x, y, w, tl);
	win_rect(wd, hi1, x, y, tl, h);
	win_rect(wd, sh1, x, y + h - tl, w, tl);
	win_rect(wd, sh1, x + w - tl, y, tl, h);
	
	win_rect(wd, hi2, x + tl, y + tl, w - 2 * tl, tl);
	win_rect(wd, hi2, x + tl, y + tl, tl, h - 2 * tl);
	win_rect(wd, sh2, x + tl, y + h - 2 * tl, w - 2 * tl, tl);
	win_rect(wd, sh2, x + w - 2 * tl, y + tl, tl, h - 2 * tl);
	
	win_rect(wd, bg, x + 2 * tl, y + 2 * tl, w - 4 * tl, h - 4 * tl);
}

static void form_draw_btn_bg(struct form *f, int x, int y, int w, int h, int st)
{
	win_color sh1, sh2;
	win_color hi1, hi2;
	win_color bg;
	
	switch (st)
	{
	case BTN_ST_NORMAL:
		sh1 = wc_get(WC_TITLE_BTN_SH1);
		sh2 = wc_get(WC_TITLE_BTN_SH2);
		hi1 = wc_get(WC_TITLE_BTN_HI1);
		hi2 = wc_get(WC_TITLE_BTN_HI2);
		bg  = wc_get(WC_TITLE_BTN_BG);
		break;
	case BTN_ST_DEPRESD:
		sh1 = wc_get(WC_TITLE_BTN_HI1);
		sh2 = wc_get(WC_TITLE_BTN_HI2);
		hi1 = wc_get(WC_TITLE_BTN_SH1);
		hi2 = wc_get(WC_TITLE_BTN_SH2);
		bg  = wc_get(WC_TITLE_BTN_BG);
		break;
	case BTN_ST_INACTIVE:
		sh1 = wc_get(WC_INACT_TITLE_BTN_SH1);
		sh2 = wc_get(WC_INACT_TITLE_BTN_SH2);
		hi1 = wc_get(WC_INACT_TITLE_BTN_HI1);
		hi2 = wc_get(WC_INACT_TITLE_BTN_HI2);
		bg  = wc_get(WC_INACT_TITLE_BTN_BG);
		break;
	default:
		abort();
	}
	form_draw_rect3d(f->wd, x, y, w, h, hi1, hi2, sh1, sh2, bg);
}

static void form_draw_closebtn(struct form *f)
{
	int x = f->closebtn_rect.x;
	int y = f->closebtn_rect.y;
	int w = f->closebtn_rect.w;
	int h = f->closebtn_rect.h;
	const struct form_theme *th;
	int tl = wm_get(WM_THIN_LINE);
	int xt = tl * 2;
	win_color fg;
	int size;
	int st;
	int i;
	
	if (!(f->flags & FORM_ALLOW_CLOSE))
		return;
	
	th = form_th_get();
	if (th != NULL && th->d_closebtn != NULL)
	{
		int fl = 0;
		
		if (form_is_active(f))
			fl |= DB_FOCUS;
		if (f->closing && f->title_btn_pointed_at)
			fl |= DB_DEPSD;
		
		th->d_closebtn(f->wd, x, y, w, h, fl);
		return;
	}
	
	if (form_is_active(f))
	{
		if (f->closing && f->title_btn_pointed_at)
			st = BTN_ST_DEPRESD;
		else
			st = BTN_ST_NORMAL;
		fg = wc_get(WC_TITLE_BTN_FG);
	}
	else
	{
		fg = wc_get(WC_INACT_TITLE_BTN_FG);
		st = BTN_ST_INACTIVE;
	}
	
	form_draw_btn_bg(f, x, y, w, h, st);
	
	if (w < h)
		size = w;
	else
		size = h;
	
	for (i = 4 * tl; i <= size - 4 * tl - xt; i++)
	{
		win_rect(f->wd, fg, x + size - i - xt, y + i, xt, xt);
		win_rect(f->wd, fg, x + i,	       y + i, xt, xt);
	}
}

static void form_draw_zoombtn(struct form *f)
{
	int x = f->zoombtn_rect.x;
	int y = f->zoombtn_rect.y;
	int w = f->zoombtn_rect.w;
	int h = f->zoombtn_rect.h;
	const struct form_theme *th;
	int tl = wm_get(WM_THIN_LINE);
	win_color fg;
	int st;
	
	if (!(f->flags & FORM_ALLOW_RESIZE))
		return;
	
	if (!w || !h)
		return;
	
	th = form_th_get();
	if (th != NULL && th->d_closebtn != NULL)
	{
		int fl = 0;
		
		if (form_is_active(f))
			fl |= DB_FOCUS;
		if (f->zooming && f->title_btn_pointed_at)
			fl |= DB_DEPSD;
		
		th->d_zoombtn(f->wd, x, y, w, h, fl);
		return;
	}
	
	if (form_is_active(f))
	{
		if (f->zooming && f->title_btn_pointed_at)
			st = BTN_ST_DEPRESD;
		else
			st = BTN_ST_NORMAL;
		fg = wc_get(WC_TITLE_BTN_FG);
	}
	else
	{
		fg = wc_get(WC_INACT_TITLE_BTN_FG);
		st = BTN_ST_INACTIVE;
	}
	
	form_draw_btn_bg(f, x, y, w, h, st);
	
	win_frame7(f->wd, fg, x + 4 * tl, y + 4 * tl, w - 8 * tl, h - 8 * tl, tl * 2);
}

static void form_draw_minibtn(struct form *f)
{
	int x = f->minibtn_rect.x;
	int y = f->minibtn_rect.y;
	int w = f->minibtn_rect.w;
	int h = f->minibtn_rect.h;
	const struct form_theme *th;
	int tl = wm_get(WM_THIN_LINE);
	int size;
	int spc;
	win_color fg;
	int st;
	
	if (!(f->flags & FORM_ALLOW_MINIMIZE))
		return;
	
	if (!w || !h)
		return;
	
	if (w < h)
		size = w;
	else
		size = h;
	
	spc  = size - 5 * tl;
	spc /= 2;
	
	th = form_th_get();
	if (th != NULL && th->d_closebtn != NULL)
	{
		int fl = 0;
		
		if (form_is_active(f))
			fl |= DB_FOCUS;
		if (f->minimizing && f->title_btn_pointed_at)
			fl |= DB_DEPSD;
		
		th->d_minibtn(f->wd, x, y, w, h, fl);
		return;
	}
	
	if (form_is_active(f))
	{
		if (f->minimizing && f->title_btn_pointed_at)
			st = BTN_ST_DEPRESD;
		else
			st = BTN_ST_NORMAL;
		fg = wc_get(WC_TITLE_BTN_FG);
	}
	else
	{
		fg = wc_get(WC_INACT_TITLE_BTN_FG);
		st = BTN_ST_INACTIVE;
	}
	
	form_draw_btn_bg(f, x, y, w, h, st);
	
	win_frame7(f->wd, fg, x + spc, y + spc, w - 2 * spc, h - 2 * spc, tl * 2);
}

static void form_draw_menubtn(struct form *f)
{
	int x = f->menubtn_rect.x;
	int y = f->menubtn_rect.y;
	int w = f->menubtn_rect.w;
	int h = f->menubtn_rect.h;
	int tl = wm_get(WM_THIN_LINE);
	const struct form_theme *th;
	win_color fg;
	int st;
	
	if (!w || !h)
		return;
	
	th = form_th_get();
	if (th != NULL && th->d_closebtn != NULL)
	{
		int fl = 0;
		
		if (form_is_active(f))
			fl |= DB_FOCUS;
		if (f->control_menu_active || f->pulling_menu)
			fl |= DB_DEPSD;
		
		th->d_menubtn(f->wd, x, y, w, h, fl);
		return;
	}
	
	if (f->control_menu_active || f->pulling_menu)
	{
		st = BTN_ST_DEPRESD;
		fg = wc_get(WC_TITLE_BTN_FG);
	}
	else if (form_is_active(f))
	{
		st = BTN_ST_NORMAL;
		fg = wc_get(WC_TITLE_BTN_FG);
	}
	else
	{
		fg = wc_get(WC_INACT_TITLE_BTN_FG);
		st = BTN_ST_INACTIVE;
	}
	
	form_draw_btn_bg(f, x, y, w, h, st);
	
	win_rect(f->wd, fg, x + 4 * tl, y + h / 2 - 2 * tl, w - 8 * tl, tl);
	win_rect(f->wd, fg, x + 4 * tl, y + h / 2 + 2 * tl, w - 8 * tl, tl);
	win_rect(f->wd, fg, x + 4 * tl, y + h / 2,	    w - 8 * tl, tl);
}

static void form_draw_frame(struct form *f)
{
	win_color sh1 = wc_get(WC_FRAME_SH1);
	win_color sh2 = wc_get(WC_FRAME_SH2);
	win_color hi1 = wc_get(WC_FRAME_HI1);
	win_color hi2 = wc_get(WC_FRAME_HI2);
	win_color bg = wc_get(WC_FRAME_BG);
	const struct form_theme *th;
	int fw = f->frame_width + f->shadow_width;
	int w = f->win_rect.w;
	int h = f->win_rect.h;
	
	if (!(f->flags & FORM_FRAME))
		return;
	
	th = form_th_get();
	if (th != NULL && th->d_formframe)
	{
		th->d_formframe(f->wd, 0, 0, w, h, fw, form_is_active(f));
		return;
	}
	
	if (!form_is_active(f))
	{
		sh1 = wc_get(WC_INACT_FRAME_SH1);
		sh2 = wc_get(WC_INACT_FRAME_SH2);
		hi1 = wc_get(WC_INACT_FRAME_HI1);
		hi2 = wc_get(WC_INACT_FRAME_HI2);
		bg  = wc_get(WC_INACT_FRAME_BG);
	}
	
	win_rect(f->wd, bg, 0,	    0,	    w,  fw);
	win_rect(f->wd, bg, 0,	    0,	    fw, h);
	win_rect(f->wd, bg, 0,	    h - fw, w,  fw);
	win_rect(f->wd, bg, w - fw, 0,	    fw, h);
	
	win_hline(f->wd, hi1, 0,     0,	    w);
	win_hline(f->wd, sh1, 0,     h - 1, w);
	win_vline(f->wd, hi1, 0,     0,	    h);
	win_vline(f->wd, sh1, w - 1, 0,	    h);
	
	win_hline(f->wd, sh1, fw - 1, fw - 1, w - 2 * fw + 2);
	win_hline(f->wd, hi1, fw - 1, h - fw, w - 2 * fw + 2);
	win_vline(f->wd, sh1, fw - 1, fw - 1, h - 2 * fw + 2);
	win_vline(f->wd, hi1, w - fw, fw - 1, h - 2 * fw + 2);
	
	win_hline(f->wd, hi2, 1,     1,	    w - 2);
	win_hline(f->wd, sh2, 1,     h - 2, w - 2);
	win_vline(f->wd, hi2, 1,     1,	    h - 2);
	win_vline(f->wd, sh2, w - 2, 1,	    h - 2);
	
	win_hline(f->wd, sh2, fw - 2,     fw - 2,     w - 2 * fw + 4);
	win_hline(f->wd, hi2, fw - 2,     h - fw + 1, w - 2 * fw + 4);
	win_vline(f->wd, sh2, fw - 2,     fw - 2,     h - 2 * fw + 4);
	win_vline(f->wd, hi2, w - fw + 1, fw - 2,     h - 2 * fw + 4);
}

static void form_draw_title(struct form *f)
{
	win_color hi1, hi2;
	win_color sh1, sh2;
	win_color bg;
	win_color fg;
	int text_w;
	int text_h;
	
	if (!(f->flags & FORM_TITLE))
		return;
	
	if (form_is_active(f))
	{
		if (f->moving)
		{
			sh1 = wc_get(WC_TITLE_HI1);
			sh2 = wc_get(WC_TITLE_HI2);
			hi1 = wc_get(WC_TITLE_SH1);
			hi2 = wc_get(WC_TITLE_SH2);
		}
		else
		{
			hi1 = wc_get(WC_TITLE_HI1);
			hi2 = wc_get(WC_TITLE_HI2);
			sh1 = wc_get(WC_TITLE_SH1);
			sh2 = wc_get(WC_TITLE_SH2);
		}
		bg = wc_get(WC_TITLE_BG);
		fg = wc_get(WC_TITLE_FG);
	}
	else
	{
		hi1 = wc_get(WC_INACT_TITLE_HI1);
		hi2 = wc_get(WC_INACT_TITLE_HI2);
		sh1 = wc_get(WC_INACT_TITLE_SH1);
		sh2 = wc_get(WC_INACT_TITLE_SH2);
		bg  = wc_get(WC_INACT_TITLE_BG);
		fg  = wc_get(WC_INACT_TITLE_FG);
	}
	
	form_draw_rect3d(f->wd,
			 f->title_rect.x, f->title_rect.y,
			 f->title_rect.w, f->title_rect.h,
			 hi1, hi2, sh1, sh2, bg);
	
	win_set_font(f->wd, WIN_FONT_DEFAULT);
	win_text_size(WIN_FONT_DEFAULT, &text_w, &text_h, f->title);
	
	if (f->title_rect.w >= text_w + 2)
		win_text(f->wd,
			 fg,
			 f->title_rect.x + (f->title_rect.w - text_w) / 2,
			 f->title_rect.y + (f->title_rect.h - text_h) / 2,
			 f->title);
}

static void form_draw_decoration_now(struct form *f, int set_clip)
{
	if (set_clip)
		win_clip(f->wd, 0, 0, f->win_rect.w, f->win_rect.h, 0, 0);
	
	win_paint();
	form_draw_frame(f);
	form_draw_title(f);
	form_draw_closebtn(f);
	form_draw_zoombtn(f);
	form_draw_minibtn(f);
	form_draw_menubtn(f);
	win_end_paint();
}

static void form_draw_decoration(struct form *f)
{
	f->defop.draw_closebtn	= 1;
	f->defop.draw_zoombtn	= 1;
	f->defop.draw_menubtn	= 1;
	f->defop.draw_minibtn	= 1;
	f->defop.draw_frame	= 1;
	f->defop.draw_title	= 1;
	form_defer(f);
}

static void form_draw_menu(struct form *f, int set_clip)
{
	struct menu *m = f->menu;
	struct menu_item *im;
	int tl;
	int wd = f->wd;
	win_color s_bg;
	win_color s_fg;
	win_color bg;
	win_color fg;
	win_color hi;
	win_color sh;
	win_color ul;
	int x = f->menu_rect.x;
	int y = f->menu_rect.y;
	int i;
	
	if (!f->menu)
		return;
	
	if (set_clip)
		win_clip(f->wd, 0, 0, f->win_rect.w, f->win_rect.h, 0, 0);
	win_set_font(f->wd, WIN_FONT_DEFAULT);
	
	s_bg = wc_get(WC_SEL_BG);
	s_fg = wc_get(WC_SEL_FG);
	bg   = wc_get(WC_WIN_BG);
	fg   = wc_get(WC_WIN_FG);
	hi   = wc_get(WC_HIGHLIGHT1);
	sh   = wc_get(WC_SHADOW1);
	ul   = wc_get(WC_MENU_UNDERLINE);
	
	tl = wm_get(WM_THIN_LINE);
	
	win_paint();
	win_rect(wd, bg, x, y, f->menu_rect.w, f->menu_rect.h - 1);
	win_rect(wd, ul, x, y + f->menu_rect.h - tl, f->menu_rect.w, tl);
	
	for (i = 0; i < m->item_count; i++)
	{
		im = &m->item[i];
		
		if (strcmp(im->text, "-"))
		{
			if (i == m->selection /* && f->menu_active */)
			{
				win_rect(wd, s_bg, x + im->rect.x,     y + im->rect.y,	   im->rect.w, im->rect.h);
				win_text(wd, s_fg, x + im->rect.x + 4 * tl, y + im->rect.y + tl, im->text);
			}
			else
				win_text(wd, fg, x + im->rect.x + 4 * tl, y + im->rect.y + tl, im->text);
		}
		else
		{
			win_rect(wd, bg, x + im->rect.x, y + im->rect.y, im->rect.w, im->rect.h);
			win_rect(wd, sh, x + im->rect.x +     tl, y + im->rect.y, tl, im->rect.h);
			win_rect(wd, hi, x + im->rect.x + 2 * tl, y + im->rect.y, tl, im->rect.h);
		}
	}
	win_end_paint();
}

static void form_setup_rects(struct form *f, int w, int h)
{
	int font_w;
	int font_h;
	
	win_set_font(f->wd, WIN_FONT_DEFAULT);
	win_text_size(WIN_FONT_DEFAULT, &font_w, &font_h, "X");
	
	f->frame_width = 0;
	
	f->win_rect.w = w;
	f->win_rect.h = h;
	
	f->title_rect.x = 0;
	f->title_rect.y = 0;
	f->title_rect.w = w;
	f->title_rect.h = 0;
	
	f->workspace_rect.x = 0;
	f->workspace_rect.y = 0;
	f->workspace_rect.w = w;
	f->workspace_rect.h = h;
	
	f->closebtn_rect.x = 0;
	f->closebtn_rect.y = 0;
	f->closebtn_rect.w = 0;
	f->closebtn_rect.h = 0;
	
	f->zoombtn_rect.x = 0;
	f->zoombtn_rect.y = 0;
	f->zoombtn_rect.w = 0;
	f->zoombtn_rect.h = 0;
	
	f->minibtn_rect.x = 0;
	f->minibtn_rect.y = 0;
	f->minibtn_rect.w = 0;
	f->minibtn_rect.h = 0;
	
	f->menu_rect.x = 0;
	f->menu_rect.y = 0;
	f->menu_rect.w = 0;
	f->menu_rect.h = 0;
	
	if (f->flags & FORM_FRAME)
	{
		f->frame_width = wm_get(WM_FRAME);
		
		f->win_rect.w += 2 * f->frame_width;
		f->win_rect.h += 2 * f->frame_width;
		
		f->workspace_rect.x += f->frame_width;
		f->workspace_rect.y += f->frame_width;
		
		f->title_rect.x += f->frame_width;
		f->title_rect.y += f->frame_width;
	}
	
	if (f->shadow_width)
	{
		f->win_rect.w += 2 * f->shadow_width;
		f->win_rect.h += 2 * f->shadow_width;
		
		f->workspace_rect.x += f->shadow_width;
		f->workspace_rect.y += f->shadow_width;
		
		f->title_rect.x += f->shadow_width;
		f->title_rect.y += f->shadow_width;
	}
	
	if (f->flags & FORM_TITLE)
	{
		int tl = wm_get(WM_THIN_LINE);
		
		f->title_rect.h = font_h + 6 * tl;
		
		f->workspace_rect.y += f->title_rect.h;
		f->win_rect.h += f->title_rect.h;
		
		f->menubtn_rect.x = f->title_rect.x;
		f->menubtn_rect.y = f->title_rect.y;
		if (f->title_rect.w >= f->title_rect.h)
			f->menubtn_rect.w = f->title_rect.h;
		else
			f->menubtn_rect.w = f->title_rect.w;
		f->menubtn_rect.h = f->title_rect.h;
		
		f->title_rect.w -= f->menubtn_rect.w;
		f->title_rect.x += f->menubtn_rect.w;
		
		if ((f->flags & FORM_ALLOW_CLOSE) && f->title_rect.w >= f->title_rect.h)
		{
			f->title_rect.w -= f->title_rect.h;
			
			f->closebtn_rect.x = f->title_rect.x + f->title_rect.w;
			f->closebtn_rect.y = f->title_rect.y;
			f->closebtn_rect.w = f->title_rect.h;
			f->closebtn_rect.h = f->title_rect.h;
		}
		
		if ((f->flags & FORM_ALLOW_RESIZE) && f->title_rect.w >= f->title_rect.h)
		{
			f->title_rect.w -= f->title_rect.h;
			
			f->zoombtn_rect.x = f->title_rect.x + f->title_rect.w;
			f->zoombtn_rect.y = f->title_rect.y;
			f->zoombtn_rect.w = f->title_rect.h;
			f->zoombtn_rect.h = f->title_rect.h;
		}
		
		if ((f->flags & FORM_ALLOW_MINIMIZE) && f->title_rect.w >= f->title_rect.h)
		{
			f->title_rect.w -= f->title_rect.h;
			
			f->minibtn_rect.x = f->title_rect.x + f->title_rect.w;
			f->minibtn_rect.y = f->title_rect.y;
			f->minibtn_rect.w = f->title_rect.h;
			f->minibtn_rect.h = f->title_rect.h;
		}
	}
	
	if (f->menu)
	{
		int tl = wm_get(WM_THIN_LINE);
		
		f->menu_rect.x = f->workspace_rect.x;
		f->menu_rect.y = f->workspace_rect.y;
		f->menu_rect.w = f->workspace_rect.w;
		f->menu_rect.h = font_h + 3 * tl;
		
		f->workspace_rect.y += f->menu_rect.h;
		f->win_rect.h += f->menu_rect.h;
	}
}

static void form_menu_do_option(struct form *f)
{
	struct menu *m = f->menu;
	struct menu_item *im;
	int mx, my;
	
	if (m->selection < 0)
		return;
	im = &m->item[m->selection];
	
	if (im->submenu)
	{
		mx = f->win_rect.x + f->menu_rect.x + im->rect.x;
		my = f->win_rect.y + f->menu_rect.y + f->menu_rect.h - 1;
		form_activate(f, 1);
		form_newref(f);
		menu_popup_l(im->submenu, mx, my, f->layer);
		form_putref(f);
		form_activate(f, 0);
	}
	
	if (im->proc)
	{
		f->locked++;
		im->proc(im);
		f->locked--;
	}
}

static int form_kbd_menu_event(struct event *e)
{
	struct form *f = e->data;
	struct menu *m = f->menu;
	int i;
	
	if (e->win.type != WIN_E_KEY_DOWN)
		return 0;
	
	switch (e->win.ch)
	{
		case WIN_KEY_LEFT:
			if (!m->selection)
				break;
			
			i = m->selection - 1;
			while (i && !strcmp(m->item[i].text, "-"))
				i--;
			m->selection = i;
			form_draw_menu(f, 1);
			break;
		
		case WIN_KEY_RIGHT:
			if (m->selection >= m->item_count - 1)
				break;
			
			i = m->selection + 1;
			while (i < m->item_count - 1 && !strcmp(m->item[i].text, "-"))
				i++;
			m->selection = i;
			form_draw_menu(f, 1);
			break;
		
		case 0x1b:
			f->kbd_menu_active = 0;
			m->selection = -1;
			form_draw_menu(f, 1);
			break;
		
		case '\n':
			form_menu_do_option(f);
			f->kbd_menu_active = 0;
			m->selection = -1;
			form_draw_menu(f, 1);
			break;
	}
	return 0;
}

static int form_menu_event(struct event *e)
{
	struct form *f = e->data;
	struct menu *m = f->menu;
	int x = e->win.ptr_x;
	int y = e->win.ptr_y;
	int i;
	
	switch (e->win.type)
	{
	case WIN_E_PTR_DOWN:
		if (f->kbd_menu_active)
			break;
		f->menu_active++;
		
		i = form_menu_xyitem(f, x, y);
		if (i < 0)
			break;
		
		m->selection = i;
		form_draw_menu(f, 1);
		break;
	
	case WIN_E_PTR_UP:
		if (!f->menu_active)
			break;
		f->menu_active--;
		if (f->menu_active)
			break;
		
		i = form_menu_xyitem(f, x, y);
		if (i >= 0)
		{
			m->selection = i;
			form_draw_menu(f, 1);
			form_menu_do_option(f);
		}
		m->selection = -1;
		form_draw_menu(f, 1);
		break;
	
	case WIN_E_PTR_MOVE:
		if (!f->menu_active)
			break;
		
		i = form_menu_xyitem(f, x, y);
		if (i < 0)
			break;
		
		m->selection = i;
		form_draw_menu(f, 1);
		break;
	}
	
	return 0;
}

static int form_resize_event(struct event *e)
{
	struct form *f = e->data;
	int w, h;
	
	if (f->zoomed)
		return 0;
	
	switch (e->win.type)
	{
	case WIN_E_PTR_DOWN:
		f->resizing++;
		break;
	case WIN_E_PTR_MOVE:
		if (e->win.more)
			break;
		
		w = e->win.ptr_x - f->workspace_rect.x - f->frame_width;
		h = e->win.ptr_y - f->workspace_rect.y - f->frame_width;
		
		if (w < 1)
			w = 1;
		if (h < 1)
			h = 1;
		
		win_rect_preview(1, f->win_rect.x,
				    f->win_rect.y,
				    w + f->workspace_rect.x + f->frame_width + 1,
				    h + f->workspace_rect.y + f->frame_width + 1);
		break;
	case WIN_E_PTR_UP:
		w = e->win.ptr_x - f->workspace_rect.x - f->frame_width + 1;
		h = e->win.ptr_y - f->workspace_rect.y - f->frame_width + 1;
		
		if (w < 1)
			w = 1;
		if (h < 1)
			h = 1;
		
		f->resizing--;
		win_rect_preview(0, 0, 0, 0, 0);
		form_resize(f, w, h);
		break;
	}
	
	return 0;
}

static int form_close_event(struct event *e)
{
	struct form *f = e->data;
	int x = e->win.ptr_x;
	int y = e->win.ptr_y;
	
	switch (e->win.type)
	{
	case WIN_E_PTR_DOWN:
		form_cbtn_uppa(f, CBTN_CLOSE, x, y);
		if (!f->closing++)
		{
			win_clip(f->wd, 0, 0, f->win_rect.w, f->win_rect.h, 0, 0);
			win_paint();
			form_draw_closebtn(f);
			win_end_paint();
		}
		break;
	case WIN_E_PTR_UP:
		form_cbtn_uppa(f, CBTN_CLOSE, x, y);
		if (!--f->closing)
		{
			win_clip(f->wd, 0, 0, f->win_rect.w, f->win_rect.h, 0, 0);
			win_paint();
			form_draw_closebtn(f);
			win_end_paint();
			
			if (f->title_btn_pointed_at)
				form_close(f);
		}
		break;
	case WIN_E_PTR_MOVE:
		form_cbtn_uppa(f, CBTN_CLOSE, x, y);
		break;
	}
	
	return 0;
}

static void form_control_switch(struct menu_item *mi)
{
	win_focus(mi->l_data);
	win_raise(mi->l_data);
}

static void form_control_close(struct menu_item *mi)
{
	form_close(mi->p_data);
}

static void form_control_zoom(struct menu_item *mi)
{
	form_zoom(mi->p_data);
}

static void form_control_mini(struct menu_item *mi)
{
	form_minimize(mi->p_data);
}

static void form_control_popup(struct form *f)
{
	struct menu *m;
	int se;
	int i;
	
	form_activate(f, 1);
	form_newref(f);
	f->control_menu_active++;
	win_clip(f->wd, 0, 0, f->win_rect.w, f->win_rect.h, 0, 0);
	win_paint();
	form_draw_menubtn(f);
	win_end_paint();
	
	m = menu_creat();
	if (f->flags & FORM_ALLOW_MINIMIZE)
		menu_newitem5(m, "Minimize", WIN_KEY_F2, WIN_SHIFT_ALT, form_control_mini)->p_data = f;
	if (f->flags & FORM_ALLOW_RESIZE)
		menu_newitem5(m, "Zoom", WIN_KEY_F3, WIN_SHIFT_ALT, form_control_zoom)->p_data = f;
	if (f->flags & FORM_ALLOW_CLOSE)
		menu_newitem5(m, "Close", WIN_KEY_F4, WIN_SHIFT_ALT, form_control_close)->p_data = f;
	
	se = _get_errno();
	if (win_chk_taskbar())
	{
		if (m->item_count)
			menu_newitem(m, "-", NULL);
		
		for (i = 0; i < WIN_MAX; i++)
		{
			char title[WIN_TITLE_MAX + 1];
			int vi;
			
			if (win_is_visible(i, &vi))
				continue;
			if (!vi)
				continue;
			if (win_get_title(i, title, sizeof title))
				continue;
			if (!*title)
				continue;
			
			menu_newitem(m, title, form_control_switch)->l_data = i;
		}
	}
	_set_errno(se);
	
	if (!m->item_count)
		menu_newitem(m, "(no items)", NULL);
	
	menu_popup_l(m,
		     f->win_rect.x + f->menubtn_rect.x,
		     f->win_rect.y + f->menubtn_rect.y + f->menubtn_rect.h,
		     f->layer);
	menu_free(m);
	
	win_idle();
	f->control_menu_active--;
	f->defop.draw_menubtn = 1;
	form_defer(f);
	form_activate(f, 0);
	form_putref(f);
}

static int form_control_menu_event(struct event *e)
{
	struct form *f = e->data;
	
	switch (e->win.type)
	{
	case WIN_E_PTR_DOWN:
		f->pulling_menu++;
		
		win_clip(f->wd, 0, 0, f->win_rect.w, f->win_rect.h, 0, 0);
		win_paint();
		form_draw_menubtn(f);
		win_end_paint();
		
		form_control_popup(f);
		break;
	case WIN_E_PTR_UP:
		f->pulling_menu--;
		
		win_clip(f->wd, 0, 0, f->win_rect.w, f->win_rect.h, 0, 0);
		win_paint();
		form_draw_menubtn(f);
		win_end_paint();
		break;
	}
	
	return 0;
}

static void form_do_zoom(struct form *f, struct win_rect *wsr)
{
	int pw, ph;
	int w, h;
	
	pw = f->win_rect.w;
	ph = f->win_rect.h;
	f->win_rect.x = wsr->x - f->frame_width;
	f->win_rect.y = wsr->y - f->frame_width;
	w  = wsr->w - f->win_rect.w + f->workspace_rect.w;
	h  = wsr->h - f->win_rect.h + f->workspace_rect.h;
	w += f->frame_width * 2;
	h += f->frame_width * 2;
	f->bg_rects_valid = 0;
	f->zoomed = 1;
	form_setup_rects(f, w, h);
	
	form_update_pos(f);
	
	if (f->resize)
		f->resize(f, w, h);
	if (f->move)
		f->move(f, f->win_rect.x, f->win_rect.y);
}

static void form_zoom(struct form *f)
{
	int pw, ph;
	
	form_newref(f);
	if (f->zoomed)
	{
		pw = f->win_rect.w;
		ph = f->win_rect.h;
		
		f->win_rect.x = f->saved_rect.x;
		f->win_rect.y = f->saved_rect.y;
		f->bg_rects_valid = 0;
		f->zoomed = 0;
		form_setup_rects(f, f->saved_rect.w, f->saved_rect.h);
		
		form_update_pos(f);
		if (f->resize)
			f->resize(f, f->workspace_rect.w, f->workspace_rect.h);
		if (f->move)
			f->move(f, f->win_rect.x, f->win_rect.y);
	}
	else
	{
		struct win_rect wsr;
		
		f->saved_rect.x = f->win_rect.x;
		f->saved_rect.y = f->win_rect.y;
		f->saved_rect.w = f->workspace_rect.w;
		f->saved_rect.h = f->workspace_rect.h;
		
		win_ws_getrect(&wsr);
		form_do_zoom(f, &wsr);
	}
	form_putref(f);
}

static void form_cbtn_uppa(struct form *f, int cbtn, int x, int y)
{
	void (*rp)(struct form *f);
	struct win_rect *r;
	int pa;
	
	switch (cbtn)
	{
	case CBTN_CLOSE:
		rp = form_draw_closebtn;
		r  = &f->closebtn_rect;
		break;
	case CBTN_ZOOM:
		rp = form_draw_zoombtn;
		r  = &f->zoombtn_rect;
		break;
	case CBTN_MINI:
		rp = form_draw_minibtn;
		r  = &f->minibtn_rect;
		break;
	default:
		abort();
	}
	
	pa = win_inrect(r, x, y);
	if (f->title_btn_pointed_at == pa)
		return;
	
	f->title_btn_pointed_at = pa;
	
	win_clip(f->wd, 0, 0, f->win_rect.w, f->win_rect.h, 0, 0);
	win_paint();
	rp(f);
	win_end_paint();
}

static int form_zoom_event(struct event *e)
{
	struct form *f = e->data;
	int x = e->win.ptr_x;
	int y = e->win.ptr_y;
	
	switch (e->win.type)
	{
	case WIN_E_PTR_DOWN:
		form_cbtn_uppa(f, CBTN_ZOOM, x, y);
		if (!f->zooming++)
		{
			win_clip(f->wd, 0, 0, f->win_rect.w, f->win_rect.h, 0, 0);
			win_paint();
			form_draw_zoombtn(f);
			win_end_paint();
		}
		break;
	case WIN_E_PTR_UP:
		form_cbtn_uppa(f, CBTN_ZOOM, x, y);
		if (!--f->zooming)
		{
			win_clip(f->wd, 0, 0, f->win_rect.w, f->win_rect.h, 0, 0);
			win_paint();
			form_draw_zoombtn(f);
			win_end_paint();
			
			if (f->title_btn_pointed_at)
				form_zoom(f);
		}
		break;
	case WIN_E_PTR_MOVE:
		form_cbtn_uppa(f, CBTN_ZOOM, x, y);
		break;
	}
	
	return 0;
}

static int form_mini_event(struct event *e)
{
	struct form *f = e->data;
	int x = e->win.ptr_x;
	int y = e->win.ptr_y;
	
	switch (e->win.type)
	{
	case WIN_E_PTR_DOWN:
		form_cbtn_uppa(f, CBTN_MINI, x, y);
		if (!f->minimizing++)
		{
			win_clip(f->wd, 0, 0, f->win_rect.w, f->win_rect.h, 0, 0);
			win_paint();
			form_draw_minibtn(f);
			win_end_paint();
		}
		break;
	case WIN_E_PTR_UP:
		form_cbtn_uppa(f, CBTN_MINI, x, y);
		if (!--f->minimizing)
		{
			win_clip(f->wd, 0, 0, f->win_rect.w, f->win_rect.h, 0, 0);
			win_paint();
			form_draw_minibtn(f);
			win_end_paint();
			
			if (f->title_btn_pointed_at)
				form_minimize(f);
		}
		break;
	case WIN_E_PTR_MOVE:
		form_cbtn_uppa(f, CBTN_MINI, x, y);
		break;
	}
	
	return 0;
}

static int form_move_event(struct event *e)
{
	struct form *f = e->data;
	
	if (f->zoomed)
		return 0;
	
	switch (e->win.type)
	{
	case WIN_E_PTR_DOWN:
		f->moving++;
		win_clip(f->wd, 0, 0, f->win_rect.w, f->win_rect.h, 0, 0);
		win_paint();
		form_draw_title(f);
		win_end_paint();
		break;
	case WIN_E_PTR_MOVE:
		if (e->win.more)
			break;
		
		if (form_cfg.display_moving)
			form_move(f, e->win.abs_x - f->ptr_down_x, e->win.abs_y - f->ptr_down_y);
		else
			win_rect_preview(1, e->win.abs_x - f->ptr_down_x,
					    e->win.abs_y - f->ptr_down_y,
					    f->win_rect.w,
					    f->win_rect.h);
		break;
	case WIN_E_PTR_UP:
		f->moving--;
		win_clip(f->wd, 0, 0, f->win_rect.w, f->win_rect.h, 0, 0);
		win_paint();
		form_draw_title(f);
		win_end_paint();
		
		win_rect_preview(0, 0, 0, 0, 0);
		form_move(f, e->win.abs_x - f->ptr_down_x, e->win.abs_y - f->ptr_down_y);
		break;
	}
	
	return 0;
}

static void form_ptr_down(struct event *e)
{
	struct form *f = e->data;
	struct gadget *g;
	int x, y;
	
	if (!(f->flags & FORM_BACKDROP))
		win_raise(f->wd);
	
	if (!(f->flags & FORM_NO_FOCUS))
		win_focus(f->wd);
	
	x = e->win.ptr_x;
	y = e->win.ptr_y;
	
	f->ptr_down_x = x;
	f->ptr_down_y = y;
	
	if (win_inrect(&f->title_rect, x, y))
	{
		form_move_event(e);
		return;
	}
	
	if (win_inrect(&f->closebtn_rect, x, y))
	{
		form_close_event(e);
		return;
	}
	
	if (win_inrect(&f->zoombtn_rect, x, y))
	{
		form_zoom_event(e);
		return;
	}
	
	if (win_inrect(&f->minibtn_rect, x, y))
	{
		form_mini_event(e);
		return;
	}
	
	if (win_inrect(&f->menubtn_rect, x, y))
	{
		form_control_menu_event(e);
		return;
	}
	
	if (x >= f->workspace_rect.x + f->workspace_rect.w &&
	    y >= f->workspace_rect.y + f->workspace_rect.h &&
	    (f->flags & FORM_ALLOW_RESIZE))
	{
		form_resize_event(e);
		return;
	}
	
	if (win_inrect(&f->menu_rect, x, y))
	{
		form_menu_event(e);
		return;
	}
	
	if (f->ptr_down)
		g = f->ptr_down_gadget;
	else
	{
		form_xygadget(f, &g, x, y);
		f->ptr_down_gadget = g;
	}
	
	f->ptr_down++;
	
	if (g && g->visible)
	{
		g->ptr_down_cnt++;
		
		gadget_focus(g);
		
		if (g->ptr_down)
			g->ptr_down(g, x - g->rect.x, y - g->rect.y, e->win.ptr_button);
	}
}

static void form_ptr_up(struct event *e)
{
	struct form *f = e->data;
	struct gadget *g;
	
	if (f->ptr_down)
	{
		g = f->ptr_down_gadget;
		
		if (g)
			g->ptr_down_cnt--;
		f->ptr_down--;
		
		if (g && g->ptr_up)
			g->ptr_up(g,
				  e->win.ptr_x - g->rect.x,
				  e->win.ptr_y - g->rect.y,
				  e->win.ptr_button);
		
		if (g && e->win.ptr_button == 1 && g->menu)
			menu_popup_l(g->menu,
				     e->win.ptr_x + f->win_rect.x,
				     e->win.ptr_y + f->win_rect.y,
				     f->layer);
	}
}

static void form_ptr_move(struct event *e)
{
	struct form *f = e->data;
	struct gadget *g;
	int x, y;
	
	if (f->ptr_down)
		g = f->ptr_down_gadget;
	else
		form_xygadget(f, &g, x, y);
	
	if (g && g->ptr_move)
		g->ptr_move(g, e->win.ptr_x - g->rect.x, e->win.ptr_y - g->rect.y);
}

static int form_mscan(struct menu *m, unsigned ch, unsigned shift)
{
	struct menu_item *mi, *emi;
	
	shift &= WIN_SHIFT_ALT | WIN_SHIFT_CTRL | WIN_SHIFT_SHIFT;
	if ((shift & WIN_SHIFT_CTRL) && ch < 0x20)
		ch |= 0x40;
	
	mi  = m->item;
	emi = m->item_count + mi;
	
	while (mi < emi)
	{
		if (mi->ch == ch && mi->shift == shift && mi->proc != NULL)
		{
			mi->proc(mi);
			return 1;
		}
		if (mi->submenu != NULL && form_mscan(mi->submenu, ch, shift))
			return 1;
		mi++;
	}
	return 0;
}

static void form_key_down(struct form *f, unsigned ch, unsigned shift)
{
	struct gadget *new_focus = NULL;
	struct gadget *g;
	int i;
	
	if (f->key_down && !f->key_down(f, ch, shift))
		return;
	
	if (ch && f->menu != NULL && form_mscan(f->menu, ch, shift))
		return;
	
	if (ch == '\t')
	{
		if (!f->focus || !f->focus->want_tab)
		{
			if (f->gadget)
			{
				g = f->gadget;
				while (g && g != f->focus)
				{
					if (g->want_focus && g->visible)
						new_focus = g;
					g = g->next;
				}
				
				if (!new_focus)
				{
					g = f->gadget;
					while (g)
					{
						if (g->want_focus && g->visible)
							new_focus = g;
						g = g->next;
					}
				}
				
				if (new_focus != NULL)
					gadget_focus(new_focus);
			}
			return;
		}
	}
	
	if (ch == WIN_KEY_F9 && f->menu)
	{
		if (!f->menu_active)
		{
			f->kbd_menu_active = 1;
			f->menu->selection = 0;
			form_draw_menu(f, 1);
		}
		return;
	}
	
	if (f->focus && f->focus->key_down)
		f->focus->key_down(f->focus, ch, shift);
}

static void form_key_up(struct form *f, unsigned ch, unsigned shift)
{
	if (f->key_up && !f->key_up(f, ch, shift))
		return;
	
	if (ch == '\t')
		return;
	
	if (f->focus && f->focus->key_up)
		f->focus->key_up(f->focus, ch, shift);
}

static void form_erect(struct form *f, int x, int y, int w, int h)
{
	int rx0, ry0;
	int rx1, ry1;
	
	if (!f->defop.draw_rect.w)
	{
		f->defop.draw_rect.x = x;
		f->defop.draw_rect.y = y;
		f->defop.draw_rect.w = w;
		f->defop.draw_rect.h = h;
		form_defer(f);
		return;
	}
	rx0 = f->defop.draw_rect.x;
	ry0 = f->defop.draw_rect.y;
	rx1 = f->defop.draw_rect.w + rx0;
	ry1 = f->defop.draw_rect.h + ry0;
	if (x < rx0)
		rx0 = x;
	if (y < ry0)
		ry0 = y;
	if (x + w > rx1)
		rx1 = x + w;
	if (y + h > ry1)
		ry1 = y + h;
	f->defop.draw_rect.x = rx0;
	f->defop.draw_rect.y = ry0;
	f->defop.draw_rect.w = rx1 - rx0;
	f->defop.draw_rect.h = ry1 - ry0;
	form_defer(f);
}

static void form_update_ptr_now(struct form *f)
{
	struct gadget *g;
	
	if (f->busy)
	{
		win_setptr(f->wd, WIN_PTR_BUSY);
		return;
	}
	
	if (f->ptr_down && f->ptr_down_gadget)
	{
		win_setptr(f->wd, f->ptr_down_gadget->ptr);
		return;
	}
	
	if (f->locked)
	{
		win_setptr(f->wd, WIN_PTR_ARROW);
		return;
	}
	
	if (f->moving || f->closing || f->zooming || f->pulling_menu)
	{
		win_setptr(f->wd, WIN_PTR_ARROW);
		return;
	}
	
	if (f->flags & FORM_ALLOW_RESIZE)
	{
		if (f->last_x >= f->workspace_rect.x + f->workspace_rect.w &&
		    f->last_y >= f->workspace_rect.y + f->workspace_rect.h)
		{
			win_setptr(f->wd, WIN_PTR_NW_SE);
			return;
		}
		if (f->resizing)
		{
			win_setptr(f->wd, WIN_PTR_NW_SE);
			return;
		}
	}
	
	form_xygadget(f, &g, f->last_x, f->last_y);
	if (g && g->visible)
	{
		win_setptr(f->wd, g->ptr);
		return;
	}
	win_setptr(f->wd, WIN_PTR_ARROW);
}

static void form_update_ptr(struct form *f)
{
	f->defop.update_ptr = 1;
	form_defer(f);
}

static void form_drop(struct event *e)
{
	struct form *f = e->data;
	struct gadget *g;
	char buf[WIN_DD_BUF];
	int serial;
	
	form_xygadget(f, &g, e->win.ptr_x, e->win.ptr_y);
	
	if (g == NULL)
		return;
	
	memset(buf, 0, sizeof buf);
	if (win_gdrop(buf, sizeof buf, &serial))
		return;
	
	if (g->drop != NULL)
		g->drop(g, buf, sizeof buf);
	if (g->udrop != NULL)
		g->udrop(g, buf, sizeof buf);
}

static void form_drag(struct form *f)
{
	struct gadget *g = f->drag_gadget;
	
	if (g == NULL)
		return;
	
	f->drag_gadget = NULL;
	
	if (g->drag != NULL)
		g->drag(g);
	if (g->udrag != NULL)
		g->udrag(g);
}

static int form_event(struct event *e)
{
	struct form *f = e->data;
	int ret = 0;
	
	form_newref(f);
	
	if (!f->visible)
		goto fini;
	
	if (f->minimized)
	{
		if (e->win.type == WIN_E_KEY_DOWN && (e->win.ch == '\n' || e->win.ch == ' '))
			form_umini(f);
		if (e->win.type == WIN_E_SWITCH_TO && !f->control_menu_active)
			form_umini(f);
		goto fini;
	}
	
	if (e->win.type == WIN_E_PTR_MOVE || e->win.type == WIN_E_PTR_UP || e->win.type == WIN_E_PTR_DOWN)
	{
		f->last_x = e->win.ptr_x;
		f->last_y = e->win.ptr_y;
		form_update_ptr(f);
	}
	
	if (e->win.type == WIN_E_DRAG)
	{
		form_drag(f);
		goto fini;
	}
	
	if (e->win.type == WIN_E_REDRAW)
	{
		form_erect(f, e->win.redraw_x, e->win.redraw_y,
			      e->win.redraw_w, e->win.redraw_h);
		goto fini;
	}
	if (e->win.type == WIN_E_BLINK)
	{
		if (f->focus && f->focus->blink)
			f->focus->blink(f->focus);
		goto fini;
	}
	
	if (e->win.type == WIN_E_RAISE)
	{
		if (f->raise)
			f->raise(f);
		
		if (f->child_dialog)
		{
			form_raise(f->child_dialog);
			goto fini;
		}
	}
	
	if (e->win.type == WIN_E_SWITCH_TO)
	{
		if (!f->child_dialog)
		{
			f->focused = 1;
			form_draw_decoration(f);
		}
		else
			form_focus(f->child_dialog);
		goto fini;
	}
	if (e->win.type == WIN_E_SWITCH_FROM)
	{
		f->focused = 0;
		form_draw_decoration(f);
	}
	
	if (f->locked && !f->ptr_down)
	{
		if (f->child_dialog && e->win.type == WIN_E_PTR_DOWN)
		{
			form_raise(f);
			form_focus(f->child_dialog);
		}
		goto fini;
	}
	
	if (f->menu_active)
	{
		ret = form_menu_event(e);
		goto fini;
	}
	
	if (f->resizing)
	{
		ret = form_resize_event(e);
		goto fini;
	}
	
	if (f->closing)
	{
		ret = form_close_event(e);
		goto fini;
	}
	
	if (f->zooming)
	{
		ret = form_zoom_event(e);
		goto fini;
	}
	
	if (f->minimizing)
	{
		ret = form_mini_event(e);
		goto fini;
	}
	
	if (f->moving)
	{
		ret = form_move_event(e);
		goto fini;
	}
	
	if (f->pulling_menu)
	{
		ret = form_control_menu_event(e);
		goto fini;
	}
	
	switch (e->win.type)
	{
	case WIN_E_PTR_DOWN:
		form_ptr_down(e);
		break;
	case WIN_E_PTR_UP:
		form_ptr_up(e);
		break;
	case WIN_E_PTR_MOVE:
		form_ptr_move(e);
		break;
	case WIN_E_DROP:
		form_drop(e);
		break;
	}
	
	if (f->locked)
		goto fini;
	
	if (f->kbd_menu_active)
	{
		ret = form_kbd_menu_event(e);
		goto fini;
	}
	
	switch (e->win.type)
	{
	case WIN_E_KEY_DOWN:
		if ((f->flags & FORM_ALLOW_CLOSE) && (e->win.shift & WIN_SHIFT_ALT) && e->win.ch == WIN_KEY_F4)
		{
			ret = form_close(f);
			goto fini;
		}
		
		if ((f->flags & FORM_ALLOW_RESIZE) && (e->win.shift & WIN_SHIFT_ALT) && e->win.ch == WIN_KEY_F3)
		{
			form_zoom(f);
			goto fini;
		}
		
		if ((f->flags & FORM_ALLOW_MINIMIZE) && (e->win.shift & WIN_SHIFT_ALT) && e->win.ch == WIN_KEY_F2)
		{
			form_minimize(f);
			goto fini;
		}
		
		form_key_down(f, e->win.ch, e->win.shift);
		goto fini;
	case WIN_E_KEY_UP:
		form_key_up(f, e->win.ch, e->win.shift);
		goto fini;
	}
	
fini:
	form_putref(f);
	return ret;
}

static int form_grow_bg_rects(struct form *f, int cap)
{
	struct win_rect *p;
	
	cap +=  15;
	cap &= ~15;
	
	if (f->bg_rects_cap >= cap)
		return 0;
	
	p = realloc(f->bg_rects, sizeof *p * cap);
	if (!p)
		return -1;
	
	f->bg_rects_cap = cap;
	f->bg_rects	= p;
	return 0;
}

static void form_clip_rect(struct win_rect *d, struct win_rect *s)
{
	int x0, y0, x1, y1;
	
	x0 = d->x;
	y0 = d->y;
	x1 = d->x + d->w;
	y1 = d->y + d->h;
	
	if (x0 < s->x)
		x0 = s->x;
	if (y0 < s->y)
		y0 = s->y;
	if (x1 > s->x + s->w)
		x1 = s->x + s->w;
	if (y1 > s->y + s->h)
		y1 = s->y + s->h;
	
	d->x = x0;
	d->y = y0;
	d->w = x1 - x0;
	d->h = y1 - y0;
}

static int form_make_bg_rects(struct form *f)
{
	struct win_rect *rp, *nrp;
	struct gadget *g;
	int cnt;
	int i;
	
	if (form_grow_bg_rects(f, 1))
		return -1;
	rp = f->bg_rects;
	rp->x = f->workspace_rect.x;
	rp->y = f->workspace_rect.y;
	rp->w = f->workspace_rect.x + f->workspace_rect.w;
	rp->h = f->workspace_rect.y + f->workspace_rect.h;
	cnt = 1;
	
	for (i = 0; i < cnt; i++)
	{
		rp = &f->bg_rects[i];
		for (g = f->gadget; g; g = g->next)
		{
			if (g->rect.x >= rp->w)
				continue;
			if (g->rect.y >= rp->h)
				continue;
			if (g->rect.x + g->rect.w <= rp->x)
				continue;
			if (g->rect.y + g->rect.h <= rp->y)
				continue;
			if (g->no_bg)
				continue;
			if (!g->visible)
				continue;
			
			if (g->rect.y > rp->y)
			{
				if (form_grow_bg_rects(f, cnt + 1))
					return -1;
				rp  = &f->bg_rects[i];
				nrp = &f->bg_rects[cnt++];
				nrp->x = rp->x;
				nrp->y = rp->y;
				nrp->w = rp->w;
				nrp->h = g->rect.y;
			}
			if (g->rect.x > rp->x)
			{
				if (form_grow_bg_rects(f, cnt + 1))
					return -1;
				rp  = &f->bg_rects[i];
				nrp = &f->bg_rects[cnt++];
				nrp->x = rp->x;
				nrp->y = max(rp->y, g->rect.y);
				nrp->w = g->rect.x;
				nrp->h = min(rp->h, g->rect.y + g->rect.h);
			}
			if (g->rect.y + g->rect.h < rp->h)
			{
				if (form_grow_bg_rects(f, cnt + 1))
					return -1;
				rp  = &f->bg_rects[i];
				nrp = &f->bg_rects[cnt++];
				nrp->x = rp->x;
				nrp->y = g->rect.y + g->rect.h;
				nrp->w = rp->w;
				nrp->h = rp->h;
			}
			if (g->rect.x + g->rect.w < rp->w)
			{
				if (form_grow_bg_rects(f, cnt + 1))
					return -1;
				rp  = &f->bg_rects[i];
				nrp = &f->bg_rects[cnt++];
				nrp->x = g->rect.x + g->rect.w;
				nrp->y = max(rp->y, g->rect.y);
				nrp->w = rp->w;
				nrp->h = min(rp->h, g->rect.y + g->rect.h);
			}
			memmove(&f->bg_rects[i], &f->bg_rects[i + 1],
				(cnt - i - 1) * sizeof *f->bg_rects);
			cnt--;
			i--;
			break;
		}
	}
	for (i = 0; i < cnt; i++)
	{
		f->bg_rects[i].w -= f->bg_rects[i].x;
		f->bg_rects[i].h -= f->bg_rects[i].y;
	}
	f->bg_rects_valid = 1;
	f->bg_rects_cnt	  = cnt;
	return 0;
}

static int form_erase_bg(struct form *f)
{
	const struct form_theme *th;
	struct win_rect *p;
	struct win_rect *e;
	win_color bg;
	int wd = f->wd;
	
	th = form_th_get();
	if (th != NULL && th->d_formbg)
	{
		th->d_formbg(wd, f->workspace_rect.x,
				 f->workspace_rect.y,
				 f->workspace_rect.w,
				 f->workspace_rect.h);
		return 0;
	}
	
	bg = wc_get(WC_WIN_BG);
	
	if (!f->bg_rects_valid && form_make_bg_rects(f))
	{
		win_rect(wd, bg, f->workspace_rect.x,
				 f->workspace_rect.y,
				 f->workspace_rect.w,
				 f->workspace_rect.h);
		return 0;
	}
	
	p = f->bg_rects;
	e = f->bg_rects_cnt + p;
	while (p < e)
	{
		win_rect(wd, bg, p->x, p->y, p->w, p->h);
		p++;
	}
	return 0;
}

static int form_redraw_rect(struct form *f, int x, int y, int w, int h)
{
	struct gadget *g;
	win_color hl1;
	win_color hl2;
	win_color sh1;
	win_color sh2;
	win_color bg;
	
	if (!f->visible)
		return 0;
	
	hl1 = wc_get(WC_HIGHLIGHT1);
	hl2 = wc_get(WC_HIGHLIGHT2);
	sh1 = wc_get(WC_SHADOW1);
	sh2 = wc_get(WC_SHADOW2);
	bg  = wc_get(WC_WIN_BG);
	
	win_paint();
	win_set_font(f->wd, WIN_FONT_DEFAULT);
	win_clip(f->wd, x, y, w, h, 0, 0);
	
	if (!(f->flags & FORM_NO_BACKGROUND))
		form_erase_bg(f);
	
	form_draw_decoration_now(f, 0);
	form_draw_menu(f, 0);
	
	for (g = f->gadget; g; g = g->next)
	{
		int x0;
		int y0;
		int x1;
		int y1;
		
		if (!g->redraw || !g->visible)
			continue;
		
		x0 = g->rect.x;
		y0 = g->rect.y;
		x1 = g->rect.x + g->rect.w;
		y1 = g->rect.y + g->rect.h;
		
		if (x0 < x)
			x0 = x;
		if (y0 < y)
			y0 = y;
		
		if (x1 > x + w)
			x1 = x + w;
		if (y1 > y + h)
			y1 = y + h;
		
		if (x0 < x1 && y0 < y1)
		{
			win_clip(g->form->wd, x0, y0, x1 - x0, y1 - y0,
				 g->rect.x, g->rect.y);
			win_set_font(f->wd, g->font);
			g->redraw(g, g->form->wd);
		}
	}
	
	win_end_paint();
	return 0;
}

void form_redraw(struct form *f)
{
	form_erect(f, 0, 0, f->win_rect.w, f->win_rect.h);
}

struct form *form_creat(int flags, int visible, int x, int y, int w, int h, const char *title)
{
	struct win_rect wsr;
	struct form *f;
	int px, py;
	char *t;
	
	if (!form_cfg_loaded)
		form_reload_config();
	if (form_cfg.smart_zoom)
		flags |= FORM_CENTER;
	
	f = malloc(sizeof *f);
	t = strdup(title);
	if (!f || !t)
	{
		free(f);
		free(t);
		return NULL;
	}
	
	memset(f, 0, sizeof *f);
	
	if (win_creat(&f->wd, 0, 0, 0, 0, 0, form_event, f))
	{
		free(f);
		free(t);
		return NULL;
	}
	
	if (!(flags & FORM_EXCLUDE_FROM_LIST))
		win_set_title(f->wd, title);
	
	f->curr_ptr	   = WIN_PTR_ARROW;
	f->visible	   = visible;
	f->last_x	   = -1;
	f->last_y	   = -1;
	f->win_rect.x	   = x;
	f->win_rect.y	   = y;
	f->title	   = t;
	f->flags	   = flags;
	win_deflayer(&f->layer);
	
	if (f->flags & FORM_FRAME)
		f->shadow_width = form_th_get()->form_shadow_w;
	
	list_init(&f->deferred_gadgets, struct gadget, deferred_item);
	f->gadget = NULL;
	
	form_setup_rects(f, w, h);
	
	if (flags & FORM_CENTER)
	{
		win_ws_getrect(&wsr);
		f->win_rect.x = (wsr.w - f->win_rect.w) >> 1;
		f->win_rect.y = (wsr.h - f->win_rect.h) >> 1;
		
		x = 0;
		y = 0;
	}
	
	if (x == -1 && y == -1)
		win_advise_pos(&f->win_rect.x, &f->win_rect.y, f->win_rect.w, f->win_rect.h);
	
	win_setlayer(f->wd, f->layer);
	win_raise(f->wd);
	if (!(flags & FORM_NO_FOCUS))
	{
		f->focused = 1;
		win_focus(f->wd);
	}
	if (form_cfg.smart_zoom && (flags & FORM_ALLOW_RESIZE))
	{
		f->defop.zoom = 1;
		form_defer(f);
	}
	else
		form_update_pos(f);
	
	win_get_ptr_pos(&px, &py);
	f->last_x = px - f->win_rect.x;
	f->last_y = py - f->win_rect.y;
	
	list_app(&form_list, f);
	return f;
}

int form_close(struct form *form)
{
	struct gadget *g;
	
	if (form->refcnt)
	{
		form->autoclose = 1;
		return 0;
	}
	
	form->locked++;
	if (form->close && !form->close(form))
	{
		_set_errno(EPERM);
		form->locked--;
		return -1;
	}
	
	form_hide(form);
	while (form->gadget)
	{
		g = form->gadget;
		while (g && g->is_internal)
			g = g->next;
		if (!g)
			__libc_panic("form_close: all gadgets are internal");
		gadget_remove(g);
	}
	
	if (form->parent_form)
		form_set_dialog(form->parent_form, NULL);
	if (form->defop.listed)
		list_rm(&form_deferred, form);
	list_rm(&form_list, form);
	win_close(form->wd);
	free(form->bg_rects);
	free(form->title);
	free(form);
	return 0;
}

static void form_update_title(struct form *form)
{
	int display = 1;
	
	if ((form->flags & FORM_EXCLUDE_FROM_LIST) || form->parent_form || !form->visible)
		win_set_title(form->wd, "");
	else
		win_set_title(form->wd, form->title);
}

int form_set_title(struct form *form, const char *title)
{
	char *dt;
	
	dt = strdup(title);
	if (!dt)
		return -1;
	
	form->title = dt;
	form_update_title(form);
	
	if (form->visible)
	{
		win_clip(form->wd, 0, 0, form->win_rect.w, form->win_rect.h, 0, 0);
		win_paint();
		form_draw_title(form);
		win_end_paint();
	}
	return 0;
}

int form_set_layer(struct form *form, int layer)
{
	win_setlayer(form->wd, layer);
	form->layer = layer;
	return 0;
}

int form_set_menu(struct form *form, void *menu)
{
	struct menu_item *im;
	struct gadget *g;
	int old_x;
	int old_y;
	int x = 0;
	int tl;
	int i;
	
	tl = wm_get(WM_THIN_LINE);
	
	form->menu = menu;
	if (menu)
	{
		form->menu->selection = -1;
		
		for (i = 0; i < form->menu->item_count; i++)
		{
			im = &form->menu->item[i];
			
			im->rect.x = x;
			im->rect.y = 0;
			
			win_text_size(WIN_FONT_DEFAULT, &im->rect.w, &im->rect.h, im->text);
			
			im->rect.h += 2 * tl;
			im->rect.w += 8 * tl;
			
			if (!strcmp(im->text, "-"))
				im->rect.w = 4 * tl;
			
			x += im->rect.w;
		}
	}
	
	old_x = form->workspace_rect.x;
	old_y = form->workspace_rect.y;
	
	form_setup_rects(form, form->workspace_rect.w, form->workspace_rect.h);
	for (g = form->gadget; g; g = g->next)
		gadget_move(g, g->rect.x - old_x, g->rect.y - old_y);
	if (form->visible)
		form_update_pos(form);
	if (form->resize)
		form->resize(form, form->workspace_rect.w, form->workspace_rect.h);
	return 0;
}

int form_wait(struct form *form)
{
	int r;
	
	form_newref(form);
	
	if (!form->visible)
		form_show(form);
	
	form->result = 0;
	while (!form->result)
	{
		if (form->autoclose)
		{
			form->autoclose = 0;
			if (form->close != NULL && !form->close(form))
				continue;
			form->close = NULL;
			form_putref(form);
			form_close(form);
			return 0;
		}
		win_wait();
	}
	r = form->result;
	
	form_putref(form);
	return r;
}

void form_unbusy(struct form *f)
{
	if (!f)
	{
		for (f = list_first(&form_list); f; f = list_next(&form_list, f))
			form_unbusy(f);
		return;
	}
	
	if (!f->busy)
		__libc_panic("form_unbusy: not busy");
	if (!--f->busy)
		form_update_ptr(f);
}

void form_busy(struct form *f)
{
	if (!f)
	{
		for (f = list_first(&form_list); f; f = list_next(&form_list, f))
			form_busy(f);
		return;
	}
	
	if (!f->busy++)
	{
		f->defop.update_ptr = 0;
		form_update_ptr_now(f);
	}
}

void form_get_state(struct form *form, struct form_state *buf)
{
	memset(buf, 0, sizeof *buf);
	if (form->zoomed)
	{
		buf->rect  = form->saved_rect;
		buf->state = 1;
		return;
	}
	buf->rect.x = form->win_rect.x;
	buf->rect.y = form->win_rect.y;
	buf->rect.w = form->workspace_rect.w;
	buf->rect.h = form->workspace_rect.h;
	buf->state  = 0;
}

void form_set_state(struct form *form, struct form_state *buf)
{
	struct form_state st = *buf;
	struct win_rect ws;
	int vm, hm;
	
	form->defop.zoom = 0;
	
	if (buf->state)
	{
		if (!form->zoomed)
			form_zoom(form);
		form->saved_rect = buf->rect;
		return;
	}
	
	hm = form->win_rect.w - form->workspace_rect.w;
	vm = form->win_rect.h - form->workspace_rect.h;
	
	win_ws_getrect(&ws);
	
	if (st.rect.x < ws.x)
		st.rect.x = ws.x;
	if (st.rect.y < ws.y)
		st.rect.y = ws.y;
	
	if (st.rect.x + st.rect.w + hm > ws.x + ws.w)
	{
		st.rect.x = ws.x;
		st.rect.w = ws.w - hm;
	}
	if (st.rect.y + st.rect.h + vm > ws.y + ws.h)
	{
		st.rect.y = ws.y;
		st.rect.h = ws.h - vm;
	}
	
	if (st.rect.x >= ws.x + ws.w)
		st.rect.x = ws.x + ws.w - 1;
	if (st.rect.y >= ws.y + ws.h)
		st.rect.y = ws.y + ws.h - 1;
	
	form_move(form, st.rect.x, st.rect.y);
	form_resize(form, st.rect.w, st.rect.h);
}

int form_on_key_down(struct form *form, form_key_cb *proc)
{
	form->key_down = proc;
	return 0;
}

int form_on_key_up(struct form *form, form_key_cb *proc)
{
	form->key_up = proc;
	return 0;
}

int form_on_close(struct form *form, form_close_cb *proc)
{
	form->close = proc;
	return 0;
}

int form_on_raise(struct form *form, form_raise_cb *proc)
{
	form->raise = proc;
	return 0;
}

int form_on_resize(struct form *form, form_resize_cb *proc)
{
	form->resize = proc;
	return 0;
}

int form_on_move(struct form *form, form_move_cb *proc)
{
	form->move = proc;
	return 0;
}

void form_set_dialog(struct form *form, struct form *child)
{
	int pa = form_is_active(form);
	
	if (form->child_dialog)
	{
		form->child_dialog->parent_form = NULL;
		if (!(form->child_dialog->flags & FORM_EXCLUDE_FROM_LIST))
			win_set_title(form->child_dialog->wd,
				      form->child_dialog->title);
		form_unlock(form);
	}
	
	form->child_dialog = child;
	
	if (child)
	{
		if (!(child->flags & FORM_EXCLUDE_FROM_LIST))
			win_set_title(child->wd, "");
		child->parent_form = form;
		form_lock(form);
	}
	
	if (form_is_active(form) != pa)
		form_draw_decoration(form);
}

void form_unlock(struct form *form)
{
	if (!form->locked)
		__libc_panic("form_unlock: form not locked");
	
	form->locked--;
	form_update_ptr(form);
}

void form_lock(struct form *form)
{
	form->locked++;
	form_update_ptr(form);
}

void form_resize(struct form *form, int w, int h)
{
	int prev_w = form->win_rect.w;
	int prev_h = form->win_rect.h;
	
	form_setup_rects(form, w, h);
	if (form->win_rect.w == prev_w && form->win_rect.h == prev_h)
		return;
	form->bg_rects_valid = 0;
	
	form_update_pos(form);
	if (form->resize)
		form->resize(form, w, h);
}

static void form_update_pos(struct form *form)
{
	int v = form->visible && !form->minimized;
	
	win_change(form->wd, v, form->win_rect.x,
				form->win_rect.y,
				form->win_rect.w,
				form->win_rect.h);
}

void form_move(struct form *form, int x, int y)
{
	if (form->win_rect.x == x && form->win_rect.y == y)
		return;
	
	form->last_x -= x - form->win_rect.x;
	form->last_y -= y - form->win_rect.y;
	
	form->win_rect.x = x;
	form->win_rect.y = y;
	
	form_update_ptr(form);
	form_update_pos(form);
	if (form->move)
		form->move(form, x, y);
}

void form_show(struct form *form)
{
	if (form->visible)
		return;
	
	form->visible = 1;
	form_update_title(form);
	form_update_pos(form);
	form_update_ptr(form);
}

void form_hide(struct form *form)
{
	if (!form->visible)
		return;
	
	form->visible = 0;
	form_update_title(form);
	form_update_pos(form);
}

static void form_umini(struct form *form)
{
	if (!form->minimized)
		return;
	
	form->minimized = 0;
	form_update_pos(form);
}

void form_minimize(struct form *form)
{
	if (form->minimized || win_chk_taskbar())
		return;
	
	form->minimized = 1;
	form_update_pos(form);
	win_ufocus(form->wd);
}

void form_raise(struct form *form)
{
	if (form->flags & FORM_BACKDROP)
		return;
	win_raise(form->wd);
}

void form_focus(struct form *form)
{
	win_focus(form->wd);
}

struct gadget *gadget_creat(struct form *form, int x, int y, int w, int h)
{
	struct gadget *g = malloc(sizeof *g);
	const struct form_theme *th = form_th_get();
	
	if (!g)
		return NULL;
	
	memset(g, 0, sizeof *g);
	
	if (th && th->d_formbg)
		g->no_bg = 1;
	
	g->form    = form;
	g->next	   = form->gadget;
	g->visible = 1;
	g->font	   = WIN_FONT_DEFAULT;
	g->ptr	   = WIN_PTR_DEFAULT;
	g->rect.x  = x + form->workspace_rect.x;
	g->rect.y  = y + form->workspace_rect.y;
	g->rect.w  = w;
	g->rect.h  = h;
	form->gadget = g;
	form->bg_rects_valid = 0;
	form_update_ptr(form);
	return g;
}

int gadget_remove(struct gadget *gadget)
{
	struct gadget **prev_next;
	struct gadget *curr;
	
	if (!gadget)
		return 0;
	
	if (gadget->visible)
	{
		gadget->form->bg_rects_valid = 0;
		form_update_ptr(gadget->form);
	}
	
	if (gadget->form->focus == gadget)
	{
		gadget->form->focus = NULL;
		if (gadget->blink)
			win_blink(gadget->form->wd, 0);
	}
	
	if (gadget->remove)
		gadget->remove(gadget);
	
	prev_next = &gadget->form->gadget;
	curr = gadget->form->gadget;
	
	while (curr)
	{
		if (curr == gadget)
		{
			*prev_next = gadget->next;
			form_redraw(gadget->form);
			free(gadget->name);
			free(gadget);
			return 0;
		}
		
		prev_next = &curr->next;
		curr = curr->next;
	}
	
	__libc_panic("gadget_remove: no such gadget in this form");
}

void gadget_set_rect(struct gadget *gadget, const struct win_rect *rect)
{
	gadget_resize(gadget, rect->w, rect->h);
	gadget_move(gadget, rect->x, rect->y);
}

void gadget_get_rect(struct gadget *gadget, struct win_rect *rect)
{
	rect->x = gadget->rect.x - gadget->form->workspace_rect.x;
	rect->y = gadget->rect.y - gadget->form->workspace_rect.y;
	rect->w = gadget->rect.w;
	rect->h = gadget->rect.h;
}

void gadget_move(struct gadget *gadget, int x, int y)
{
	if (gadget->rect.x == x && gadget->rect.y == y)
		return;
	
	form_erect(gadget->form, gadget->rect.x, gadget->rect.y,
				 gadget->rect.w, gadget->rect.h);
	gadget->rect.x = x + gadget->form->workspace_rect.x;
	gadget->rect.y = y + gadget->form->workspace_rect.y;
	form_erect(gadget->form, gadget->rect.x, gadget->rect.y,
				 gadget->rect.w, gadget->rect.h);
	gadget->form->bg_rects_valid = 0;
	form_update_ptr(gadget->form);
	if (gadget->move)
		gadget->move(gadget, x, y);
}

void gadget_resize(struct gadget *gadget, int w, int h)
{
	int redraw_w;
	int redraw_h;
	int prev_w;
	int prev_h;
	
	prev_w = gadget->rect.w;
	prev_h = gadget->rect.h;
	
	if (prev_w == w && prev_h == h)
		return;
	
	gadget->form->bg_rects_valid = 0;
	gadget->rect.w = w;
	gadget->rect.h = h;
	
	if (gadget->redraw)
	{
		if (w > prev_w)
			redraw_w = w;
		else
			redraw_w = prev_w;
		
		if (h > prev_h)
			redraw_h = h;
		else
			redraw_h = prev_h;
		
		form_erect(gadget->form, gadget->rect.x, gadget->rect.y, redraw_w, redraw_h);
	}
	form_update_ptr(gadget->form);
	if (gadget->resize)
		gadget->resize(gadget, w, h);
}

void gadget_setptr(struct gadget *gadget, int ptr)
{
	gadget->ptr = ptr;
	form_update_ptr(gadget->form);
}

void gadget_hide(struct gadget *g)
{
	if (!g->visible)
		return;
	if (g->redraw)
		form_erect(g->form, g->rect.x, g->rect.y, g->rect.w, g->rect.h);
	if (g == g->form->focus)
	{
		win_blink(g->form->wd, 0);
		g->form->focus = NULL;
	}
	g->form->bg_rects_valid = 0;
	g->visible = 0;
	form_update_ptr(g->form);
}

void gadget_show(struct gadget *g)
{
	if (g->visible)
		return;
	if (g->redraw)
		form_erect(g->form, g->rect.x, g->rect.y, g->rect.w, g->rect.h);
	g->form->bg_rects_valid = 0;
	g->visible = 1;
	form_update_ptr(g->form);
}

void gadget_clip(struct gadget *g, int x, int y, int w, int h, int x0, int y0)
{
	int rx0, ry0, rx1, ry1;
	
	if (!g->visible)
		__libc_panic("gadget_clip: gadget not visible");
	
	rx0 = g->rect.x + x;
	ry0 = g->rect.y + y;
	rx1 = g->rect.x + x + w;
	ry1 = g->rect.y + y + h;
	
	if (rx0 < g->rect.x)
		rx0 = g->rect.x;
	if (ry0 < g->rect.y)
		ry0 = g->rect.y;
	
	if (rx1 > g->rect.x + g->rect.w)
		rx1 = g->rect.x + g->rect.w;
	if (ry1 > g->rect.y + g->rect.h)
		ry1 = g->rect.y + g->rect.h;
	
	win_clip(g->form->wd, rx0, ry0, rx1 - rx0, ry1 - ry0,
			      x0 + g->rect.x, y0 + g->rect.y);
}

void gadget_defer(struct gadget *g)
{
	if (g->deferred)
		return;
	
	list_app(&g->form->deferred_gadgets, g);
	g->form->defop.gadgets = 1;
	g->deferred = 1;
	form_defer(g->form);
}

void gadget_redraw(struct gadget *g)
{
	if (!g->visible || !g->form->visible)
		return;
	
	if (!g->redraw)
		return;
	
	form_erect(g->form, g->rect.x, g->rect.y, g->rect.w, g->rect.h);
	return;
}

void gadget_focus(struct gadget *gadget)
{
	struct gadget *prev = gadget->form->focus;
	
	if (gadget->want_focus)
	{
		gadget->form->focus = gadget;
		
		if (prev != gadget)
		{
			if (prev && prev->focus)
				prev->focus(prev);
			if (gadget->focus)
				gadget->focus(gadget);
			win_blink(gadget->form->wd, gadget->blink != NULL);
		}
	}
}

void gadget_set_menu(struct gadget *gadget, void *menu)
{
	gadget->menu = menu;
}

void gadget_set_font(struct gadget *gadget, int ftd)
{
	gadget->font = ftd;
	gadget_redraw(gadget);
	if (gadget->set_font)
		gadget->set_font(gadget, ftd);
}

int form_set_result(struct form *form, int result)
{
	form->result = result;
	return 0;
}

int form_get_result(struct form *form)
{
	return form->result;
}

struct gadget *form_get_focus(struct form *form)
{
	return form->focus;
}

int gadget_dragdrop(struct gadget *gadget, const void *data, size_t len)
{
	if (win_dragdrop(data, len))
	{
		warn("win_dragdrop");
		return -1;
	}
	
	gadget->form->drag_gadget = gadget;
	return 0;
}

void gadget_on_drop(struct gadget *gadget, gadget_drop_cb *on_drop)
{
	gadget->udrop = on_drop;
}

void gadget_on_drag(struct gadget *gadget, gadget_drag_cb *on_drag)
{
	gadget->udrag = on_drag;
}

void form_reload_config(void)
{
	if (c_load("form", &form_cfg, sizeof form_cfg))
	{
		form_cfg.display_moving	= DEFAULT_FORM_DISPLAY_MOVING;
		form_cfg.smart_zoom	= 0;
	}
	form_cfg_loaded = 1;
}

static void form_activate(struct form *f, int activate)
{
	int prev = form_is_active(f);
	
	if (activate)
		f->active++;
	else
		f->active--;
	
	if (form_is_active(f) != prev)
		form_draw_decoration(f);
}

static void form_defer(struct form *f)
{
	if (f->defop.listed)
		return;
	list_app(&form_deferred, f);
	f->defop.listed = 1;
}

void form_do_deferred(void)
{
	struct win_rect r;
	struct gadget *g;
	struct form *f;
	
	while (f = list_first(&form_deferred), f)
	{
		list_rm(&form_deferred, f);
		f->defop.listed = 0;
		
		if (f->defop.update_ptr)
		{
			f->defop.update_ptr = 0;
			form_update_ptr_now(f);
		}
		
		win_paint();
		if (f->defop.draw_rect.w)
		{
			r = f->defop.draw_rect;
			memset(&f->defop.draw_rect, 0, sizeof f->defop.draw_rect);
			form_redraw_rect(f, r.x, r.y, r.w, r.h);
		}
		
		win_clip(f->wd, 0, 0, f->win_rect.w, f->win_rect.h, 0, 0);
		if (f->defop.draw_closebtn)
		{
			f->defop.draw_closebtn = 0;
			form_draw_closebtn(f);
		}
		if (f->defop.draw_zoombtn)
		{
			f->defop.draw_zoombtn = 0;
			form_draw_zoombtn(f);
		}
		if (f->defop.draw_menubtn)
		{
			f->defop.draw_menubtn = 0;
			form_draw_menubtn(f);
		}
		if (f->defop.draw_minibtn)
		{
			f->defop.draw_minibtn = 0;
			form_draw_minibtn(f);
		}
		if (f->defop.draw_frame)
		{
			f->defop.draw_frame = 0;
			form_draw_frame(f);
		}
		if (f->defop.draw_title)
		{
			f->defop.draw_title = 0;
			form_draw_title(f);
		}
		if (f->defop.zoom)
		{
			f->defop.zoom = 0;
			form_zoom(f);
		}
		if (f->defop.gadgets)
		{
			f->defop.gadgets = 0;
			while (g = list_first(&f->deferred_gadgets), g)
			{
				list_rm(&f->deferred_gadgets, g);
				g->deferred = 0;
				g->do_defops(g);
			}
		}
		win_end_paint();
	}
}

void form_newref(struct form *form)
{
	form->refcnt++;
}

void form_putref(struct form *form)
{
	if (!form->refcnt)
		__libc_panic("form_putref: !form->refcnt");
	form->refcnt--;
	if (!form->refcnt && form->autoclose)
	{
		form->autoclose = 0;
		form_close(form);
	}
}

void form_ws_resized(void)
{
	struct win_rect wsr;
	struct form *f;
	
	win_ws_getrect(&wsr);
	for (f = list_first(&form_list); f; f = list_next(&form_list, f))
	{
		if (!f->zoomed)
			continue;
		form_do_zoom(f, &wsr);
	}
}

void form_init(void)
{
	list_init(&form_deferred, struct form, defop.list_item);
	list_init(&form_list, struct form, list_item);
}
