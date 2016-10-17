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
#include <wingui_msgbox.h>
#include <wingui_form.h>
#include <wingui.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <err.h>

static struct form *oform,
		   *iform;

static int f_close(struct form *f)
{
	exit(0);
}

static void adjust_position(void)
{
	int fw = wm_get(WM_FRAME);
	
	form_resize(oform, iform->win_rect.w + 40,
			   iform->win_rect.h + 40);
	
	form_move(oform, iform->win_rect.x - 20 - fw,
			 iform->win_rect.y - 20 - fw - oform->workspace_rect.y);
}

static void of_move(struct form *f, int x, int y)
{
	int fw = wm_get(WM_FRAME);
	
	form_move(iform, x + 20 + fw, y + 20 + fw + oform->workspace_rect.y);
}

static void if_move(struct form *f, int x, int y)
{
	adjust_position();
}

static void if_resize(struct form *f, int w, int h)
{
	adjust_position();
}

static void of_raise(struct form *f)
{
	form_raise(iform);
}

int main(int argc, char **argv)
{
	char msg[256];
	int flags;
	
	if (win_attach())
		err(255, NULL);
	
	if (argc != 2)
	{
		msgbox(NULL, "Form Viewer", "Wrong nr of args.");
		return 1;
	}
	iform = form_load(argv[1]);
	if (!iform)
	{
		sprintf(msg, "Cannot load \"%s\"", argv[1]);
		msgbox_perror(NULL, "Form Viewer", msg, errno);
		return 1;
	}
	
	win_idle();
	
	flags = FORM_FRAME | FORM_TITLE | FORM_ALLOW_CLOSE;
	oform = form_creat(flags, 1, 10, 10, 10, 0, "Form Viewer");
	adjust_position();
	
	form_raise(iform);
	
	form_on_close(oform, f_close);
	form_on_close(iform, f_close);
	
	form_on_move(oform, of_move);
	form_on_move(iform, if_move);
	
	form_on_raise(oform, of_raise);
	
	form_on_resize(iform, if_resize);
	
	if (oform->win_rect.x < 0 || oform->win_rect.y < 0)
		form_move(oform, 0, 0);
	
	form_show(iform);
	form_wait(oform);
	return 0;
}
