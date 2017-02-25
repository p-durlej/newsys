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

#include <wingui_color.h>
#include <wingui_form.h>
#include <wingui.h>

static void sizebox_redraw(struct gadget *g, int wd)
{
	win_color fg;
	win_color bg;
	
	fg = wc_get(WC_WIN_FG);
	bg = wc_get(WC_WIN_BG);
	
	win_rect(wd, bg, 0, 0, g->rect.w, g->rect.h);
}

static void sizebox_remove(struct gadget *g)
{
	g->form->sizebox = NULL;
}

struct gadget *sizebox_creat(struct form *f, int w, int h)
{
	struct gadget *g;
	
	g = gadget_creat(f, f->workspace_rect.w - w, f->workspace_rect.h - h, w, h);
	if (!g)
		return NULL;
	
	g->remove = sizebox_remove;
	g->redraw = sizebox_redraw;
	gadget_redraw(g);
	gadget_setptr(g, WIN_PTR_NW_SE);
	
	f->sizebox = g;
	return g;
}
