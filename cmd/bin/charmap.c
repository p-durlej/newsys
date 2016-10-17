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

#include <priv/copyright.h>
#include <wingui_msgbox.h>
#include <wingui_form.h>
#include <wingui_menu.h>
#include <wingui_dlg.h>
#include <wingui_buf.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <err.h>

#define COLUMNS	16

static struct form *f;

static int f_close(struct form *f)
{
	exit(0);
}

static void b_click(struct gadget *g, int x, int y)
{
	if (wbu_store(g->text, 1))
	{
		msgbox_perror(f, "Character map", "Cannot copy", errno);
		return;
	}
	msgbox(f, "Character map", "The character you selected has been copied to the clipboard.");
}

static void about_click(struct menu_item *mi)
{
	dlg_about7(f, NULL, "Character Map v1", SYS_PRODUCT, SYS_AUTHOR, SYS_CONTACT, "charmap.pnm");
}

int main(int argc, char **argv)
{
	struct menu *mm, *m;
	char txt[2];
	int btn_w;
	int btn_h;
	int x, y;
	int fh;
	int i;
	
	if (win_attach())
		err(255, NULL);
	
	win_text_size(WIN_FONT_DEFAULT, &btn_w, &btn_h, "X");
	btn_w += 12;
	btn_h += 9;
	
	mm = menu_creat();
	
	m = menu_creat();
	menu_newitem(m, "About ...", about_click);
	menu_submenu(mm, "Options", m);
	
	for (fh = 0, i = 0; i < 255; i++)
	{
		if (!isprint(i) || isspace(i))
			continue;
		
		fh++;
	}
	fh += COLUMNS - 1;
	fh /= COLUMNS;
	fh *= btn_h + 2;
	fh += 2;
	
	f = form_creat(FORM_TITLE | FORM_FRAME | FORM_ALLOW_CLOSE | FORM_ALLOW_MINIMIZE, 1,
			-1, -1, COLUMNS * (btn_w + 2) + 2, fh, "Character map");
	form_on_close(f, f_close);
	form_set_menu(f, mm);
	
	x = 2;
	y = 2;
	for (i = 0; i < 255; i++)
	{
		if (!isprint(i) || isspace(i))
			continue;
		
		txt[0] = i;
		txt[1] = 0;
		button_creat(f, x, y, btn_w, btn_h, txt, b_click);
		
		x += btn_w + 2;
		if (x >= f->workspace_rect.w)
		{
			y += btn_h + 2;
			x  = 2;
		}
	}
	
	for (;;)
		win_wait();
}
