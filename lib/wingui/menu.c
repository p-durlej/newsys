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

#include <priv/wingui_theme.h>
#include <priv/wingui_form.h>
#include <wingui_metrics.h>
#include <wingui_color.h>
#include <wingui_menu.h>
#include <wingui.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <event.h>

const struct menu_kn
{
	char *		name;
	unsigned	ch;
} menu_kn[] =
{
	{ "Return",	'\n'		},
	
	{ "Up",		WIN_KEY_UP	},
	{ "Down",	WIN_KEY_DOWN	},
	{ "Left",	WIN_KEY_LEFT	},
	{ "Right",	WIN_KEY_RIGHT	},
	
	{ "PgUp",	WIN_KEY_PGUP	},
	{ "PgDn",	WIN_KEY_PGDN	},
	{ "Home",	WIN_KEY_HOME	},
	{ "End",	WIN_KEY_END	},
	
	{ "Ins",	WIN_KEY_INS	},
	{ "Del",	WIN_KEY_DEL	},
	
	{ "F1",		WIN_KEY_F1	},
	{ "F2",		WIN_KEY_F2	},
	{ "F3",		WIN_KEY_F3	},
	{ "F4",		WIN_KEY_F4	},
	{ "F5",		WIN_KEY_F5	},
	{ "F6",		WIN_KEY_F6	},
	{ "F7",		WIN_KEY_F7	},
	{ "F8",		WIN_KEY_F8	},
	{ "F9",		WIN_KEY_F9	},
	{ "F10",	WIN_KEY_F10	},
	{ "F11",	WIN_KEY_F11	},
	{ "F12",	WIN_KEY_F12	},
	{ "F13",	WIN_KEY_F13	},
	{ "F14",	WIN_KEY_F14	},
	{ "F15",	WIN_KEY_F15	},
	{ "F16",	WIN_KEY_F16	},
	
	{ "Shift",	WIN_KEY_SHIFT	},
	{ "Ctrl",	WIN_KEY_CTRL	},
	{ "Alt",	WIN_KEY_ALT	},
	
	{ "SysRq",	WIN_KEY_SYSRQ	},
	
	{ "CapsLock",	WIN_KEY_CAPS	},
	{ "NumLock",	WIN_KEY_NLOCK	},
	{ "ScrollLock",	WIN_KEY_SLOCK	},
};

static void menu_chstr(char *buf, unsigned ch, unsigned shift)
{
	int len;
	int i;
	
	*buf = 0;
	
	if (shift & WIN_SHIFT_ALT)
		strcpy(buf, "Alt+");
	
	if (shift & WIN_SHIFT_CTRL)
	{
		strcat(buf, "Ctrl+");
		if (ch < 0x20)
			ch |= 0x40;
	}
	
	if (shift & WIN_SHIFT_SHIFT)
		strcat(buf, "Shift+");
	
	for (i = 0; i < sizeof menu_kn / sizeof *menu_kn; i++)
		if (menu_kn[i].ch == ch)
		{
			strcat(buf, menu_kn[i].name);
			break;
		}
	if (i >= sizeof menu_kn / sizeof *menu_kn)
	{
		len = strlen(buf);
		buf[len++] = ch;
		buf[len] = 0;
	}
}

struct menu *menu_creat(void)
{
	struct menu *m = malloc(sizeof *m);
	
	if (!m)
		return NULL;
	memset(m, 0, sizeof *m);
	m->selection = -1;
	m->wd = -1;
	return m;
}

void menu_free(struct menu *menu)
{
	int i;
	
	if (menu->wd >= 0)
		win_close(menu->wd);
	for (i = 0; i < menu->item_count; i++)
		free(menu->item[i].text);
	free(menu->item);
	free(menu);
	return;
}

struct menu_item *menu_newitem5(
	struct menu *menu, const char *text, unsigned ch, unsigned shift,
	menu_cb *proc)
{
	struct menu_item *i;
	char *p;
	
	p = strdup(text);
	if (!p)
		return NULL;
	
	i = realloc(menu->item, (menu->item_count + 1) * sizeof(struct menu_item));
	
	if (!i)
	{
		free(p);
		return NULL;
	}
	
	memset(&i[menu->item_count], 0, sizeof *i);
	
	menu->item = i;
	menu->item[menu->item_count].text	= p;
	menu->item[menu->item_count].shift	= shift;
	menu->item[menu->item_count].ch		= ch;
	menu->item[menu->item_count].proc	= proc;
	menu->item[menu->item_count].submenu	= NULL;
	menu->item_count++;
	return &menu->item[menu->item_count - 1];
}

struct menu_item *menu_newitem4(struct menu *menu, const char *text, unsigned ch, menu_cb *proc)
{
	return menu_newitem5(menu, text, ch, WIN_SHIFT_CTRL, proc);
}

