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

#include <config/defaults.h>
#include <wingui_color.h>
#include <wingui.h>
#include <confdb.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <paths.h>

typedef struct win_rgba wc_rgba_t[WC_COUNT];
typedef win_color wc_tab_t[WC_COUNT];

static wc_rgba_t	wc_rgba;
static wc_tab_t		wc_tab;

int wc_loaded;

static struct
{
	char *name;
	int index;
} color_names[] =
{
	{ "desktop_bg",			WC_DESKTOP		},
	{ "desktop",			WC_DESKTOP		},
	
	{ "win_bg",			WC_WIN_BG		},
	{ "win_fg",			WC_WIN_FG		},
	{ "bg",				WC_BG			},
	{ "fg",				WC_FG			},
	{ "sel_bg",			WC_SEL_BG		},
	{ "sel_fg",			WC_SEL_FG		},
	{ "cur_bg",			WC_CUR_BG		},
	{ "cur_fg",			WC_CUR_FG		},
	{ "highlight1",			WC_HIGHLIGHT1		},
	{ "highlight2",			WC_HIGHLIGHT2		},
	{ "shadow1",			WC_SHADOW1		},
	{ "shadow2",			WC_SHADOW2		},
	{ "title_bg",			WC_TITLE_BG		},
	{ "title_fg",			WC_TITLE_FG		},
	{ "cursor",			WC_CURSOR		},
	{ "sbar",			WC_SBAR			},
	{ "desktop_fg",			WC_DESKTOP_FG		},
	{ "title_sh1",			WC_TITLE_SH1		},
	{ "title_sh2",			WC_TITLE_SH2		},
	{ "title_hi1",			WC_TITLE_HI1		},
	{ "title_hi2",			WC_TITLE_HI2		},
	{ "nfsel_bg",			WC_NFSEL_BG		},
	{ "nfsel_fg",			WC_NFSEL_FG		},
	{ "inact_title_bg",		WC_INACT_TITLE_BG	},
	{ "inact_title_fg",		WC_INACT_TITLE_FG	},
	{ "inact_title_sh1",		WC_INACT_TITLE_SH1	},
	{ "inact_title_sh2",		WC_INACT_TITLE_SH2	},
	{ "inact_title_hi1",		WC_INACT_TITLE_HI1	},
	{ "inact_title_hi2",		WC_INACT_TITLE_HI2	},
	{ "title_btn_bg",		WC_TITLE_BTN_BG		},
	{ "title_btn_fg",		WC_TITLE_BTN_FG		},
	{ "title_btn_sh1",		WC_TITLE_BTN_SH1	},
	{ "title_btn_sh2",		WC_TITLE_BTN_SH2	},
	{ "title_btn_hi1",		WC_TITLE_BTN_HI1	},
	{ "title_btn_hi2",		WC_TITLE_BTN_HI2	},
	{ "inact_title_btn_bg",		WC_INACT_TITLE_BTN_BG	},
	{ "inact_title_btn_fg",		WC_INACT_TITLE_BTN_FG	},
	{ "inact_title_btn_sh1",	WC_INACT_TITLE_BTN_SH1	},
	{ "inact_title_btn_sh2",	WC_INACT_TITLE_BTN_SH2	},
	{ "inact_title_btn_hi1",	WC_INACT_TITLE_BTN_HI1	},
	{ "inact_title_btn_hi2",	WC_INACT_TITLE_BTN_HI2	},
	{ "menu_underline",		WC_MENU_UNDERLINE	},
	{ "frame_bg",			WC_FRAME_BG		},
	{ "frame_sh1",			WC_FRAME_SH1		},
	{ "frame_sh2",			WC_FRAME_SH2		},
	{ "frame_hi1",			WC_FRAME_HI1		},
	{ "frame_hi2",			WC_FRAME_HI2		},
	{ "inact_frame_bg",		WC_INACT_FRAME_BG	},
	{ "inact_frame_sh1",		WC_INACT_FRAME_SH1	},
	{ "inact_frame_sh2",		WC_INACT_FRAME_SH2	},
	{ "inact_frame_hi1",		WC_INACT_FRAME_HI1	},
	{ "inact_frame_hi2",		WC_INACT_FRAME_HI2	},
	{ "button_bg",			WC_BUTTON_BG		},
	{ "button_fg",			WC_BUTTON_FG		},
	{ "button_sh1",			WC_BUTTON_SH1		},
	{ "button_sh2",			WC_BUTTON_SH2		},
	{ "button_hi1",			WC_BUTTON_HI1		},
	{ "button_hi2",			WC_BUTTON_HI2		},
	{ "tty_cursor",			WC_TTY_CURSOR		},
	{ "tty_bg",			WC_TTY_BG		},
	{ "tty_fg",			WC_TTY_FG		},
};

