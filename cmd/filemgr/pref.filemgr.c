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

#include <prefs/filemgr.h>
#include <wingui_msgbox.h>
#include <wingui_form.h>
#include <wingui_dlg.h>
#include <confdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

#include "filemgr.h"

#define MAIN_FORM	"/lib/forms/pref.filemgr.frm"

static struct gadget *chk1;
static struct gadget *chk2;
static struct gadget *chk3;
static struct gadget *chk4;
static struct gadget *chk5;
static struct form *f;

struct pref_filemgr *config;

static int on_close(struct form *f)
{
	exit(0);
}

static void ok_click()
{
	char msg[256];
	int sdc;
	
	sdc = config->double_click;
	
	config->show_dotfiles = chkbox_get_state(chk1);
	config->show_path     = chkbox_get_state(chk2);
	config->large_icons   = chkbox_get_state(chk3);
	config->win_desk      = chkbox_get_state(chk4);
	config->double_click  = chkbox_get_state(chk5);
	
	if (pref_filemgr_save())
	{
		msgbox_perror(f, "File Manager Prefs", "Cannot save configuration", errno);
		return;
	}
	
	if (sdc != config->double_click)
		dlg_newsess(f, NULL);
	exit(0);
}

int main(int argc, char **argv)
{
	if (win_attach())
		err(255, NULL);
	
	config = pref_filemgr_get();
	
	f    = form_load(MAIN_FORM);
	chk1 = gadget_find(f, "dots");
	chk2 = gadget_find(f, "pathname");
	chk3 = gadget_find(f, "large");
	chk4 = gadget_find(f, "windesk");
	chk5 = gadget_find(f, "doubleclick");
	
	form_on_close(f, on_close);
	
	chkbox_set_state(chk1, config->show_dotfiles);
	chkbox_set_state(chk2, config->show_path);
	chkbox_set_state(chk3, config->large_icons);
	chkbox_set_state(chk4, config->win_desk);
	chkbox_set_state(chk5, config->double_click);
	
	while (form_wait(f) != 2)
		ok_click();
	return 0;
}
