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

struct gadget *device_edbox;
struct gadget *prefix_edbox;
struct gadget *fstype_edbox;

struct gadget *ro_chkbox;
struct gadget *na_chkbox;

int _mount(char *prefix, char *dev, char *type, int ro);

void mount_click(struct gadget *g, int x, int y)
{
	char *p;
	char *d;
	char *t;
	int ro;
	int na;
	
	edbox_get_text_ptr(prefix_edbox, &p);
	edbox_get_text_ptr(device_edbox, &d);
	edbox_get_text_ptr(fstype_edbox, &t);
	ro = chkbox_get_state(ro_chkbox);
	na = chkbox_get_state(na_chkbox);
	
	if (_mount(p, d, t, ro | (na << 1)))
	{
		char msg[128];
		
		sprintf(msg, "Cannot mount \"%s\":\n\n%m", d);
		msgbox(main_form, "Mount File System", msg);
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
	
	main_form = form_creat(FORM_FRAME|FORM_TITLE, 1, -1, -1, 200, 113, "Mount File System");
	
	label_creat(main_form, 2, 2, "Device Name:");
	device_edbox = edbox_creat(main_form, 2, 15, 140, 11);
	
	label_creat(main_form, 2, 28, "File Name Prefix:");
	prefix_edbox = edbox_creat(main_form, 2, 41, 140, 11);
	
	label_creat(main_form, 2, 54, "File System Type:");
	fstype_edbox = edbox_creat(main_form, 2, 67, 140, 11);
	
	ro_chkbox = chkbox_creat(main_form, 2, 87, "Mount read-only");
	na_chkbox = chkbox_creat(main_form, 2, 100, "Do not update access time");
	
	button_creat(main_form, 144, 2, 54, 16, "OK", mount_click);
	button_creat(main_form, 144, 20, 54, 16, "Cancel", cancel_click);
	gadget_focus(device_edbox);
	
	edbox_set_text(device_edbox, "fd0");
	edbox_set_text(prefix_edbox, "/mnt");
	edbox_set_text(fstype_edbox, "boot");
	
	for (;;)
		win_wait();
}