static const struct win_rgba wc_default_tab[] =
{
	[WC_DESKTOP_FG]		 = { 255, 255, 255, 255 },
	[WC_DESKTOP]		 = {   0,   0,  96, 255 },
	
	[WC_WIN_BG]		 = { 203, 200, 192, 224 },
	[WC_WIN_FG]		 = {   0,   0,   0, 255 },
	
	[WC_BG]			 = { 255, 255, 255, 255 },
	[WC_FG]			 = {   0,   0,   0, 255 },
	
	[WC_SEL_BG]		 = {   0, 128, 128, 255 },
	[WC_SEL_FG]		 = { 255, 255, 255, 255 },
	
	[WC_CUR_BG]		 = { 255, 248, 239, 255 },
	[WC_CUR_FG]		 = {   0,   0,   0, 255 },
	
	[WC_HIGHLIGHT1]		 = { 255, 255, 255, 255 },
	[WC_HIGHLIGHT2]		 = { 224, 224, 224, 255 },
	[WC_SHADOW1]		 = {  64,  64,  64, 255 },
	[WC_SHADOW2]		 = { 128, 128, 128, 255 },
	
	[WC_TITLE_BG]		 = {   0, 128, 128, 192 },
	[WC_TITLE_FG]		 = {   0,   0,   0, 255 },
	[WC_TITLE_HI1]		 = {   0, 160, 160, 192 },
	[WC_TITLE_HI2]		 = {   0, 144, 144, 192 },
	[WC_TITLE_SH1]		 = {   0,  96,  96, 192 },
	[WC_TITLE_SH2]		 = {   0, 112, 112, 192 },
	
	[WC_CURSOR]		 = {   0,   0, 255, 255 },
	
	[WC_SBAR]		 = { 224, 216, 208, 255 },
	
	[WC_NFSEL_FG]		 = {   0,   0,   0, 255 },
	[WC_NFSEL_BG]		 = { 203, 200, 192, 255 },
	
	[WC_INACT_TITLE_BG]	 = { 150, 150, 150, 192 },
	[WC_INACT_TITLE_FG]	 = {   0,   0,   0, 255 },
	[WC_INACT_TITLE_HI1]	 = { 160, 160, 160, 192 },
	[WC_INACT_TITLE_HI2]	 = { 155, 155, 155, 192 },
	[WC_INACT_TITLE_SH1]	 = {  96,  96,  96, 192 },
	[WC_INACT_TITLE_SH2]	 = { 123, 123, 123, 192 },
	
	[WC_TITLE_BTN_BG]	 = { 203, 200, 192, 255 },
	[WC_TITLE_BTN_FG]	 = {   0,   0,   0, 255 },
	[WC_TITLE_BTN_HI1]	 = { 255, 255, 255, 255 },
	[WC_TITLE_BTN_HI2]	 = { 229, 229, 229, 255 },
	[WC_TITLE_BTN_SH1]	 = {  64,  64,  64, 255 },
	[WC_TITLE_BTN_SH2]	 = { 134, 134, 134, 255 },
	
	[WC_INACT_TITLE_BTN_BG]	 = { 203, 200, 192, 255 },
	[WC_INACT_TITLE_BTN_FG]	 = {   0,   0,   0, 255 },
	[WC_INACT_TITLE_BTN_HI1] = { 255, 255, 255, 255 },
	[WC_INACT_TITLE_BTN_HI2] = { 229, 229, 229, 255 },
	[WC_INACT_TITLE_BTN_SH1] = { 64,   64,  64, 255 },
	[WC_INACT_TITLE_BTN_SH2] = { 134, 134, 134, 255 },
	
	[WC_MENU_UNDERLINE]	 = { 203, 200, 192, 224 },
	
	[WC_FRAME_BG]		 = {   0, 128, 128, 192 },
	[WC_FRAME_HI1]		 = {   0, 160, 160, 192 },
	[WC_FRAME_HI2]		 = {   0, 144, 144, 192 },
	[WC_FRAME_SH1]		 = {   0,  96,  96, 192 },
	[WC_FRAME_SH2]		 = {   0, 112, 112, 192 },
	
	[WC_INACT_FRAME_BG]	 = { 150, 150, 150, 192 },
	[WC_INACT_FRAME_HI1]	 = { 160, 160, 160, 192 },
	[WC_INACT_FRAME_HI2]	 = { 155, 155, 155, 192 },
	[WC_INACT_FRAME_SH1]	 = {  96,  96,  96, 192 },
	[WC_INACT_FRAME_SH2]	 = { 123, 123, 123, 192 },
	
	[WC_BUTTON_BG]		 = { 203, 200, 192, 224 },
	[WC_BUTTON_FG]		 = {   0,   0,   0, 255 },
	[WC_BUTTON_HI1]		 = { 255, 255, 255, 255 },
	[WC_BUTTON_HI2]		 = { 224, 224, 224, 255 },
	[WC_BUTTON_SH1]		 = {  64,  64,  64, 255 },
	[WC_BUTTON_SH2]		 = { 128, 128, 128, 255 },
	
	[WC_TTY_CURSOR]		 = { 255, 255, 255, 255 },
	[WC_TTY_BG]		 = {   0,   0,   0, 255 },
	[WC_TTY_FG]		 = { 192, 192, 192, 255 },
};

