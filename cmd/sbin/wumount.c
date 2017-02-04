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
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>

struct form *main_form;

struct gadget *prefix_edbox;

int _umount(char *prefix);

void mount_click(struct gadget *g, int x, int y)
{
	char p[PATH_MAX];
	
	edbox_get_text(prefix_edbox, p, sizeof p);
	
	if (_umount(p))
	{
		char msg[128];
		
		sprintf(msg, "Cannot unmount \"%s\":\n\n%m", p);
		msgbox(main_form, "Unmount File System", msg);
	}
}

void cancel_click(struct gadget *g, int x, int y)
{
	exit(0);
}

int main(int argc, char **argv)
{
	if (win_attach())
		err(255, NULL);
	
	main_form = form_creat(FORM_FRAME|FORM_TITLE, 1, -1, -1, 200, 100, "Unmount File System");
	
	label_creat(main_form, 2, 2, "File Name Prefix:");
	prefix_edbox = edbox_creat(main_form, 2, 15, 140, 11);
	
	button_creat(main_form, 144, 2, 54, 16, "OK", mount_click);
	button_creat(main_form, 144, 20, 54, 16, "Cancel", cancel_click);
	gadget_focus(prefix_edbox);
	
	edbox_set_text(prefix_edbox, "/mnt");
	
	for (;;)
		win_wait();
}
