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

#include <wingui_bitmap.h>
#include <wingui_color.h>
#include <wingui_form.h>
#include <wingui.h>

static void pict_remove(struct gadget *g)
{
	bmp_free(g->pict.bitmap);
}

static void pict_redraw(struct gadget *g, int wd)
{
	if (!g->pict.bitmap)
	{
		win_rect(wd, wc_get(WC_WIN_BG), 0, 0, g->rect.w, g->rect.h);
		return;
	}
	bmp_draw(wd, g->pict.bitmap, 0, 0);
}

struct gadget *pict_creat(struct form *form, int x, int y, const char *pathname)
{
	struct gadget *g;
	
	g = gadget_creat(form, x, y, 0, 0);
	if (!g)
		return NULL;
	g->remove = pict_remove;
	g->redraw = pict_redraw;
	if (pathname && pict_load(g, pathname))
	{
		gadget_remove(g);
		return NULL;
	}
	
	return g;
}

int pict_set_bitmap(struct gadget *g, struct bitmap *bmp)
{
	int w, h;
	
	g->pict.bitmap = bmp;
	if (!bmp)
	{
		gadget_resize(g, 0, 0);
		return 0;
	}
	bmp_size(bmp, &w, &h);
	gadget_resize(g, w, h);
	gadget_redraw(g);
	return 0;
}

int pict_scale(struct gadget *g, int w, int h)
{
	struct bitmap *nb;
	
	if (g->rect.w == w && g->rect.h == h)
		return 0;
	
	if (!g->pict.bitmap)
	{
		gadget_resize(g, w, h);
		return 0;
	}
	nb = bmp_scale(g->pict.bitmap, w, h);
	if (!nb)
		return -1;
	if (bmp_conv(nb))
	{
		bmp_free(nb);
		return -1;
	}
	
	bmp_free(g->pict.bitmap);
	g->pict.bitmap = nb;
	gadget_resize(g, w, h);
	return 0;
}

int pict_load(struct gadget *g, const char *pathname)
{
	struct win_rgba bg;
	struct bitmap *bmp;
	
	if (!pathname)
	{
		bmp_free(g->pict.bitmap);
		g->pict.bitmap = NULL;
		gadget_resize(g, 0, 0);
		return 0;
	}
	
	bmp = bmp_load(pathname);
	if (!bmp)
		return -1;
	
	wc_get_rgba(WC_WIN_BG, &bg);
	if (!g->no_bg)
		bmp_set_bg(bmp, bg.r, bg.g, bg.b, bg.a);
	
	if (bmp_conv(bmp))
	{
		bmp_free(bmp);
		return -1;
	}
	bmp_free(g->pict.bitmap);
	pict_set_bitmap(g, bmp);
	return 0;
}