static const struct win_rgba wc_lowcolor_tab[] =
{
	[WC_DESKTOP_FG]			= { 255, 255, 255, 255 },
	[WC_DESKTOP]			= {   0, 128, 128, 255 },
	[WC_WIN_BG]			= { 191, 191, 191, 255 },
	[WC_WIN_FG]			= {   0,   0,   0, 255 },
	[WC_BG]				= { 255, 255, 255, 255 },
	[WC_FG]				= {   0,   0,   0, 255 },
	[WC_SEL_BG]			= { 128,   0,   0, 255 },
	[WC_SEL_FG]			= { 255, 255, 255, 255 },
	[WC_CUR_BG]			= { 255, 255, 255, 255 },
	[WC_CUR_FG]			= {   0,   0,   0, 255 },
	[WC_HIGHLIGHT1]			= { 255, 255, 255, 255 },
	[WC_HIGHLIGHT2]			= { 128, 128, 128, 255 },
	[WC_SHADOW1]			= {   0,   0,   0, 255 },
	[WC_SHADOW2]			= { 128, 128, 128, 255 },
	[WC_TITLE_BG]			= { 128,   0,   0, 255 },
	[WC_TITLE_FG]			= { 255, 255, 255, 255 },
	[WC_TITLE_HI1]			= { 255, 255, 255, 255 },
	[WC_TITLE_HI2]			= { 128,   0,   0, 255 },
	[WC_TITLE_SH1]			= {   0,   0,   0, 255 },
	[WC_TITLE_SH2]			= {  64,  64,  64, 255 },
	[WC_CURSOR]			= {   0,   0, 255, 255 },
	[WC_SBAR]			= { 255, 255, 255, 255 },
	[WC_NFSEL_BG]			= { 191, 191, 191, 255 },
	[WC_NFSEL_FG]			= {   0,   0,   0, 255 },
	[WC_INACT_TITLE_BG]		= { 128, 128, 128, 255 },
	[WC_INACT_TITLE_FG]		= {   0,   0,   0, 255 },
	[WC_INACT_TITLE_HI1]		= { 255, 255, 255, 255 },
	[WC_INACT_TITLE_HI2]		= { 128, 128, 128, 255 },
	[WC_INACT_TITLE_SH1]		= {   0,   0,   0, 255 },
	[WC_INACT_TITLE_SH2]		= { 128, 128, 128, 255 },
	[WC_TITLE_BTN_BG]		= { 128, 128, 128, 255 },
	[WC_TITLE_BTN_FG]		= {   0,   0,   0, 255 },
	[WC_TITLE_BTN_HI1]		= { 255, 255, 255, 255 },
	[WC_TITLE_BTN_HI2]		= { 128, 128, 128, 255 },
	[WC_TITLE_BTN_SH1]		= {   0,   0,   0, 255 },
	[WC_TITLE_BTN_SH2]		= { 128, 128, 128, 255 },
	[WC_INACT_TITLE_BTN_BG]		= { 128, 128, 128, 255 },
	[WC_INACT_TITLE_BTN_FG]		= {   0,   0,   0, 255 },
	[WC_INACT_TITLE_BTN_HI1]	= { 255, 255, 255, 255 },
	[WC_INACT_TITLE_BTN_HI2]	= { 128, 128, 128, 255 },
	[WC_INACT_TITLE_BTN_SH1]	= {   0,   0,   0, 255 },
	[WC_INACT_TITLE_BTN_SH2]	= { 128, 128, 128, 255 },
	[WC_MENU_UNDERLINE]		= {   0,   0,   0, 255 },
	[WC_FRAME_BG]			= { 128, 128, 128, 255 },
	[WC_FRAME_HI1]			= { 255, 255, 255, 255 },
	[WC_FRAME_HI2]			= { 128, 128, 128, 255 },
	[WC_FRAME_SH1]			= {   0,   0,   0, 255 },
	[WC_FRAME_SH2]			= { 128, 128, 128, 255 },
	[WC_INACT_FRAME_BG]		= { 128, 128, 128, 255 },
	[WC_INACT_FRAME_HI1]		= { 255, 255, 255, 255 },
	[WC_INACT_FRAME_HI2]		= { 128, 128, 128, 255 },
	[WC_INACT_FRAME_SH1]		= {   0,   0,   0, 255 },
	[WC_INACT_FRAME_SH2]		= { 128, 128, 128, 255 },
	[WC_BUTTON_BG]			= { 191, 191, 191, 255 },
	[WC_BUTTON_FG]			= {   0,   0,   0, 255 },
	[WC_BUTTON_HI1]			= { 255, 255, 255, 255 },
	[WC_BUTTON_HI2]			= { 128, 128, 128, 255 },
	[WC_BUTTON_SH1]			= {   0,   0,   0, 255 },
	[WC_BUTTON_SH2]			= { 128, 128, 128, 255 },
	[WC_TTY_CURSOR]			= { 255, 255, 255, 255 },
	[WC_TTY_BG]			= {   0,   0,   0, 255 },
	[WC_TTY_FG]			= { 192, 192, 192, 255 },
};

