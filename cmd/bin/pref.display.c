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
#include <wingui_dlg.h>
#include <wingui.h>
#include <prefs/dmode.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <confdb.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <err.h>

#define MAIN_FORM	"/lib/forms/display.frm"

static struct form *main_form;

static struct gadget *res_label;
static struct gadget *res_sbar;
static struct gadget *hidpi_chkbox;

static struct gadget *depth_label;
static struct gadget *depth_sbar;

static int res_count;

static int reboot;

static struct res
{
	int width;
	int height;
} res[256];

static int depth_count;
static int depth[16];

static int odpi;

static void save_mode(int nr, int xres, int yres, int nclr, int hidpi)
{
	int wm_tab[WM_COUNT];
	struct dmode *dm;
	int dflags;
	char *ddir;
	FILE *f;
	int i;
	
	ddir = getenv("DESKTOP_DIR");
	if (!ddir)
		return;
	if (chdir(ddir))
		return;
	
	dm = dm_get();
	if (!dm)
		goto fail;
	
	dm->xres  = xres;
	dm->yres  = yres;
	dm->nclr  = nclr;
	dm->nr    = nr;
	dm->hidpi = hidpi;
	
	if (dm_save())
		goto fail;
	
	dflags = win_dispflags();
	if (dflags >= 0 && dflags & WIN_DF_BOOTFB)
	{
		f = fopen("/etc/bootfb.conf", "w");
		if (f)
		{
			int bpp = nclr > 256 ? 32 : 8;
			
			fprintf(f, "W%i\n", xres);
			fprintf(f, "H%i\n", yres);
			fprintf(f, "B%i\n", bpp);
			fclose(f);
		}
	}
	
	if (odpi != hidpi)
	{
		if (!getenv("OSDEMO"))
		{
			for (i = 0; i < WM_COUNT; i++)
				wm_tab[i] = wm_get(i);
			
			if (hidpi)
			{
				wm_tab[WM_DOUBLECLICK]	= 6;
				wm_tab[WM_SCROLLBAR]	= 24;
				wm_tab[WM_THIN_LINE]	= 2;
			}
			else
			{
				wm_tab[WM_DOUBLECLICK]	= 3;
				wm_tab[WM_SCROLLBAR]	= 12;
				wm_tab[WM_THIN_LINE]	= 1;
			}
			
			wm_tab[WM_FRAME] *= hidpi ? 2 : 1;
			wm_tab[WM_FRAME] /= odpi  ? 2 : 1;
			
			if (c_save("/user/metrics", &wm_tab, sizeof wm_tab))
				goto fail;
			
		}
		
		reboot = 1;
	}
	
	return;
fail:
	msgbox_perror(main_form, "Display Settings", "Cannot save configuration", errno);
	return;
}

static void xit(void)
{
	if (reboot)
		dlg_reboot(main_form, "Display Settings");
	exit(0);
}

static void ok_click()
{
	struct win_modeinfo m;
	int hidpi;
	int ri = 0;
	int di = 0;
	int i = 0;
	
	if (depth_sbar)
		di = hsbar_get_pos(depth_sbar);
	if (res_sbar)
		ri = hsbar_get_pos(res_sbar);
	
	if (ri < 0 || di < 0 || ri >= res_count || di >= depth_count)
		return;
	
	hidpi = chkbox_get_state(hidpi_chkbox);
	
	while (!win_modeinfo(i, &m))
	{
		if (m.width == res[ri].width && m.height == res[ri].height && m.ncolors == depth[di])
		{
			if (!win_setmode(i, 0))
			{
				save_mode(i, m.width, m.height, m.ncolors, hidpi);
				xit();
			}
			if (errno == EBUSY)
			{
				save_mode(i, m.width, m.height, m.ncolors, hidpi);
				reboot = 1;
				xit();
			}
			msgbox_perror(main_form, "Display Settings", "Cannot set display mode", errno);
			return;
		}
		i++;
	}
	
	msgbox(main_form, "Display Settings", "This display mode is not available.");
}

static void cn_click()
{
	exit(0);
}

