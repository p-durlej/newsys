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
#include <string.h>
#include <errno.h>

static struct gadget *label;
static struct form *f;
static int cursor;

static void cursor_click()
{
	if (cursor)
	{
		cursor = 0;
		return;
	}
	
	form_unlock(f);
	cursor = 1;
	while (cursor)
	{
		win_setptr(f->wd, WIN_PTR_BUSY);
		win_setptr(f->wd, WIN_PTR_DRAG);
		win_idle();
	}
	win_setptr(f->wd, WIN_PTR_ARROW);
	form_lock(f);
}

static void hide_click()
{
	if (win_paint())
		label_set_text(label, strerror(errno));
	else
		label_set_text(label, "OK");
}

static void show_click()
{
	if (win_end_paint())
		label_set_text(label, strerror(errno));
	else
		label_set_text(label, "OK");
}

int main()
{
	win_attach();
	
	f = form_creat(FORM_FXS_APPFLAGS, 1, -1, -1, 320, 64, "hideptr test");
	
	label = label_creat(f, 76, 8, "(status)");
	
	button_creat(f,  8,  8, 60, 20, "Hide",	  hide_click);
	button_creat(f,  8, 36, 60, 20, "Show",	  show_click);
	button_creat(f, 76, 36, 60, 20, "Cursor", cursor_click);
	
	form_wait(f);
	return 0;
}