static void wc_default(struct win_rgba *rgba)
{
	struct win_modeinfo mi;
	int i;
	
	if (win_getmode(&i) || win_modeinfo(i, &mi) || mi.ncolors < 256)
		memcpy(rgba, wc_lowcolor_tab, sizeof wc_lowcolor_tab);
	else
		memcpy(rgba, wc_default_tab, sizeof wc_default_tab);
}

static void wc_load_rgba(struct win_rgba *buf)
{
	char lbuf[256];
	
	char pathname1[PATH_MAX];
	char pathname2[PATH_MAX];
	char name[NAME_MAX + 1];
	const char *home;
	FILE *f = NULL;
	
	if (c_load("/user/w_colors", name, sizeof name))
		strcpy(name, DEFAULT_COLORS);
	
	home = getenv("HOME");
	if (!home)
		home = _PATH_H_DEFAULT;
	
	sprintf(pathname1, "/etc/w_color/%s", name);
	sprintf(pathname2, "%s/.wc/%s", home, name);
	
	f = fopen(pathname1, "r");
	if (!f)
		f = fopen(pathname2, "r");
	if (!f)
		goto fail;
	
	memset(buf, 255, sizeof *buf * WC_COUNT);
	while (fgets(lbuf, sizeof lbuf, f))
	{
		char *p;
		int i;
		
		if (*lbuf == '#' || *lbuf == '\n' || !*lbuf)
			continue;
		
		p = strchr(lbuf, '=');
		if (!p)
			continue;
		*p = 0;
		
		for (i = 0; i < sizeof color_names / sizeof *color_names; i++)
			if (!strcmp(lbuf, color_names[i].name))
			{
				int color_i = color_names[i].index;
				uint32_t v;
				
				v = strtoul(p + 1, NULL, 0);
				
				buf[color_i].a = v >> 24;
				buf[color_i].r = v >> 16;
				buf[color_i].g = v >> 8;
				buf[color_i].b = v;
				break;
			}
	}
	fclose(f);
	return;
fail:
	if (f)
		fclose(f);
	wc_default(buf);
}

void wc_load_cte(void)
{
	struct win_modeinfo mi;
	int i;
	
	win_modeinfo(-1, &mi);
	if (mi.user_cte_count >= WC_COUNT)
	{
		wc_load_rgba(wc_rgba);
		
		for (i = 0; i < WC_COUNT; i++)
			win_setcte(i + mi.user_cte, wc_rgba[i].r, wc_rgba[i].g, wc_rgba[i].b);
	}
}

void wc_load(void)
{
	struct win_modeinfo mi;
	int i;
	
	win_modeinfo(-1, &mi);
	if (mi.user_cte_count >= WC_COUNT)
		for (i = 0; i < WC_COUNT; i++)
			wc_tab[i] = i + mi.user_cte;
	else
	{
		wc_load_rgba(wc_rgba);
		
		for (i = 0; i < WC_COUNT; i++)
			win_rgba2color(&wc_tab[i], wc_rgba[i].r, wc_rgba[i].g, wc_rgba[i].b, wc_rgba[i].a);
	}
	wc_loaded = 1;
}

void wc_get_rgba(int id, struct win_rgba *buf)
{
	if (!wc_loaded)
		wc_load();
	
	if (id < 0 || id >= WC_COUNT)
	{
		memset(buf, 255, sizeof *buf);
		return;
	}
	*buf = wc_rgba[id];
}

win_color wc_get(int id)
{
	if (!wc_loaded)
		wc_load();
	
	if (id < 0 || id >= WC_COUNT)
		return 0;
	
	return wc_tab[id];
}
