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

#include <sys/signal.h>
#include <unistd.h>
#include <wingui.h>
#include <stdlib.h>
#include <event.h>
#include <errno.h>
#include <stdio.h>
#include <err.h>

static struct win_rgba *buf;
static win_color *rbuf;

static int win_w, win_h;
static int wd;

static struct comp
{
	int r, g, b;
	int angle;
	int x, y;
} comp[] =
{
	{ .r = 255, .g =   0, .b =   0, .angle = 10, .x = 500, .y = 300 },
	{ .r =   0, .g = 255, .b =   0, .angle = 70, .x =   0, .y =   0 },
	{ .r =   0, .g =   0, .b = 255, .angle = 60, .x =   0, .y =   0 },
	{ .r = 128, .g = 128, .b = 255, .angle = 50, .x =   0, .y =   0 },
};

static int sine_tab[360]=
{
	    0,   17,   34,   52,   69,   87,
	  104,  121,  139,  156,  173,  190,
	  207,  224,  241,  258,  275,  292,
	  309,  325,  342,  358,  374,  390,
	  406,  422,  438,  453,  469,  484,
	  499,  515,  529,  544,  559,  573,
	  587,  601,  615,  629,  642,  656,
	  669,  681,  694,  707,  719,  731,
	  743,  754,  766,  777,  788,  798,
	  809,  819,  829,  838,  848,  857,
	  866,  874,  882,  891,  898,  906,
	  913,  920,  927,  933,  939,  945,
	  951,  956,  961,  965,  970,  974,
	  978,  981,  984,  987,  990,  992,
	  994,  996,  997,  998,  999,  999,
	 1000,  999,  999,  998,  997,  996,
	  994,  992,  990,  987,  984,  981,
	  978,  974,  970,  965,  961,  956,
	  951,  945,  939,  933,  927,  920,
	  913,  906,  898,  891,  882,  874,
	  866,  857,  848,  838,  829,  819,
	  809,  798,  788,  777,  766,  754,
	  743,  731,  719,  707,  694,  681,
	  669,  656,  642,  629,  615,  601,
	  587,  573,  559,  544,  529,  515,
	  499,  484,  469,  453,  438,  422,
	  406,  390,  374,  358,  342,  325,
	  309,  292,  275,  258,  241,  224,
	  207,  190,  173,  156,  139,  121,
	  104,   87,   69,   52,   34,   17,
	    0,  -17,  -34,  -52,  -69,  -87,
	 -104, -121, -139, -156, -173, -190,
	 -207, -224, -241, -258, -275, -292,
	 -309, -325, -342, -358, -374, -390,
	 -406, -422, -438, -453, -469, -484,
	 -499, -515, -529, -544, -559, -573,
	 -587, -601, -615, -629, -642, -656,
	 -669, -681, -694, -707, -719, -731,
	 -743, -754, -766, -777, -788, -798,
	 -809, -819, -829, -838, -848, -857,
	 -866, -874, -882, -891, -898, -906,
	 -913, -920, -927, -933, -939, -945,
	 -951, -956, -961, -965, -970, -974,
	 -978, -981, -984, -987, -990, -992,
	 -994, -996, -997, -998, -999, -999,
	-1000, -999, -999, -998, -997, -996,
	 -994, -992, -990, -987, -984, -981,
	 -978, -974, -970, -965, -961, -956,
	 -951, -945, -939, -933, -927, -920,
	 -913, -906, -898, -891, -882, -874,
	 -866, -857, -848, -838, -829, -819,
	 -809, -798, -788, -777, -766, -754,
	 -743, -731, -719, -707, -694, -681,
	 -669, -656, -642, -629, -615, -601,
	 -587, -573, -559, -544, -529, -515,
	 -500, -484, -469, -453, -438, -422,
	 -406, -390, -374, -358, -342, -325,
	 -309, -292, -275, -258, -241, -224,
	 -207, -190, -173, -156, -139, -121,
	 -104,  -87,  -69,  -52,  -34,  -17
};

static int slope[] =
{   0,   1,   2,   4,   8,  16, 32,
   64,  96, 128, 160, 192,
  224, 240, 248, 252, 254, 255
};

#define SLOPE_CNT	(sizeof slope / sizeof *slope)

#define IMAGE_WIDTH	16
#define IMAGE_HEIGHT	16

