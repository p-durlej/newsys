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

#include <wingui_metrics.h>
#include <wingui_bitmap.h>
#include <wingui_msgbox.h>
#include <wingui_color.h>
#include <wingui_form.h>
#include <wingui.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

static struct bitmap *bmp_s;
static struct bitmap *bmp;
static struct gadget *p;
static struct form *f;

static void f_resize(struct form *f, int w, int h)
{
	struct bitmap *o = bmp_s;
	int ow, oh;
	
	bmp_size(bmp, &ow, &oh);
	
	pict_set_bitmap(p, NULL);
	if (w == ow && h == oh)
	{
		pict_set_bitmap(p, bmp);
		bmp_s = NULL;
		bmp_free(o);
		return;
	}
	
	bmp_s = bmp_scale(bmp, w, h);
	if (!bmp_s)
	{
		msgbox_perror(f, "Picture Viewer", "Cannot rescale picture", errno);
		return;
	}
	if (bmp_conv(bmp_s))
	{
		msgbox_perror(f, "Picture Viewer", "Cannot convert picture", errno);
		return;
	}
	pict_set_bitmap(p, bmp_s);
	bmp_free(o);
	return;
}

static int f_close(struct form *f)
{
	exit(0);
}

int main(int argc, char **argv)
{
	struct win_rgba bg;
	struct win_rect ws;
	struct win_rect r;
	char *msg;
	int se;
	int fr;
	
	if (win_attach())
		err(errno, NULL);
	
	if (argc != 2)
	{
		msgbox(NULL, "Picture Viewer", "usage:\n  pictview PATHNAME");
		return 0;
	}
	
	f = form_creat(FORM_APPFLAGS, 0, -1, -1, 160, 100, argv[1]);
	p = pict_creat(f, 0, 0, NULL);
	
	wc_get_rgba(WC_WIN_BG, &bg);
	
	bmp = bmp_load(argv[1]);
	if (!bmp)
	{
		se = errno;
		if (asprintf(&msg, "Cannot open \"%s\"", argv[1]) < 0)
			return se;
		
		msgbox_perror(NULL, "Picture Viewer", msg, se);
		return se;
	}
	bmp_set_bg(bmp, bg.r, bg.g, bg.b, bg.a);
	bmp_conv(bmp);
	pict_set_bitmap(p, bmp);
	
	fr = wm_get(WM_FRAME);
	win_ws_getrect(&ws);
	ws.w -= fr * 3;
	ws.h -= fr * 3 + 16;
	r = ws;
	
	if (r.w > p->rect.w)
		r.w = p->rect.w;
	if (r.h > p->rect.h)
		r.h = p->rect.h;
	r.x = (ws.w - r.w) / 2;
	r.y = (ws.h - r.h) / 2;
	
	form_resize(f, r.w, r.h);
	form_move(f, r.x, r.y);
	form_on_resize(f, f_resize);
	form_on_close(f, f_close);
	f_resize(f, r.w, r.h);
	form_show(f);
	
	for (;;)
		win_wait();
}
