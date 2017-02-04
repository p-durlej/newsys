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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>

struct form *main_form;

int main_close(struct form *f)
{
	exit(0);
}

void sync_click(struct gadget *g, int x, int y)
{
	char msg[128];
	
	if (sync())
	{
		sprintf(msg, "Cannot sync:\n\n%m");
		msgbox(main_form, "Sync", msg);
	}
}

int main(int argc, char **argv)
{
	if (win_attach())
		err(255, NULL);
	
	main_form = form_creat(FORM_FRAME | FORM_TITLE | FORM_ALLOW_CLOSE | FORM_ALLOW_MINIMIZE, 1, -1, -1, 100, 56, "Sync");
	form_on_close(main_form, main_close);
	button_creat(main_form, 20, 20, 60, 16, "Sync", sync_click);
	
	for (;;)
		win_wait();
}