#define abs(x)		((x) >= 0 ? (x) : -(x))
#define clamp(x, l)	((x) > (l) ? (l) : (x))

static void redraw(int x, int y, int w, int h)
{
	win_color bg, fg;
	
	win_rgb2color(&bg, 255, 255, 255);
	win_rgb2color(&fg, 0, 0, 0);
	
	win_clip(wd, x, y, w, h, 0, 0);
	win_paint();
	win_bitmap(wd, rbuf, 0, 0, win_w, win_h);
	win_btext(wd, bg, fg, 0, 0, "Copyright (C) Piotr Durlej");
	win_end_paint();
}

static void event(struct event *e)
{
	switch (e->win.type)
	{
		case WIN_E_REDRAW:
			redraw(e->win.redraw_x, e->win.redraw_y,
			       e->win.redraw_w, e->win.redraw_h);
			break;
		case WIN_E_PTR_DOWN:
		case WIN_E_KEY_DOWN:
			exit(0);
		default:
			;
	}
}

static inline int isin(int v)
{
	return sine_tab[v % 360];
}

static inline int icos(int v)
{
	return sine_tab[(v + 90) % 360];
}

static void addcomp(struct comp *cp)
{
	struct win_rgba *pp;
	int r, g, b;
	int tx, ty;
	int sx, sy;
	int x, y;
	int it;
	int p;
	int a;
	
	for (pp = buf, sy = 0, ty = -cp->y; sy < win_h; sy++, ty++)
		for (sx = 0, tx = -cp->x; sx < win_w; sx++, pp++, tx++)
		{
			x  =   tx * icos(cp->angle) + ty * isin(cp->angle);
			y  = - tx * isin(cp->angle) + ty * icos(cp->angle);
			
			x /= 1000;
			y /= 1000;
			x = abs(x);
			y = abs(y);
			
			p  = SLOPE_CNT * 4;
			p += isin(y) / 100;
			p += y;
			
			a  = x % p;
			a -= p / 2;
			a  = abs(a);
			a *= SLOPE_CNT - 1;
			a /= p >> 1;
			it = slope[a];
			x %= IMAGE_WIDTH;
			y %= IMAGE_HEIGHT;
			
			r = pp->r + it * cp->r / 255;
			g = pp->g + it * cp->g / 255;
			b = pp->b + it * cp->b / 255;
			pp->r = clamp(r, 255);
			pp->g = clamp(g, 255);
			pp->b = clamp(b, 255);
		}
}

static void paint(void)
{
	static long t;
	
	struct win_rgba *p;
	struct comp *cp;
	win_color bg, fg;
	unsigned i, cnt;
	
	cnt = win_w * win_h;
	for (p = buf, i = 0; i < cnt; i++, p++)
		p->r = p->g = p->b = 0;
	for (cp = comp, i = 0; i < sizeof comp / sizeof *comp; i++, cp++)
	{
		cp->x = isin(t * (i + 1));
		cp->y = icos(t * (i + 1));
		cp->angle += i + 1;
		cp->angle %= 360;
		addcomp(&comp[i]);
	}
	t++;
	
	win_rgb2color(&bg, 255, 255, 255);
	win_rgb2color(&fg, 0, 0, 0);
	win_bconv(rbuf, buf, win_w * win_h);
	win_clip(wd, 0, 0, win_w, win_h, 0, 0);
	win_paint();
	win_bitmap(wd, rbuf, 0, 0, win_w, win_h);
	win_btext(wd, bg, fg, 0, 0, "Copyright (C) Piotr Durlej");
	win_end_paint();
}

int main(int argc, char **argv)
{
	struct win_rgba *p;
	struct win_rgba *e;
	unsigned cnt;
	
	if (win_attach())
		err(255, NULL);
	
	win_desktop_size(&win_w, &win_h);
	if (win_creat(&wd, 1, 0, 0, win_w, win_h, event, NULL))
	{
		perror("win_creat");
		return errno;
	}
	win_raise(wd);
	win_focus(wd);
	
	cnt  = win_w * win_h;
	rbuf = malloc(sizeof *rbuf * cnt);
	buf  = malloc(sizeof *buf  * cnt);
	if (!buf || !rbuf)
		return errno;
	p = buf;
	e = buf + cnt;
	while (p < e)
		p++->a = 255;
	for (;;)
	{
		win_idle();
		paint();
	}
}
