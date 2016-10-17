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

#include <priv/wingui_theme.h>
#include <wingui_metrics.h>
#include <wingui_color.h>
#include <wingui.h>

#define min(a, b)	((a) < (b) ? (a) : (b))

static int d_button(int wd, int x, int y, int w, int h, const char *label, int flags)
{
	win_color fr, fr1, bg, fg;
	int tx, ty, tw, th;
	
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
	
	win_hline(wd, fr, x,		y,	   w);
	win_hline(wd, fr, x,		y + h - 1, w);
	win_vline(wd, fr, x,		y + 1,     h - 2);
	win_vline(wd, fr, x + w - 1,	y + 1,     h - 2);
	
	win_hline(wd, fr1, x + 1,	y + 1,	   w - 2);
	win_hline(wd, fr1, x + 1,	y + h - 2, w - 2);
	win_vline(wd, fr1, x + 1,	y + 2,     h - 4);
	win_vline(wd, fr1, x + w - 2,	y + 2,     h - 4);
	
	win_rect(wd, bg, x + 2, y + 2, w - 4, h - 4);
	
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
		win_rgba2color(&fg, 192, 0, 0, 255);
	else
		fg = wc_get(WC_INACT_TITLE_BTN_FG);
	
	d_titlebtn(wd, x, y, w, h, flags);
	
	for (i = 4; i < size - 5; i++)
	{
		win_pixel(wd, fg, x + i,		y + i);
		win_pixel(wd, fg, x + size - i - 1,	y + i);
		
		win_pixel(wd, fg, x + i,		y + i + 1);
		win_pixel(wd, fg, x + size - i - 1,	y + i + 1);
		
		win_pixel(wd, fg, x + i + 1,		y + i);
		win_pixel(wd, fg, x + size - i - 2,	y + i);
	}
	
	win_pixel(wd, fg, x + i,		y + i);
	win_pixel(wd, fg, x + size - i - 1,	y + i);
	
	return 0;
}

static int d_zoombtn(int wd, int x, int y, int w, int h, int flags)
{
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
	
	win_hline(wd, fg, x + 4,	y + 4,	   w - 8);
	win_vline(wd, fg, x + 4,	y + 4,	   h - 8);
	win_hline(wd, fg, x + 4,	y + h - 5, w - 8);
	win_vline(wd, fg, x + w - 5,	y + 4,	   h - 8);
	
	win_hline(wd, fg, x + 5,	y + 5,	   w - 10);
	win_vline(wd, fg, x + 5,	y + 5,	   h - 10);
	win_hline(wd, fg, x + 5,	y + h - 6, w - 10);
	win_vline(wd, fg, x + w - 6,	y + 5,	   h - 10);
	
	return 0;
}

static int d_minibtn(int wd, int x, int y, int w, int h, int flags)
{
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
	
	win_hline(wd, fg, x + 6,	y + 6,	   w - 12);
	win_vline(wd, fg, x + 6,	y + 6,	   h - 12);
	win_hline(wd, fg, x + 6,	y + h - 7, w - 12);
	win_vline(wd, fg, x + w - 7,	y + 6,	   h - 12);
	
	win_hline(wd, fg, x + 7,	y + 7,	   w - 14);
	win_vline(wd, fg, x + 7,	y + 7,	   h - 14);
	win_hline(wd, fg, x + 7,	y + h - 8, w - 14);
	win_vline(wd, fg, x + w - 8,	y + 7,	   h - 14);
	
	return 0;
}

static int d_menubtn(int wd, int x, int y, int w, int h, int flags)
{
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
	
	win_hline(wd, fg, x + 4, y + h / 2 - 2, w - 8);
	win_hline(wd, fg, x + 4, y + h / 2 + 2, w - 8);
	win_hline(wd, fg, x + 4, y + h / 2,	w - 8);
	
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
