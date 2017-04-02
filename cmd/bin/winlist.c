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

#include <wingui_bitmap.h>
#include <wingui_msgbox.h>
#include <wingui_color.h>
#include <wingui_menu.h>
#include <wingui.h>
#include <confdb.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <event.h>
#include <err.h>

#define WIDTH	24
#define HEIGHT	13

#define WIN_MAX	256

static struct
{
	int x;
	int y;
} config;

static struct bitmap *icon;
static int wd;

static void sig_term()
{
	c_save("winlist", &config, sizeof config);
	exit(0);
}

static void m_click(struct menu_item *m)
{
	win_raise(m->l_data);
	win_focus(m->l_data);
}

static void popup()
{
	char title[WIN_TITLE_MAX + 1];
	struct menu_item *mi;
	struct menu *menu;
	int vi;
	int i;
	
	menu = menu_creat();
	for (i = 0; i < WIN_MAX; i++)
	{
		if (win_is_visible(i, &vi))
			continue;
		if (win_get_title(i, title, sizeof title))
			continue;
		
		if (vi && *title)
		{
			mi = menu_newitem(menu, title, m_click);
			mi->l_data = i;
		}
	}
	menu_popup(menu, config.x, config.y + HEIGHT);
	menu_free(menu);
}

static void w_proc(struct event *e)
{
	static int ptr_down = 0;
	static int x0;
	static int y0;
	win_color bg;
	
	switch (e->win.type)
	{
	case WIN_E_REDRAW:
		win_clip(wd, e->win.redraw_x, e->win.redraw_y, e->win.redraw_w, e->win.redraw_h, 0, 0);
		
		bg = wc_get(WC_WIN_BG);
		
		win_paint();
		win_rect(wd, bg, 0, 0, WIDTH, HEIGHT);
		bmp_draw(wd, icon, 0, 0);
		win_end_paint();
		break;
	case WIN_E_PTR_DOWN:
		x0 = e->win.ptr_x;
		y0 = e->win.ptr_y;
		ptr_down = 1;
		break;
	case WIN_E_PTR_MOVE:
		if (!ptr_down)
			break;
		
		win_rect_preview(1, config.x + e->win.ptr_x - x0,
				    config.y + e->win.ptr_y - y0,
				    WIDTH, HEIGHT);
		break;
	case WIN_E_PTR_UP:
		if (!ptr_down)
			break;
		
		ptr_down = 0;
		if (x0 == e->win.ptr_x && y0 == e->win.ptr_y)
		{
			popup();
			break;
		}
		
		config.x += e->win.ptr_x - x0;
		config.y += e->win.ptr_y - y0;
		
		win_change(wd, 1, config.x, config.y, WIDTH, HEIGHT);
		win_rect_preview(0, 0, 0, 0, 0);
		break;
	}
}

int main()
{
	evt_signal(SIGTERM, sig_term);
	
	if (win_attach())
		err(255, NULL);
	
	if (c_load("winlist", &config, sizeof config))
	{
		config.x = 0;
		config.y = 0;
	}
	
	icon = bmp_load("/lib/icons/winlist.pnm");
	if (!icon)
	{
		msgbox_perror(NULL, "Window List", "Cannot load \"/lib/icons/winlist.pnm\"", errno);
		return errno;
	}
	bmp_conv(icon);
	
	win_creat(&wd, 1, config.x, config.y, WIDTH, HEIGHT, w_proc, NULL);
	win_setlayer(wd, WIN_LAYER_BELOW_MENU);
	win_raise(wd);
	
	for (;;)
		win_wait();
}
