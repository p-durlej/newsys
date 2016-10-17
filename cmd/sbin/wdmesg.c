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

#include <priv/copyright.h>
#include <wingui_metrics.h>
#include <wingui_msgbox.h>
#include <wingui_form.h>
#include <wingui_menu.h>
#include <wingui_dlg.h>
#include <wingui_buf.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <confdb.h>
#include <errno.h>
#include <os386.h>
#include <timer.h>
#include <time.h>
#include <err.h>

#define WIDTH	600
#define HEIGHT	400

struct gadget *vsbar;
struct gadget *edbox;
struct form *form;
struct menu *menu_edit;
struct menu *menu_font;
struct menu *menu_opt;
struct menu *menu;

static int	zflag;

static void fetch_dmesg(void)
{
	static char buf[DMESG_BUF + 1];
	static int lastpos = -1;
	
	int pos;
	
	pos = _dmesg(buf, sizeof buf - 1);
	if (pos < 0)
	{
		msgbox_perror(form, "Error", "_dmesg", errno);
		exit(errno);
	}
	if (lastpos < 0)
		lastpos = (pos + 1) % DMESG_BUF;
	if (lastpos == pos)
		return;
	
	buf[pos] = 0;
	edbox_goto_index(edbox, strlen(edbox->text));
	edbox_insert(edbox, buf + lastpos);
	if (pos < lastpos)
		edbox_insert(edbox, buf);
	edbox_goto_index(edbox, strlen(edbox->text));
	lastpos = pos;
}

static void on_scroll(struct gadget *g, int pos)
{
	int cnt;
	
	edbox_get_line_count(edbox, &cnt);
	vsbar_set_limit(vsbar, cnt);
	
	vsbar_set_pos(vsbar, pos);
	edbox_vscroll(edbox, pos);
}

static void on_resize(struct form *f, int w, int h)
{
	int sbw = wm_get(WM_SCROLLBAR);
	
	gadget_resize(edbox, w - sbw, h);
	gadget_resize(vsbar, sbw, h);
	gadget_move(vsbar, w - sbw, 0);
}

static void on_edit(struct gadget *g)
{
	int cnt;
	
	edbox_get_line_count(edbox, &cnt);
	vsbar_set_limit(vsbar, cnt);
}

static void m_setfont(struct menu_item *m)
{
	edbox_set_font(edbox, m->l_data);
}

static void m_about(struct menu_item *m)
{
	dlg_about7(form, NULL, "Diagnostic Messages v1", SYS_PRODUCT, SYS_AUTHOR, SYS_CONTACT, "diag.pnm");
}

static void m_copy(struct menu_item *m)
{
	int s, e;
	
	edbox_sel_get(edbox, &s, &e);
	if (e > s && wbu_store(edbox->text + s, e - s))
		msgbox_perror(form, "Copy", "Cannot copy", errno);
}

static int on_close(struct form *f)
{
	struct form_state fst;
	
	form_get_state(f, &fst);
	c_save("wdmesg", &fst, sizeof fst);
	return 1;
}

static void update(void *data)
{
	fetch_dmesg();
}

int main(int argc, char **argv)
{
	struct timeval utv = { .tv_sec = 0, .tv_usec = 250000 };
	int sbw = wm_get(WM_SCROLLBAR);
	struct form_state fst;
	struct timer *tmr;
	int c;
	
	while (c = getopt(argc, argv, "z"), c > 0)
		switch (c)
		{
		case 'z':
			zflag = 1;
			break;
		}
	
	if (win_attach())
		err(255, NULL);
	
	menu_edit = menu_creat();
	menu_newitem(menu_edit, "Copy", m_copy);
	
	menu_font = menu_creat();
	menu_newitem(menu_font, "Mono", m_setfont)->l_data = WIN_FONT_MONO;
	menu_newitem(menu_font, "System", m_setfont)->l_data = WIN_FONT_SYSTEM;
	menu_newitem(menu_font, "Large Mono", m_setfont)->l_data = 2;
	menu_newitem(menu_font, "Large System", m_setfont)->l_data = 3;
	
	menu_opt = menu_creat();
	menu_newitem(menu_opt, "About", m_about);
	
	menu = menu_creat();
	menu_submenu(menu, "Edit", menu_edit);
	menu_submenu(menu, "Font", menu_font);
	menu_submenu(menu, "Options", menu_opt);
	
	form  = form_creat(FORM_APPFLAGS|FORM_NO_BACKGROUND, 0, -1, -1, WIDTH, HEIGHT, "Diagnostic Messages");
	edbox = edbox_creat(form, 0, 0, WIDTH - sbw, HEIGHT);
	vsbar = vsbar_creat(form, WIDTH - sbw, 0, sbw, HEIGHT);
	edbox_set_font(edbox, WIN_FONT_MONO);
	edbox_on_vscroll(edbox, on_scroll);
	edbox_on_edit(edbox, on_edit);
	vsbar_on_move(vsbar, on_scroll);
	form_on_resize(form, on_resize);
	form_on_close(form, on_close);
	gadget_focus(edbox);
	form_set_menu(form, menu);
	edbox->read_only = 1;
	edbox->menu = menu_font;
	
	tmr = tmr_creat(&utv, &utv, update, NULL, 1);
	if (!c_load("wdmesg", &fst, sizeof fst))
		form_set_state(form, &fst);
	
	if (zflag)
	{
		form_get_state(form, &fst);
		fst.state = 1;
		form_set_state(form, &fst);
	}
	
	fetch_dmesg();
	form_wait(form);
	return 0;
}
