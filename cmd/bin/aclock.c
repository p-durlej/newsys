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

#include <priv/copyright.h>
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

#define WIDTH	160
#define HEIGHT	160

#define abs(a)	((a) >= 0 ? (a) : -(a))

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

struct
{
	int x;
	int y;
} config;

struct form *	main_form;
struct gadget *	clock_gadget;

void old_clock_redraw(struct gadget *g, int wd)
{
	win_color bg;
	win_color fg;
	time_t t;
	char *s;
	int w, h;
	
	time(&t);
	s = ctime(&t);
	
	win_rgb2color(&bg, 255, 255, 255);
	win_rgb2color(&fg, 0, 0, 0);
	
	win_text_size(WIN_FONT_DEFAULT, &w, &h, s);
	win_rect(wd, bg, 0, 0, WIDTH, HEIGHT);
	win_text(wd, fg, (WIDTH-w) >> 1, (HEIGHT-h) >> 1, s);
}

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
	int x, y;
	int i;
	
	fg = wc_get(WC_FG);
	
	for (i = 0; i < 12; i++)
	{
		x = sine_tab[i * 5];
		y = sine_tab[(i * 5 + 15) % 60];
		
		win_line(wd, fg, (WIDTH  >> 1) + x * 60 / 1000,
				 (HEIGHT >> 1) - y * 60 / 1000,
				 (WIDTH  >> 1) + x * 70 / 1000,
				 (HEIGHT >> 1) - y * 70 / 1000);
	}
	
	for (i = 0; i < 60; i++)
	{
		if (i % 5 == 0)
			continue;
		
		x = sine_tab[i];
		y = sine_tab[(i + 15) % 60];
		
		win_line(wd, fg, (WIDTH  >> 1) + x * 65 / 1000,
				 (HEIGHT >> 1) - y * 65 / 1000,
				 (WIDTH  >> 1) + x * 70 / 1000,
				 (HEIGHT >> 1) - y * 70 / 1000);
	}
}

static void draw_hand(int wd, win_color c, int w, int l, int a)
{
	int x0, y0, x1, y1, x2, y2, x3, y3;
	
	x0 = WIDTH  >> 1;
	y0 = HEIGHT >> 1;
	
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
	time_t t;
	
	time(&t);
	tm = localtime(&t);
	
	win_rgb2color(&red, 255, 0, 0);
	
	bg = wc_get(WC_BG);
	fg = wc_get(WC_FG);
	
	win_rect(wd, bg, 0, 0, WIDTH, HEIGHT);
	draw_marks(wd);
	
	win_rect(wd, fg, (WIDTH >> 1) - 3, (HEIGHT >> 1) - 3, 6, 6);
	
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

void sig_term(int nr)
{
	c_save("aclock", &config, sizeof config);
	exit(0);
}

void on_move(struct form *f, int x, int y)
{
	config.x = x;
	config.y = y;
}

int on_close(struct form *f)
{
	c_save("aclock", &config, sizeof config);
	exit(0);
}

static void about_click(struct menu_item *mi)
{
	dlg_about7(main_form, NULL, "Clock v1", SYS_PRODUCT, SYS_AUTHOR, SYS_CONTACT, "clock.pnm");
}

int main()
{
	struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
	struct menu *mm, *m;
	struct win_rect wsr;
	
	if (win_attach())
		err(255, NULL);
	
	win_ws_getrect(&wsr);
	
	config.x = 20;
	config.y = 20;
	c_load("aclock", &config, sizeof config);
	
	if (config.x + WIDTH > wsr.w)
		config.x = wsr.w - WIDTH;
	
	if (config.y + HEIGHT > wsr.h)
		config.y = wsr.h - HEIGHT;
	
	mm = menu_creat();
	
	m = menu_creat();
	menu_newitem(m, "About ...", about_click);
	menu_submenu(mm, "Options", m);
	
	main_form = form_creat(
		   FORM_TITLE | FORM_FRAME | FORM_ALLOW_CLOSE | FORM_ALLOW_MINIMIZE | FORM_NO_BACKGROUND, 1,
		   config.x, config.y, WIDTH, HEIGHT, "Clock");
	form_set_menu(main_form, mm);
	form_on_close(main_form, on_close);
	form_on_move(main_form, on_move);
	
	clock_gadget = gadget_creat(main_form, 0, 0, WIDTH, HEIGHT);
	clock_gadget->redraw = clock_redraw;
	
	tmr_creat(&tv, &tv, timer_proc, NULL, 1);
	
	for (;;)
		win_wait();
}
