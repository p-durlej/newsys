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

#include <priv/copyright.h>
#include <wingui_cgadget.h>
#include <wingui_metrics.h>
#include <wingui_color.h>
#include <wingui_form.h>
#include <sys/signal.h>
#include <wingui_menu.h>
#include <wingui_dlg.h>
#include <wingui.h>
#include <unistd.h>
#include <stdlib.h>
#include <confdb.h>
#include <timer.h>
#include <time.h>
#include <err.h>

#define SECONDS	1

#define min(a,b)	((a) < (b) ? (a) : (b))
#define abs(a)		((a) >= 0 ? (a) : -(a))

int sine_tab[60] =
{
	    0,  104,  207,  309,  406,  499,
	  587,  669,  743,  809,  866,  913,
	  951,  978,  994, 1000,  994,  978,
	  951,  913,  866,  809,  743,  669,
	  587,  499,  406,  309,  207,  104,
	    0, -104, -207, -309, -406, -499,
	 -587, -669, -743, -809, -866, -913,
	 -951, -978, -994,-1000, -994, -978,
	 -951, -913, -866, -809, -743, -669,
	 -587, -500, -406, -309, -207, -104
};

struct form *	main_form;
struct gadget *	clock_gadget;
struct gadget *	sizebox;

void win_line(int wd, win_color c, int x0, int y0, int x1, int y1)
{
	int tmp;
	int i;
	
	if (abs(x0 - x1) > abs(y0 - y1))
	{
		if (x0 > x1)
		{
			tmp = x0;
			x0  = x1;
			x1  = tmp;
			
			tmp = y0;
			y0  = y1;
			y1  = tmp;
		}
		
		for (i = x0; i <= x1; i++)
			win_pixel(wd, c, i, y0 + (y1 - y0) * (i - x0) / (x1 - x0));
	}
	else
	{
		if (y0 > y1)
		{
			tmp = x0;
			x0  = x1;
			x1  = tmp;
			
			tmp = y0;
			y0  = y1;
			y1  = tmp;
		}
		
		for (i = y0; i <= y1; i++)
			win_pixel(wd, c, x0 + (x1 - x0) * (i - y0) / (y1 - y0), i);
	}
}

void draw_marks(int wd)
{
	win_color fg;
	int d1, d2, d3;
	int sz;
	int w, h;
	int x, y;
	int i;
	
	w = main_form->workspace_rect.w;
	h = main_form->workspace_rect.h;
	
	fg = wc_get(WC_FG);
	
	sz = min(w, h);
	
	d1 = sz * 60 / 160;
	d2 = sz * 65 / 160;
	d3 = sz * 70 / 160;
	
	for (i = 0; i < 12; i++)
	{
		x = sine_tab[i * 5];
		y = sine_tab[(i * 5 + 15) % 60];
		
		win_line(wd, fg, (w >> 1) + x * d1 / 1000,
				 (h >> 1) - y * d1 / 1000,
				 (w >> 1) + x * d3 / 1000,
				 (h >> 1) - y * d3 / 1000);
	}
	
	for (i = 0; i < 60; i++)
	{
		if (i % 5 == 0)
			continue;
		
		x = sine_tab[i];
		y = sine_tab[(i + 15) % 60];
		
		win_line(wd, fg, (w >> 1) + x * d2 / 1000,
				 (h >> 1) - y * d2 / 1000,
				 (w >> 1) + x * d3 / 1000,
				 (h >> 1) - y * d3 / 1000);
	}
}

static void draw_hand(int wd, win_color c, int w, int l, int a)
{
	int x0, y0, x1, y1, x2, y2, x3, y3;
	int fw, fh;
	int sz;
	
	fw = main_form->workspace_rect.w;
	fh = main_form->workspace_rect.h;
	
	sz = min(fw, fh);
	
	l *= sz;
	l /= 160;
	
	w *= sz;
	w /= 160;
	
	x0 = fw >> 1;
	y0 = fh >> 1;
	
	x1 = x0 + (-sine_tab[a] - sine_tab[(a + 15) % 60]) * w / 1000;
	y1 = y0 + (-sine_tab[a] + sine_tab[(a + 15) % 60]) * w / 1000;
	
	x2 = x0 + (-sine_tab[a] + sine_tab[(a + 15) % 60]) * w / 1000;
	y2 = y0 + ( sine_tab[a] + sine_tab[(a + 15) % 60]) * w / 1000;
	
	x3 = x0 + sine_tab[a]		  * l / 1000;
	y3 = y0 - sine_tab[(a + 15) % 60] * l / 1000;
	
	win_line(wd, c, x1, y1, x3, y3);
	win_line(wd, c, x2, y2, x3, y3);
	win_line(wd, c, x1, y1, x2, y2);
}

