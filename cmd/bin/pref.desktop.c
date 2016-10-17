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

#include <priv/wingui_form_cfg.h>
#include <config/defaults.h>
#include <wingui_metrics.h>
#include <wingui_msgbox.h>
#include <wingui_form.h>
#include <wingui_dlg.h>
#include <limits.h>
#include <confdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <priv/filemgr.h>
#include <err.h>

#define MAIN_FORM	"/lib/forms/desktop.frm"

static struct gadget *dc_d;
static struct gadget *fr_w;
static struct gadget *sb_w;
static struct gadget *bd_l;
static struct gadget *chk1;
static struct gadget *chk2;
static struct gadget *zoom;
static struct gadget *larg;
static struct form *form;

static struct form_cfg old_config, config =
{
	.display_moving = DEFAULT_FORM_DISPLAY_MOVING,
};

static int wm_tab[WM_COUNT];

static int su;

static struct back_conf back_conf =
{
	.pathname = DEFAULT_BACKGROUND_PATHNAME,
	.tile	  = DEFAULT_BACKGROUND_TILE
};

static int on_close(struct form *f)
{
	exit(0);
}

static void ok_click()
{
	char *fls;
	FILE *f;
	
	config.display_moving = chkbox_get_state(chk1);
	config.smart_zoom     = chkbox_get_state(zoom);
	if (su)
		config.large_fonts = chkbox_get_state(larg);
	back_conf.tile	      = chkbox_get_state(chk2);
	
	wm_tab[WM_DOUBLECLICK] = atoi(dc_d->text);
	wm_tab[WM_FRAME]       = atoi(fr_w->text);
	wm_tab[WM_SCROLLBAR]   = atoi(sb_w->text);
	
	if (c_save("/user/metrics",	&wm_tab,	sizeof wm_tab))		goto error;
	if (c_save("/user/form",	&config,	sizeof config))		goto error;
	if (c_save("/user/backdrop",	&back_conf,	sizeof back_conf))	goto error;
	
	win_update();
	
	if (config.large_fonts)
		fls = "/lib/fonts/mono-large\n"
		      "/lib/fonts/system-large\n"
		      "/lib/fonts/mono-large\n"
		      "/lib/fonts/system-large\n";
	else
		fls = "/lib/fonts/mono\n"
		      "/lib/fonts/system\n"
		      "/lib/fonts/mono-large\n"
		      "/lib/fonts/system-large\n";
	
	if (su && old_config.large_fonts != config.large_fonts)
	{
		f = fopen("/etc/sysfonts-new", "w");
		if (!f)
			goto error;
		if (fputs(fls, f) == EOF)
			goto error;
		if (fclose(f))
			goto error;
		if (rename("/etc/sysfonts-new", "/etc/sysfonts"))
			goto error;
		msgbox(form, "Desktop Prefs", "Reboot the system for the font size\n"
					      "change to take effect.");
	}
	exit(0);
	
error:
	msgbox_perror(form, "Desktop Prefs", "Cannot save configuration", errno);
}

static void ubdlabel(void)
{
	char *p;
	
	p = strrchr(back_conf.pathname, '/');
	if (p == NULL)
		p = back_conf.pathname;
	else
		p++;
	
	label_set_text(bd_l, p);
}

static void bd_b_click()
{
	char pathname[PATH_MAX];
	
	strcpy(pathname, back_conf.pathname);
	if (!*pathname)
		strcpy(pathname, "/lib/bg");
	
	if (dlg_open(form, "Backdrop Picture", pathname, sizeof pathname))
	{
		strcpy(back_conf.pathname, pathname);
		ubdlabel();
	}
}

static void bd_r_click()
{
	*back_conf.pathname = 0;
	ubdlabel();
}

int main(int argc, char **argv)
{
	static char buf[64];
	int r;
	int i;
	
	su = !geteuid();
	
	if (win_attach())
		err(255, NULL);
	
	c_load("/user/form",		&config,	sizeof config);
	c_load("/user/backdrop",	&back_conf,	sizeof back_conf);
	
	old_config = config;
	
	for (i = 0; i < WM_COUNT; i++)
		wm_tab[i] = wm_get(i);
	
	form = form_load(MAIN_FORM);
	chk1 = gadget_find(form, "moving");
	chk2 = gadget_find(form, "tile");
	bd_l = gadget_find(form, "bd_path");
	dc_d = gadget_find(form, "dc_dist");
	zoom = gadget_find(form, "zoom");
	larg = gadget_find(form, "large");
	fr_w = gadget_find(form, "frame_width");
	sb_w = gadget_find(form, "sb_width");
	
	form_on_close(form, on_close);
	
	chkbox_set_state(chk1, config.display_moving);
	chkbox_set_state(zoom, config.smart_zoom);
	if (su)
		chkbox_set_state(larg, config.large_fonts);
	else
		gadget_remove(larg);
	
	chkbox_set_state(chk2, back_conf.tile);
	ubdlabel();
	
	sprintf(buf, "%i", wm_tab[WM_DOUBLECLICK]);
	input_set_text(dc_d, buf);
	
	sprintf(buf, "%i", wm_tab[WM_FRAME]);
	input_set_text(fr_w, buf);
	
	sprintf(buf, "%i", wm_tab[WM_SCROLLBAR]);
	input_set_text(sb_w, buf);
	
	while (r = form_wait(form), r != -1)
		switch (r)
		{
		case 1:
			bd_b_click();
			break;
		case 2:
			bd_r_click();
			break;
		case 3:
			ok_click();
			break;
		}
	return 0;
}
