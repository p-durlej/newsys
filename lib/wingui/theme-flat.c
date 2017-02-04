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
#include <wingui_metrics.h>
#include <wingui_color.h>
#include <wingui.h>

#define min(a, b)	((a) < (b) ? (a) : (b))

static int d_button(int wd, int x, int y, int w, int h, const char *label, int flags)
{
	win_color fr, fr1, bg, fg;
	int tx, ty, tw, th;
	int tl;
	
	if (flags & DB_DEPSD)
	{
		win_rgba2color(&fr,   0,   0,   0, 255);
		win_rgba2color(&bg,   0,   0,   0, 255);
		win_rgba2color(&fg, 255, 255, 255, 255);
		
		fr1 = bg;
	}
	else
	{
		fr = wc_get(WC_BUTTON_HI1);
		bg = wc_get(WC_BUTTON_BG);
		fg = wc_get(WC_BUTTON_FG);
		
		fr1 = bg;
		
		if (flags & DB_FOCUS)
		{
			win_rgba2color(&fr, 0, 0, 0, 255);
			fr1 = fr;
		}
	}
	
	tl = wm_get(WM_THIN_LINE);
	
	win_frame7(wd, fr, x, y, w, h, tl);
	win_frame7(wd, fr1, x + tl, y + tl, w - 2 * tl, h - 2 * tl, tl);
	
	win_rect(wd, bg, x + 2 * tl, y + 2 * tl, w - 4 * tl, h - 4 * tl);
	
	win_text_size(WIN_FONT_DEFAULT, &tw, &th, label);
	tx = x + (w - tw) / 2;
	ty = y + (h - th) / 2;
	
	win_text(wd, fg, tx, ty, label);
	return 0;
}

static int d_titlebtn(int wd, int x, int y, int w, int h, int flags)
{
	win_color bg;
	int split;
	int size;
	int i;
	
	if (flags & DB_DEPSD)
		bg = wc_get(WC_SEL_BG);
	else if (flags & DB_FOCUS)
		bg = wc_get(WC_TITLE_BTN_BG);
	else
		bg = wc_get(WC_INACT_TITLE_BTN_BG);
	
	win_rect(wd, bg, x, y, w, h);
	return 0;
}

static int d_closebtn(int wd, int x, int y, int w, int h, int flags)
{
	int tl = wm_get(WM_THIN_LINE);
	int th = tl * 2;
	win_color fg;
	int size;
	int i, n;
	
	if (w < h)
		size = w;
	else
		size = h;
	
	if (flags & DB_DEPSD)
		fg = wc_get(WC_SEL_FG);
	else if (flags & DB_FOCUS)
		win_rgba2color(&fg, 192, 0, 0, 255);
	else
		fg = wc_get(WC_INACT_TITLE_BTN_FG);
	
	d_titlebtn(wd, x, y, w, h, flags);
	
	for (i = 4 * tl; i <= size - 4 * tl - th; i++)
	{
		win_rect(wd, fg, x + size - i - th, y + i, th, th);
		win_rect(wd, fg, x + i,		    y + i, th, th);
	}
	
	return 0;
}

static int d_zoombtn(int wd, int x, int y, int w, int h, int flags)
{
	int tl = wm_get(WM_THIN_LINE);
	win_color fg;
	int size;
	int i;
	
	if (w < h)
		size = w;
	else
		size = h;
	
	if (flags & DB_DEPSD)
		fg = wc_get(WC_SEL_FG);
	else if (flags & DB_FOCUS)
		fg = wc_get(WC_TITLE_BTN_FG);
	else
		fg = wc_get(WC_INACT_TITLE_BTN_FG);
	
	d_titlebtn(wd, x, y, w, h, flags);
	
	win_frame7(wd, fg, x + 4 * tl, y + 4 * tl, w - 8 * tl, h - 8 * tl, tl * 2);
	return 0;
}

static int d_minibtn(int wd, int x, int y, int w, int h, int flags)
{
	int tl = wm_get(WM_THIN_LINE);
	int spc;
	win_color fg;
	int size;
	int i;
	
	if (w < h)
		size = w;
	else
		size = h;
	
	spc  = size - 5 * tl;
	spc /= 2;
	
	if (flags & DB_DEPSD)
		fg = wc_get(WC_SEL_FG);
	else if (flags & DB_FOCUS)
		fg = wc_get(WC_TITLE_BTN_FG);
	else
		fg = wc_get(WC_INACT_TITLE_BTN_FG);
	
	d_titlebtn(wd, x, y, w, h, flags);
	
	win_frame7(wd, fg, x + spc, y + spc, w - 2 * spc, h - 2 * spc, tl * 2);
	return 0;
}

static int d_menubtn(int wd, int x, int y, int w, int h, int flags)
{
	int tl = wm_get(WM_THIN_LINE);
	win_color fg;
	int size;
	int i;
	
	if (w < h)
		size = w;
	else
		size = h;
	
	if (flags & DB_DEPSD)
		fg = wc_get(WC_SEL_FG);
	else if (flags & DB_FOCUS)
		fg = wc_get(WC_TITLE_BTN_FG);
	else
		fg = wc_get(WC_INACT_TITLE_BTN_FG);
	
	d_titlebtn(wd, x, y, w, h, flags);
	
	win_rect(wd, fg, x + 4 * tl, y + h / 2 - 2 * tl, w - 8 * tl, tl);
	win_rect(wd, fg, x + 4 * tl, y + h / 2 + 2 * tl, w - 8 * tl, tl);
	win_rect(wd, fg, x + 4 * tl, y + h / 2,		 w - 8 * tl, tl);
	
	return 0;
}

#if 0
static int d_menuframe(int wd, int x, int y, int w, int h)
{
}

static int d_formframe(int wd, int x, int y, int w, int h, int th, int act)
{
}

static int d_formbg(int wd, int x, int y, int w, int h)
{
}

static int d_edboxframe(int wd, int x, int y, int w, int h, int th, int act)
{
}
#endif

const struct form_theme fl_theme =
{
	.d_button	= d_button,
	.d_closebtn	= d_closebtn,
	.d_zoombtn	= d_zoombtn,
	.d_minibtn	= d_minibtn,
	.d_menubtn	= d_menubtn,
	// .d_menuframe	= d_menuframe,
	// .d_formframe	= d_formframe,
	// .d_formbg	= d_formbg,
	// .d_edboxframe	= d_edboxframe,
	
	.form_shadow_w	= 0,
	.edbox_shadow_w	= 0,
};