void clock_redraw(struct gadget *g, int wd)
{
	win_color red;
	win_color bg;
	win_color fg;
	struct tm *tm;
	int fw, fh;
	int sbw;
	time_t t;
	
	fw = main_form->workspace_rect.w;
	fh = main_form->workspace_rect.h;
	
	sbw = wm_get(WM_SCROLLBAR);
	
	time(&t);
	tm = localtime(&t);
	
	win_rgb2color(&red, 255, 0, 0);
	
	bg = wc_get(WC_BG);
	fg = wc_get(WC_FG);
	
	win_rect(wd, bg, 0, 0, fw, fh - sbw);
	win_rect(wd, bg, 0, fh - sbw, fw - sbw, sbw);
	draw_marks(wd);
	
	win_rect(wd, fg, (fw >> 1) - 3, (fh >> 1) - 3, 6, 6);
	
#if SECONDS
	draw_hand(wd, red, 2, 60, tm->tm_sec);
#endif
	
	draw_hand(wd, fg,  4, 55,  tm->tm_min);
	draw_hand(wd, fg,  6, 40, (tm->tm_hour * 5) % 60);
}

static void timer_proc(void *data)
{
	static time_t pt = -1L;
	static time_t t;
	
	time(&t);
#if SECONDS
	if (t != pt)
#else
	if (t / 60 != pt / 60)
#endif
	{
		gadget_redraw(clock_gadget);
		pt = t;
	}
}

static void save_fst(void)
{
	struct form_state fst;
	
	form_get_state(main_form, &fst);
	c_save("aclock", &fst, sizeof fst);
}

void sig_term(int nr)
{
	save_fst();
	exit(0);
}

void on_resize(struct form *f, int w, int h)
{
	if (clock_gadget != NULL)
		gadget_resize(clock_gadget, w, h);
}

void on_move(struct form *f, int x, int y)
{
}

int on_close(struct form *f)
{
	save_fst();
	exit(0);
}

static void about_click(struct menu_item *mi)
{
	dlg_about7(main_form, NULL, "Clock v1.4", SYS_PRODUCT, SYS_AUTHOR, SYS_CONTACT, "clock.pnm");
}

static void update_colors(void)
{
	win_color c;
	
	win_rgb2color(&c, 255, 255, 255);
	sizebox_set_bg(sizebox, c);
}

int main()
{
	struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
	struct menu *mm, *m;
	struct win_rect wsr;
	struct form_state fst;
	int sbw;
	
	if (win_attach())
		err(255, NULL);
	
	sbw = wm_get(WM_SCROLLBAR);
	
	win_ws_getrect(&wsr);
	
	mm = menu_creat();
	
	m = menu_creat();
	menu_newitem(m, "About ...", about_click);
	menu_submenu(mm, "Options", m);
	
	main_form = form_creat(FORM_APPFLAGS | FORM_NO_BACKGROUND, 0, 20, 20, 160, 160, "Clock");
	form_set_menu(main_form, mm);
	form_on_resize(main_form, on_resize);
	form_on_close(main_form, on_close);
	form_on_move(main_form, on_move);
	form_min_size(main_form, 80, 80);
	
	if (!c_load("aclock", &fst, sizeof fst))
		form_set_state(main_form, &fst);
	
	clock_gadget = gadget_creat(main_form, 0, 0, main_form->workspace_rect.w, main_form->workspace_rect.h);
	gadget_set_redraw_cb(clock_gadget, clock_redraw);
	
	sizebox = sizebox_creat(main_form, sbw, sbw);
	update_colors();
	
	tmr_creat(&tv, &tv, timer_proc, NULL, 1);
	
	win_on_setmode(update_colors);
	
	form_show(main_form);
	
	for (;;)
		win_wait();
}