static void update_res_label(void)
{
	static char str[32];
	int i;
	
	i = hsbar_get_pos(res_sbar);
	sprintf(str, "%i * %i", res[i].width, res[i].height);
	label_set_text(res_label, str);
}

static void res_sbar_move(struct gadget *g, int new_pos)
{
	update_res_label();
}

static void update_depth_label(void)
{
	static char str[32];
	int i;
	
	i = hsbar_get_pos(depth_sbar);
	sprintf(str, "%i", depth[i]);
	label_set_text(depth_label, str);
}

static void depth_sbar_move(struct gadget *g, int new_pos)
{
	update_depth_label();
}

static int depth_cmp(const void *a, const void *b)
{
	if (*(int *)a > *(int *)b)
		return 1;
	if (*(int *)a < *(int *)b)
		return -1;
	return 0;
}

static int res_cmp(const void *a, const void *b)
{
	const struct res *ra = a;
	const struct res *rb = b;
	
	if (ra->width > rb->width)
		return 1;
	if (ra->width < rb->width)
		return -1;
	if (ra->height > rb->height)
		return 1;
	if (ra->height < rb->height)
		return -1;
	return 0;
}

static void get_modes()
{
	struct win_modeinfo m;
	int i = 0;
	int n;
	
	while (!win_modeinfo(i++, &m))
	{
		for (n = 0; n < res_count; n++)
			if (m.width == res[n].width &&
			    m.height == res[n].height)
				break;
		
		if (n == res_count)
		{
			res[res_count].width = m.width;
			res[res_count].height = m.height;
			res_count++;
		}
		
		for (n = 0; n < depth_count; n++)
			if (m.ncolors == depth[n])
				break;
		
		if (n == depth_count)
			depth[depth_count++] = m.ncolors;
	}
	
	qsort(depth, depth_count, sizeof *depth, depth_cmp);
	qsort(res, res_count, sizeof *res, res_cmp);
}

static void create_form(void)
{
	main_form   = form_load(MAIN_FORM);
	if (!main_form)
	{
		msgbox_perror(NULL, "Display Settings", "Unable to load form", errno);
		exit(errno);
	}
	res_label    = gadget_find(main_form, "res_label");
	res_sbar     = gadget_find(main_form, "res_sbar");
	hidpi_chkbox = gadget_find(main_form, "hidpi_chkbox");
	depth_label  = gadget_find(main_form, "depth_label");
	depth_sbar   = gadget_find(main_form, "depth_sbar");
	
	button_on_click(gadget_find(main_form, "ok"), ok_click);
	button_on_click(gadget_find(main_form, "cn"), cn_click);
	
	hsbar_on_move(res_sbar, res_sbar_move);
	hsbar_on_move(depth_sbar, depth_sbar_move);
	
	hsbar_set_limit(res_sbar, res_count);
	hsbar_set_limit(depth_sbar, depth_count);
	
	if (res_count < 2)
	{
		label_set_text(gadget_find(main_form, "res_note"),
				"Current configuration does not\n"
				"allow to change the resolution.");
		gadget_hide(res_sbar);
	}
	
	if (depth_count < 2)
	{
		label_set_text(gadget_find(main_form, "depth_note"),
				"Current configuration does not\n"
				"allow to change the number of\n"
				"colors.");
		gadget_hide(depth_sbar);
	}
}

int main(int argc, char **argv)
{
	struct win_modeinfo m;
	int i;
	
	if (getuid() != geteuid())
		clearenv();
	
	if (win_attach())
		err(255, NULL);
	
	get_modes();
	create_form();
	
	win_getmode(&i);
	win_modeinfo(i, &m);
	for (i = 0; i < res_count; i++)
		if (m.width == res[i].width && m.height == res[i].height)
			hsbar_set_pos(res_sbar, i);
	
	for (i = 0; i < depth_count; i++)
		if (m.ncolors == depth[i])
			hsbar_set_pos(depth_sbar, i);
	
	odpi = win_get_dpi_class();
	
	chkbox_set_state(hidpi_chkbox, odpi);
	
	update_depth_label();
	update_res_label();
	
	form_wait(main_form);
	return 0;
}
