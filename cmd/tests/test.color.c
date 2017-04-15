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

#include <wingui_cgadget.h>
#include <wingui_form.h>
#include <wingui_dlg.h>
#include <wingui.h>
#include <stdlib.h>
#include <err.h>

struct win_rgba wc = { 255, 255, 0, 255 };

void g_redraw(struct gadget *g, int wd)
{
	win_color c;
	
	win_rgb2color(&c, wc.r, wc.g, wc.b);
	win_rect(wd, c, 0, 0, 160, 160);
}

void g_ptr_down(struct gadget *g, int x, int y, int button)
{
	dlg_color(g->form, "Choose color", &wc);
	gadget_redraw(g);
}

int f_close(struct form *f)
{
	exit(0);
}

int main()
{
	struct gadget *g;
	struct form *f;
	
	if (win_attach())
		err(255, NULL);
	
	f = form_creat(FORM_TITLE | FORM_FRAME | FORM_ALLOW_CLOSE | FORM_ALLOW_MINIMIZE, 1,
			-1, -1, 160, 100, "color_test");
	form_on_close(f, f_close);
	
	g = gadget_creat(f, 5, 5, 150, 90);
	gadget_set_redraw_cb(g, g_redraw);
	gadget_set_ptr_down_cb(g, g_ptr_down);
	
	for (;;)
		win_wait();
}
