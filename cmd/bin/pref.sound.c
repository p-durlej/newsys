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

#include <dev/speaker.h>
#include <prefs/wbeep.h>
#include <wingui_msgbox.h>
#include <wingui_form.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

static struct form *main_form;

static struct gadget *sbar_freq;
static struct gadget *sbar_dur;
static struct gadget *l_freq;
static struct gadget *l_dur;
static struct gadget *chk_ena;

static struct pref_wbeep *pref;

static void sbar_move(struct gadget *g, int pos);

static void get_from_ui(int *freq, int *dur, int *ena)
{
	*freq = hsbar_get_pos(sbar_freq) + 20; /* XXX should use logarithmic scale */
	*dur  = hsbar_get_pos(sbar_dur)  + 50;
	*ena  = chkbox_get_state(chk_ena);
}

static void set_ui(void)
{
	hsbar_set_pos(sbar_freq, pref->freq - 20); /* XXX should use logarithmic scale */
	hsbar_set_pos(sbar_dur,  pref->dur  - 50);
	chkbox_set_state(chk_ena, pref->ena);
	
	sbar_move(NULL, -1);
}

static void sbar_move(struct gadget *g, int pos)
{
	char buf[64];
	int f, d, e;
	
	get_from_ui(&f, &d, &e);
	
	sprintf(buf, "%i Hz", f);
	label_set_text(l_freq, buf);
	
	sprintf(buf, "%i ms", d);
	label_set_text(l_dur, buf);
}

static int save(void)
{
	get_from_ui(&pref->freq, &pref->dur, &pref->ena);
	
	if (pref_wbeep_save())
	{
		msgbox_perror(main_form, "Warning Beep", "Cannot save settings", errno);
		return -1;
	}
	
	win_update();
	return 0;
}

static void test(void)
{
	int f, d, e;
	
	get_from_ui(&f, &d, &e);
	if (e)
		spk_tone(-1, f, d);
}

int main(int argc, char **argv)
{
	const char *p;
	
	if (win_attach())
		err(1, "win_attach");
	
	p = getenv("SPEAKER");
	if (!p || access(p, R_OK | W_OK))
	{
		msgbox(NULL, "Warning Beep", "The speaker device is either not present or not accessible.");
		return 1;
	}
	
	pref = pref_wbeep_get();
	if (!pref)
	{
		msgbox_perror(NULL, "Warning Beep", "Cannot read settings", errno);
		return 1;
	}
	
	main_form = form_load("/lib/forms/pref.sound.frm");
	
	sbar_freq = gadget_find(main_form, "frequency");
	sbar_dur  = gadget_find(main_form, "duration");
	l_freq	  = gadget_find(main_form, "l_frequency");
	l_dur	  = gadget_find(main_form, "l_duration");
	chk_ena   = gadget_find(main_form, "enable");
	
	hsbar_set_limit(sbar_freq, 9981);
	hsbar_set_limit(sbar_dur,  951);
	
	hsbar_on_move(sbar_freq, sbar_move);
	hsbar_on_move(sbar_dur, sbar_move);
	
	set_ui();
	for (;;)
	{
		switch (form_wait(main_form))
		{
		case 1:
			if (!save())
				return 0;
			break;
		case 2:
			return 0;
		case 3:
			test();
			break;
		default:
			return 0;
		}
	}
	
	return 0;
}
