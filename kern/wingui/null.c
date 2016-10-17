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

#include <kern/wingui.h>
#include <errno.h>

static int modeinfo(void *dd, int mode, struct win_modeinfo *buf);
static int setmode(void *dd, int mode, int refresh);

static void setptr(void *dd, const win_color *shape, const unsigned *mask);
static void moveptr(void *dd, int x, int y);
static void showptr(void *dd);
static void hideptr(void *dd);

static void putpix(void *dd, int x, int y, win_color c);
static void getpix(void *dd, int x, int y, win_color *c);
static void hline(void *dd, int x, int y, int len, win_color c);
static void vline(void *dd, int x, int y, int len, win_color c);
static void rect(void *dd, int x, int y, int w, int h, win_color c);
static void copy(void *dd, int x0, int y0, int x1, int y1, int w, int h);

static void rgba2color(void *dd, int r, int g, int b, int a, win_color *c);
static void color2rgba(void *dd, int *r, int *g, int *b, int *a, win_color c);
static void invert(void *dd, win_color *c);

struct win_display null_display =
{
	width:		1,
	height:		1,
	ncolors:	2,
	user_cte:	0,
	user_cte_count:	0,
	mode:		0,
	
	modeinfo:	modeinfo,
	setmode:	setmode,
	
	setptr:		setptr,
	moveptr:	moveptr,
	showptr:	showptr,
	hideptr:	hideptr,
	
	putpix:		putpix,
	getpix:		getpix,
	hline:		hline,
	vline:		vline,
	rect:		rect,
	copy:		copy,
	
	rgba2color:	rgba2color,
	color2rgba:	color2rgba,
	invert:		invert,
	
	setcte:		NULL,
};

static int setmode(void *dd, int mode, int refresh)
{
	if (mode)
		return EINVAL;
	
	return 0;
}

static int modeinfo(void *dd, int mode, struct win_modeinfo *buf)
{
	if (mode)
		return EINVAL;
	
	buf->width = 1;
	buf->height = 1;
	buf->ncolors = 2;
	return 0;
}

static void setptr(void *dd, const win_color *shape, const unsigned *mask)
{
}

static void moveptr(void *dd, int x, int y)
{
}

static void showptr(void *dd)
{
}

static void hideptr(void *dd)
{
}

static void putpix(void *dd, int x, int y, win_color c)
{
}

static void getpix(void *dd, int x, int y, win_color *c)
{
	*c = 0;
}

static void hline(void *dd, int x, int y, int len, win_color c)
{
}

static void vline(void *dd, int x, int y, int len, win_color c)
{
}

static void rect(void *dd, int x, int y, int w, int h, win_color c)
{
}

static void copy(void *dd, int x0, int y0, int x1, int y1, int w, int h)
{
}

static void rgba2color(void *dd, int r, int g, int b, int a, win_color *c)
{
	*c = 0;
}

static void color2rgba(void *dd, int *r, int *g, int *b, int *a, win_color c)
{
	*r = 0;
	*g = 0;
	*b = 0;
}

static void invert(void *dd, win_color *c)
{
	*c = 1 - *c;
}
