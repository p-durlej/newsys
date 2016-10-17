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

#include <wingui_msgbox.h>
#include <wingui_form.h>
#include <confdb.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <err.h>

#include <priv/osk.h>

#define MAIN_FORM	"/lib/forms/pref.osk.frm"

static struct gadget *chk1;
static struct form *f;

static struct osk_conf config;

static int on_close(struct form *f)
{
	exit(0);
}

static void ok_click()
{
	config.floating = chkbox_get_state(chk1);
	
	if (c_save("osk", &config, sizeof config))
		msgbox_perror(f, "OSK Prefs", "Cannot save configuration", errno);
	else
		exit(0);
}

int main(int argc, char **argv)
{
	if (win_attach())
		err(255, NULL);
	
	if (c_load("osk", &config, sizeof config))
		config.floating = 1;
	
	f    = form_load(MAIN_FORM);
	chk1 = gadget_find(f, "float");
	
	form_on_close(f, on_close);
	
	chkbox_set_state(chk1, config.floating);
	
	while (form_wait(f) != 2)
		ok_click();
	return 0;
}
