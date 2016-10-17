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

#include <wingui_color.h>
#include <sys/signal.h>
#include <wingui.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <confdb.h>
#include <time.h>
#include <event.h>
#include <err.h>

#define WIDTH	150
#define HEIGHT	13

struct
{
	int x;
	int y;
} config;

static int wd;

static void redraw_time(void)
{
	char buf[128];
	win_color fg;
	win_color bg;
	time_t tm;
	char *nl;
	
	tm = time(NULL);
	strcpy(buf, asctime(localtime(&tm)));
	nl = strchr(buf, '\n');
	if (nl)
		*nl = 0;
	strcat(buf, "  ");
	
	bg = wc_get(WC_WIN_BG);
	fg = wc_get(WC_WIN_FG);
	
	win_paint();
	win_btext(wd, bg, fg, 2, 1, buf);
	win_end_paint();
}

static void redraw(void)
{
	win_color hi;
	win_color sh;
	win_color bg;
	
	hi = wc_get(WC_HIGHLIGHT1);
	sh = wc_get(WC_SHADOW1);
	bg = wc_get(WC_WIN_BG);
	
	win_paint();
	win_rect(wd, bg, 0, 0, WIDTH, HEIGHT);
	win_hline(wd, sh, 0, 0, WIDTH);
	win_hline(wd, hi, 0, HEIGHT - 1, WIDTH);
	win_vline(wd, sh, 0, 0, HEIGHT);
	win_vline(wd, hi, WIDTH - 1, 0, HEIGHT);
	redraw_time();
	win_end_paint();
}

static void sigalrm(int nr)
{
	redraw_time();
	
	win_signal(SIGALRM, sigalrm);
	alarm(1);
}

static int event(struct event *e)
{
	static int down = 0;
	static int x0;
	static int y0;
	
	switch (e->win.type)
	{
	case WIN_E_PTR_DOWN:
		if (!down)
		{
			x0 = e->win.ptr_x;
			y0 = e->win.ptr_y;
		}
		down++;
		break;
	case WIN_E_PTR_UP:
		if (down)
		{
			down--;
			
			if (!down)
			{
				config.x += e->win.ptr_x - x0;
				config.y += e->win.ptr_y - y0;
				
				win_change(wd, 1, config.x, config.y, WIDTH, HEIGHT);
				win_rect_preview(0, 0, 0, 0, 0);
			}
		}
		break;
	case WIN_E_PTR_MOVE:
		if (down)
			win_rect_preview(1, config.x + e->win.ptr_x - x0, config.y + e->win.ptr_y - y0, WIDTH, HEIGHT);
		break;
	case WIN_E_REDRAW:
		redraw();
		break;
	}
	
	return 0;
}

static void sigterm()
{
	c_save("clock", &config, sizeof config);
	exit(0);
}

static void load_config()
{
	c_load("clock", &config, sizeof config);
}

int main(int argc, char **argv)
{
	if (win_attach())
		err(255, NULL);
	
	load_config();
	win_creat(&wd, 1, config.x, config.y, WIDTH, HEIGHT, event, NULL);
	win_raise(wd);
	
	signal(SIGTERM, sigterm);
	signal(SIGALRM, sigalrm);
	alarm(1);
	
	while (!win_wait());
	return errno;
}
