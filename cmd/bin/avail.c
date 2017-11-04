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
#include <sys/signal.h>
#include <fmthuman.h>
#include <unistd.h>
#include <systat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

#define MAIN_FORM	"/lib/forms/avail.frm"

struct gadget *core_value;
struct gadget *task_value;
struct gadget *fso_value;
struct gadget *file_value;
struct gadget *blk_value;

struct form *main_form;

static struct systat prev_st;

void sig_alrm(int nr)
{
	struct systat st;
	char buf[64];
	
	if (_systat(&st))
	{
		msgbox_perror(main_form, "Avail", "Could not get info", errno);
		exit(errno);
	}
	
	if (st.core_avail != prev_st.core_avail)
	{
		char a[64], m[64];
		
		strcpy(a, fmthumansz(st.core_avail, 1));
		strcpy(m, fmthumansz(st.core_max, 1));
		
		sprintf(buf, "%s (%s max)", a, m);
		label_set_text(core_value, buf);
	}
	if (st.task_avail != prev_st.task_avail)
	{
		sprintf(buf, "%i (%i max)", st.task_avail, st.task_max);
		label_set_text(task_value, buf);
	}
	if (st.file_avail != prev_st.file_avail)
	{
		sprintf(buf, "%i (%i max)", st.file_avail, st.file_max);
		label_set_text(file_value, buf);
	}
	if (st.fso_avail != prev_st.fso_avail)
	{
		sprintf(buf, "%i (%i max)", st.fso_avail, st.fso_max);
		label_set_text(fso_value, buf);
	}
	if (st.blk_avail != prev_st.blk_avail || st.blk_max != prev_st.blk_max)
	{
		sprintf(buf, "%i (%i max)", st.blk_avail, st.blk_max);
		label_set_text(blk_value, buf);
	}
	prev_st = st;
	
	win_signal(SIGALRM, sig_alrm);
	ualarm(100000, 0);
}

int main_form_close(struct form *f)
{
	exit(0);
}

int main(int argc, char **argv)
{
	struct win_rect wsr;
	int taskbar = 0;
	int x, y;
	
	if (argc == 2 && !strcmp(argv[1], "--taskbar"))
		taskbar = 1;
	if (getuid() != geteuid())
		clearenv();
	
	if (win_attach())
		err(255, NULL);
	
	main_form = form_load(MAIN_FORM);
	if (!main_form)
	{
		msgbox_perror(NULL, "Avail", "Cannot load main form", errno);
		return 1;
	}
	form_on_close(main_form, main_form_close);
	
	if (taskbar)
	{
		win_ws_getrect(&wsr);
		x = wsr.x + wsr.w - main_form->win_rect.w - 10;
		y = wsr.y + wsr.h - main_form->win_rect.h - 10;
		form_move(main_form, x, y);
		form_set_layer(main_form, WIN_LAYER_TASKBAR);
		form_raise(main_form);
	}
	
	core_value = gadget_find(main_form, "core");
	task_value = gadget_find(main_form, "procs");
	fso_value  = gadget_find(main_form, "fsos");
	file_value = gadget_find(main_form, "files");
	blk_value  = gadget_find(main_form, "bufs");
	
	memset(&prev_st, 255, sizeof prev_st);
	sig_alrm(SIGALRM);
	
	form_show(main_form);
	for (;;)
		win_wait();
}
