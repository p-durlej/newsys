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

#include <wingui_form.h>
#include <wingui.h>
#include <stdlib.h>
#include <err.h>

#define BOX_WIDTH	80
#define BOX_HEIGHT	80

static struct gadget *	g;
static struct form *	f;
static int		dir_x = 1;
static int		dir_y = 1;
static int		box_x, box_y;
static int		win_w = 320, win_h = 240;

static void g_redraw(struct gadget *g, int wd)
{
	win_color bg;
	win_color fg;
	
	win_rgba2color(&fg, 255, 255, 255, 64);
	win_rgba2color(&bg, 0, 0, 0, 255);
	
	win_rect(wd, bg, 0, 0, win_w, box_y);
	win_rect(wd, bg, 0, box_y + BOX_WIDTH, win_w, win_h - box_y - BOX_HEIGHT);
	win_rect(wd, bg, 0, box_y, box_x, BOX_HEIGHT);
	win_rect(wd, bg, box_x + BOX_WIDTH, box_y, win_w - BOX_WIDTH - box_x, BOX_HEIGHT);
	
	win_rect(wd, fg, box_x, box_y, BOX_WIDTH, BOX_HEIGHT);
}

static void f_resize(struct form *f, int w, int h)
{
	win_w = w;
	win_h = h;
	gadget_resize(g, w, h);
}

static int f_close(struct form *f)
{
	exit(0);
}

int main()
{
	if (win_attach())
		err(255, NULL);
	
	f = form_creat(FORM_NO_BACKGROUND | FORM_APPFLAGS, 1, -1, -1, win_w, win_h, "Box");
	form_on_resize(f, f_resize);
	form_on_close(f, f_close);
	g = gadget_creat(f, 0, 0, win_w, win_h);
	g->redraw = g_redraw;
	
	for (;;)
	{
		if (box_x <= 0)
			dir_x = 1;
		if (box_y <= 0)
			dir_y = 1;
		if (box_x >= win_w - BOX_WIDTH)
			dir_x = -1;
		if (box_y >= win_h - BOX_HEIGHT)
			dir_y = -1;
		box_x += dir_x;
		box_y += dir_y;
		gadget_redraw(g);
		win_idle();
	}
}
