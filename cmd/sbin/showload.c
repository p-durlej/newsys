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
#include <wingui_color.h>
#include <wingui_form.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <fmthuman.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <systat.h>
#include <timer.h>
#include <err.h>

#define BAR_WIDTH	200
#define BAR_HEIGHT	20

#define FORM		"/lib/forms/showload.frm"

static struct form *	main_form;
static struct gadget *	csw_bar;
static struct gadget *	cpu_bar;
static struct gadget *	mem_bar;
static int		csw_max = 1;
static struct systat	st;
static struct timer *	tmr;
static int		red;

static void create_form(void)
{
	main_form = form_load(FORM);
	csw_bar	  = gadget_find(main_form, "csw");
	cpu_bar	  = gadget_find(main_form, "cpu");
	mem_bar	  = gadget_find(main_form, "mem");
}

static void sig_alrm(void *junk)
{
	if (_systat(&st))
	{
		msgbox_perror(main_form, "System Load", "systat failed", errno);
		exit(errno);
	}
	if (st.sw_freq > csw_max)
	{
		csw_max = st.sw_freq;
		red = 1;
	}
	else
		red = 0;
	
	bargraph_set_labels(cpu_bar, "", NULL);
	bargraph_set_limit(csw_bar, csw_max);
	bargraph_set_value(csw_bar, st.sw_freq);
	
	bargraph_set_labels(cpu_bar, "", "100%");
	bargraph_set_limit(cpu_bar, st.cpu_max);
	bargraph_set_value(cpu_bar, st.cpu);
	
	bargraph_set_labels(mem_bar, "", fmthumansz(st.core_max, 0));
	bargraph_set_limit(mem_bar, st.core_max);
	bargraph_set_value(mem_bar, st.core_max - st.core_avail);
	
	win_signal(SIGALRM, sig_alrm);
	alarm(1);
}

int main()
{
	if (win_attach())
		err(255, NULL);
	
	create_form();
	
	tmr = tmr_creat(
		&(struct timeval){ 0, 0 },
		&(struct timeval){ 1, 0 },
		sig_alrm, NULL, 1);
	
	form_wait(main_form);
	return 0;
}
