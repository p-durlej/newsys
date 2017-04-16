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

#include <wingui_msgbox.h>
#include <wingui_form.h>
#include <wingui_menu.h>
#include <stdio.h>
#include <err.h>

static struct gadget *bg1, *bg2, *bg3;
static struct gadget *g;
static struct form *f;

static void hsbar_move(struct gadget *g, int pos)
{
	warnx("pos = %i", pos);
	
	bargraph_set_value(bg1, pos);
	bargraph_set_value(bg2, pos);
	bargraph_set_value(bg3, pos);
}

static void hide_click(struct gadget *g, int x, int y)
{
	form_hide(f);
}

int main()
{
	if (win_attach())
		err(255, NULL);
	
	f = form_creat(FORM_APPFLAGS, 1, -1, -1, 320, 170, "gui_test");
	if (!f)
	{
		perror("form_creat");
		return 127;
	}
	colorsel_creat(f, 10, 10, 20, 20);
	button_creat(f,  40, 10, 60, 20, "Hide form", hide_click);
	button_creat(f, 250, 10, 60, 20, "A button", NULL);
	
	bg1 = bargraph_creat(f, 10,  80, 300, 20);
	bg2 = bargraph_creat(f, 10, 110, 300, 20);
	bg3 = bargraph_creat(f, 10, 140, 300, 20);
	
	bargraph_set_labels(bg2, "min", "max");
	bargraph_set_labels(bg3, "", "");
	
	g = hsbar_creat(f, 10, 40, 300, 20);
	hsbar_on_move(g, hsbar_move);
	hsbar_set_limit(g, 101);
	
	form_show(f);
	form_wait(f);
	return 0;
}