struct menu_item *menu_newitem(struct menu *menu, const char *text, menu_cb *proc)
{
	return menu_newitem4(menu, text, 0, proc);
}

struct menu_item *menu_submenu(struct menu *menu, const char *text, struct menu *submenu)
{
	struct menu_item *i;
	char *p;
	
	p = strdup(text);
	if (!p)
		return NULL;
	
	i = realloc(menu->item, (menu->item_count + 1) * sizeof *i);
	
	if (!i)
	{
		free(p);
		return NULL;
	}
	
	memset(&i[menu->item_count], 0, sizeof *i);
	
	menu->item = i;
	menu->item[menu->item_count].text = p;
	menu->item[menu->item_count].proc = NULL;
	menu->item[menu->item_count].submenu = submenu;
	menu->item_count++;
	return &menu->item[menu->item_count - 1];
}

static void menu_make_rects(struct menu *menu, int x0, int y0)
{
	int max_w = 0, max_ow = 0;
	int w, h;
	int tl;
	int y;
	int i;
	
	tl = wm_get(WM_THIN_LINE);
	y  = 2 * tl;
	
	for (i = 0; i < menu->item_count; i++)
	{
		struct menu_item *mi = &menu->item[i];
		
		win_text_size(WIN_FONT_DEFAULT, &w, &h, mi->text);
		w += 10 * tl;
		
		mi->rect.x = 2 * tl;
		mi->rect.y = y;
		mi->rect.h = h;
		
		y += h;
		if (max_w < w)
			max_w = w;
		
		if (mi->ch)
		{
			char buf[256]; /* XXX */
			
			menu_chstr(buf, mi->ch, mi->shift);
			win_text_size(WIN_FONT_DEFAULT, &w, &h, buf);
			w += 10 * tl;
			
			if (max_ow < w)
				max_ow = w;
		}
	}
	max_w += max_ow;
	
	for (i = 0; i < menu->item_count; i++)
		menu->item[i].rect.w = max_w;
	
	menu->rect.x = x0;
	menu->rect.y = y0;
	menu->rect.w = max_w + 4 * tl;
	menu->rect.h = y + 2 * tl;
}

static void menu_redraw_item(struct menu *m, int i)
{
	struct menu_item *mi;
	win_color sh;
	win_color hi;
	win_color fg;
	win_color bg;
	int tl;
	
	if (i < 0)
		return;
	mi = &m->item[i];
	
	sh = wc_get(WC_SHADOW1);
	hi = wc_get(WC_HIGHLIGHT1);
	
	tl = wm_get(WM_THIN_LINE);
	
	if (m->selection != i || (mi->text[0] == '-' && !mi->text[1]))
	{
		fg = wc_get(WC_WIN_FG);
		bg = wc_get(WC_WIN_BG);
	}
	else
	{
		fg = wc_get(WC_SEL_FG);
		bg = wc_get(WC_SEL_BG);
	}
	
	if (mi->text[0] == '-' && !mi->text[1])
	{
		win_rect(m->wd, bg, mi->rect.x, mi->rect.y,
				    mi->rect.w, mi->rect.h);
		
		win_rect(m->wd, sh, mi->rect.x, mi->rect.y + mi->rect.h / 2,	  mi->rect.w, tl);
		win_rect(m->wd, hi, mi->rect.x, mi->rect.y + mi->rect.h / 2 + tl, mi->rect.w, tl);
	}
	else
	{
		win_rect(m->wd, bg, mi->rect.x, mi->rect.y, mi->rect.w, mi->rect.h);
		win_text(m->wd, fg, mi->rect.x + 5, mi->rect.y, mi->text);
		if (mi->ch)
		{
			char buf[256]; /* XXX */
			int w, h;
			int x;
			
			menu_chstr(buf, mi->ch, mi->shift);
			win_text_size(WIN_FONT_DEFAULT, &w, &h, buf);
			
			x = mi->rect.x + mi->rect.w - w - 5;
			win_text(m->wd, fg, x, mi->rect.y, buf);
		}
	}
}

static void menu_redraw(struct menu *m)
{
	const struct form_theme *th;
	win_color hi1;
	win_color hi2;
	win_color sh1;
	win_color sh2;
	win_color bg;
	int i;
	
	th = form_th_get();
	
	hi1 = wc_get(WC_HIGHLIGHT1);
	hi2 = wc_get(WC_HIGHLIGHT2);
	sh1 = wc_get(WC_SHADOW1);
	sh2 = wc_get(WC_SHADOW2);
	bg  = wc_get(WC_WIN_BG);
	
	win_paint();
	if (th != NULL && th->d_menuframe != NULL)
		th->d_menuframe(m->wd, 0, 0, m->rect.w, m->rect.h);
	else
		win_rect(m->wd, bg, 0, 0, m->rect.w, m->rect.h);
	
	for (i = 0; i < m->item_count; i++)
		menu_redraw_item(m, i);
	
	if (th == NULL || th->d_menuframe == NULL)
		form_draw_frame3d(m->wd, 0, 0, m->rect.w, m->rect.h, hi1, hi2, sh1, sh2);
	
	win_end_paint();
}

