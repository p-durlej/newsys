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

#include <wingui.h>
#include <event.h>
#include <stdlib.h>
#include <err.h>

static struct win_modeinfo mi;
static struct win_rect wr;
static int wd;
static int scale = 1;

static int win_event(struct event *e, void *data)
{
	win_color bg, fr;
	int i;
	
	switch (e->win.type)
	{
	case WIN_E_SWITCH_FROM:
	case WIN_E_PTR_DOWN:
		exit(0);
	case WIN_E_KEY_DOWN:
		if (e->win.ch == WIN_KEY_F4 && (e->win.shift & WIN_SHIFT_ALT))
			exit(0);
		if (e->win.ch == '\033')
			exit(0);
		break;
	case WIN_E_REDRAW:
		win_clip(wd, e->win.redraw_x, e->win.redraw_y,
			     e->win.redraw_w, e->win.redraw_h,
			     0, 0);
		win_paint();
		win_rgb2color(&fr, 255, 255, 255);
		win_rgb2color(&bg, 0, 0, 0);
		win_rect(wd, bg, 0, 0, wr.w, wr.h);
		win_hline(wd, fr, 0, wr.h - 1, mi.ncolors * scale + 4);
		win_hline(wd, fr, 0, 0, mi.ncolors * scale + 4);
		win_vline(wd, fr, wr.w - 1, 0, mi.ncolors * scale + 4);
		win_vline(wd, fr, 0, 0, mi.ncolors * scale + 4);
		for (i = 0; i < mi.ncolors; i++)
			win_rect(wd, i, 2 + i * scale, 2, scale, wr.h - 4);
		win_end_paint();
	}
	return 0;
}

int main(int argc, char **argv)
{
	struct win_rect wsr;
	int m;
	
	if (win_attach())
		err(255, NULL);
	
	win_getmode(&m);
	win_modeinfo(m, &mi);
	win_ws_getrect(&wsr);
	if (mi.ncolors > wsr.w)
		errx(1, "too many colors");
	while (mi.ncolors * scale * 2 <= wsr.w)
		scale++;
	wr.w = mi.ncolors * scale + 4;
	wr.h = wsr.h / 16;
	wr.x = wsr.x + (wsr.w - wr.w) / 2;
	wr.y = wsr.y + (wsr.h - wr.h) / 2;
	win_creat(&wd, 1, wr.x, wr.y, wr.w, wr.h, win_event, NULL); /* XXX */
	win_raise(wd);
	win_focus(wd);
	for (;;)
		win_wait();
}