static void menu_xyitem(int *ip, struct menu *m, int x, int y)
{
	struct menu_item *item;
	int i;
	
	for (i = 0; i < m->item_count; i++)
	{
		item = &m->item[i];
		
		if (x < item->rect.x)
			continue;
		
		if (y < item->rect.y)
			continue;
		
		if (x >= item->rect.x + item->rect.w)
			continue;
		
		if (y >= item->rect.y + item->rect.h)
			continue;
		
		*ip = i;
		return;
	}
	
	*ip = -1;
}

static void menu_key_down(struct menu *m, unsigned ch)
{
	struct menu_item *mi;
	int prev;
	
	switch (ch)
	{
	case '\n':
		if (m->selection < 0)
			break;
		menu_popdown(m);
		
		mi = &m->item[m->selection];
		if (mi->proc)
			mi->proc(mi);
		break;
	case 0x1b:
		menu_popdown(m);
		break;
	case WIN_KEY_UP:
		if (m->selection <= 0)
			break;
		
		prev = m->selection;
		m->selection--;
		
		while (m->selection &&
		       m->item[m->selection].text[0] == '-' &&
		       !m->item[m->selection].text[1])
			m->selection--;
		
		win_paint();
		menu_redraw_item(m, m->selection);
		menu_redraw_item(m, prev);
		win_end_paint();
		break;
	case WIN_KEY_DOWN:
		if (m->selection >= m->item_count - 1)
			break;
		
		prev = m->selection;
		m->selection++;
		
		while (m->selection < m->item_count - 1 &&
		       m->item[m->selection].text[0] == '-' &&
		       !m->item[m->selection].text[1])
			m->selection++;
		
		win_paint();
		menu_redraw_item(m, m->selection);
		menu_redraw_item(m, prev);
		win_end_paint();
		break;
	}
}

static int menu_proc(struct event *e)
{
	struct menu *m = e->data;
	struct menu_item *mi;
	int prev;
	
	switch (e->win.type)
	{
	case WIN_E_SWITCH_FROM:
		menu_popdown(m);
		break;
	case WIN_E_REDRAW:
		menu_redraw(m);
		break;
	case WIN_E_KEY_DOWN:
		menu_key_down(m, e->win.ch);
		break;
	case WIN_E_PTR_MOVE:
		prev = m->selection;
		menu_xyitem(&m->selection, m, e->win.ptr_x, e->win.ptr_y);
		if (prev != m->selection)
		{
			win_paint();
			menu_redraw_item(m, prev);
			menu_redraw_item(m, m->selection);
			win_end_paint();
		}
		break;
	case WIN_E_PTR_UP:
		menu_xyitem(&m->selection, m, e->win.ptr_x, e->win.ptr_y);
		menu_popdown(m);
		
		if (m->selection < 0)
			break;
		mi = &m->item[m->selection];
		if (m->selection >= 0 && mi->proc)
			mi->proc(mi);
		break;
	}
	
	return 0;
}

int menu_popup_l(struct menu *menu, int x, int y, int layer)
{
	struct win_rect wsr;
	
	if (menu->visible)
		menu_popdown(menu);
	
	if (menu->wd < 0)
		if (win_creat(&menu->wd, 0, 0, 0, 0, 0, menu_proc, menu))
			return -1;
	
	menu->visible = 1;
	menu_make_rects(menu, x, y);
	
	win_ws_getrect(&wsr);
	if (x + menu->rect.w > wsr.x + wsr.w)
		x = wsr.w - menu->rect.w;
	if (y + menu->rect.h > wsr.x + wsr.h)
		y = wsr.h - menu->rect.h;
	if (x < wsr.x)
		x = wsr.x;
	if (y < wsr.y)
		y = wsr.y;
	
	win_setlayer(menu->wd, layer);
	win_focus(menu->wd);
	win_raise(menu->wd);
	win_change(menu->wd, 1, x, y, menu->rect.w, menu->rect.h);
	
	while (menu->visible)
		win_wait();
	
	return 0;
}

int menu_popup(struct menu *menu, int x, int y)
{
	return menu_popup_l(menu, x, y, WIN_LAYER_MENU);
}

int menu_popdown(struct menu *menu)
{
	if (!menu->visible)
		return 0;
	
	win_close(menu->wd);
	
	menu->wd      = -1;
	menu->visible = 0;
	return 0;
}
