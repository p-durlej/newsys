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

#include <priv/copyright.h>
#include <wingui_metrics.h>
#include <wingui_msgbox.h>
#include <wingui_menu.h>
#include <wingui_form.h>
#include <wingui_dlg.h>
#include <wingui_buf.h>
#include <wingui.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <confdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <fcntl.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>

#define FW	320
#define FH	200

static struct gadget *main_edbox;
static struct gadget *main_vsbar;
static struct gadget *main_hsbar;
static struct form *main_form;

struct
{
	int ftd;
} config;

static char pathname[PATH_MAX];
static int visible_line_count;
static int text_modified;
static int top_line, horiz_scroll;

static int  main_form_close(struct form *form);
static void main_form_resize(struct form *form, int w, int h);
static void main_edbox_edit(struct gadget *g);
static void main_edbox_vscroll(struct gadget *g, int topmost_line);
static void main_edbox_hscroll(struct gadget *g, int topmost_line);
static void main_vsbar_move(struct gadget *g, int newpos);
static void main_hsbar_move(struct gadget *g, int newpos);
static int  load_file(const char *pathname);
static int  save_file(void);
static int  ask_for_save(void);
static void new_click(struct menu_item *mi);
static void open_click(struct menu_item *mi);
static void save_click(struct menu_item *mi);
static void save_as_click(struct menu_item *mi);
static void about(struct menu_item *mi);
static void close_click(struct menu_item *mi);
static void update_vsbar(void);
static void update_hsbar(void);
static void update_sbars(void);
static void update_ranges(void);
static void create_form(void);

static void save_config(void)
{
	struct form_state fst;
	
	form_get_state(main_form, &fst);
	c_save("edit_fst", &fst, sizeof fst);
	c_save("edit", &config, sizeof config);
}

static void load_config(void)
{
	int junk;
	
	if (c_load("edit", &config, sizeof config))
		config.ftd = WIN_FONT_SYSTEM;
	if (win_chr_size(config.ftd, &junk, &junk, 'X'))
	{
		msgbox_perror(NULL, "Text Editor",
			      "The configured font is not available. Using \n"
			      "default font instead.\n", errno);
		config.ftd = WIN_FONT_SYSTEM;
	}
}

static int main_form_close(struct form *form)
{
	if (ask_for_save())
		return 0;
	save_config();
	exit(0);
}

static void main_form_resize(struct form *form, int w, int h)
{
	int sbw = wm_get(WM_SCROLLBAR);
	
	gadget_resize(main_edbox, w - sbw, h - sbw);
	gadget_resize(main_vsbar, sbw, h - sbw);
	gadget_resize(main_hsbar, w - sbw, sbw);
	gadget_move(main_vsbar, w - sbw, 0);
	gadget_move(main_hsbar, 0, h - sbw);
	update_ranges();
	update_sbars();
}

static void main_edbox_edit(struct gadget *g)
{
	text_modified = 1;
	win_unsaved(1);
	update_sbars();
}

static void main_edbox_vscroll(struct gadget *g, int topmost_line)
{
	vsbar_set_pos(main_vsbar, topmost_line);
	top_line = topmost_line;
}

static void main_edbox_hscroll(struct gadget *g, int hpix)
{
	hsbar_set_pos(main_hsbar, hpix);
	horiz_scroll = hpix;
}

static void main_vsbar_move(struct gadget *g, int newpos)
{
	edbox_vscroll(main_edbox, newpos);
}

static void main_hsbar_move(struct gadget *g, int newpos)
{
	edbox_hscroll(main_edbox, newpos);
}

static int load_file(const char *pathname)
{
	struct stat st;
	ssize_t count;
	size_t i;
	char *msg;
	char *buf;
	int fd;
	
	fd = open(pathname, O_RDONLY);
	if (fd < 0)
	{
		if (asprintf(&msg, "Cannot open file \"%s\"", pathname) > 0)
		{
			msgbox_perror(main_form, "Text Editor", msg, errno);
			free(msg);
		}
		return -1;
	}
	
	if (fstat(fd, &st))
	{
		if (asprintf(&msg, "Cannot stat file \"%s\"", pathname) > 0)
		{
			msgbox_perror(main_form, "Text Editor", msg, errno);
			free(msg);
		}
		close(fd);
		return -1;
	}
	
	buf = malloc(st.st_size + 1);
	if (!buf)
	{
		if (asprintf(&msg, "Cannot allocate memory for \"%s\":\n\n"
			     "The file may be too long", pathname) > 0)
		{
			msgbox_perror(main_form, "Text Editor", msg, errno);
			free(msg);
		}
		close(fd);
		return -1;
	}
	
	count = read(fd, buf, st.st_size);
	if (count < 0)
	{
		if (asprintf(&msg, "Cannot read file \"%s\"", pathname) > 0)
		{
			msgbox_perror(main_form, "Text Editor", msg, errno);
			free(msg);
		}
		free(buf);
		close(fd);
		return -1;
	}
	buf[count] = 0;
	
	if (count != st.st_size)
	{
		if (asprintf(&msg, "\"%s\":\n\nShort read\n"
				   "(File size changed between fstat and read)", pathname) > 0)
		{
			msgbox(main_form, "Text Editor", msg);
			free(msg);
		}
		free(buf);
		close(fd);
		return -1;
	}
	
	for (i = 0; i < count; i++)
		if (!buf[i])
		{
			if (asprintf(&msg, "File \"%s\" is a binary file and "
					   "cannot be\nopened in this program.", pathname) > 0)
			{
				msgbox(main_form, "Text Editor", msg);
				free(msg);
			}
			close(fd);
			free(buf);
			return -1;
		}
	
	if (edbox_set_text(main_edbox, buf))
	{
		char msg[64 + PATH_MAX];
		
		sprintf(msg, "Cannot allocate memory for \"%s\":\n\n"
			     "The file may be too long", pathname);
		msgbox_perror(main_form, "Text Editor", msg, errno);
		free(buf);
		close(fd);
		return -1;
	}
	
	form_set_title(main_form, pathname);
	update_sbars();
	
	text_modified = 0;
	win_unsaved(0);
	close(fd);
	free(buf);
	return 0;
}

static int save_file(void)
{
	char tmp[PATH_MAX];
	char *p;
	int len;
	int fd;
	int i;
	
	if (!*pathname && !dlg_save(main_form, "Save as", pathname, sizeof pathname))
		return -1;
	
	strcpy(tmp, pathname);
	p = strrchr(tmp, '/');
	if (p)
		p++;
	else
		p = tmp;
	
	for (i = 0; i < 1000; i++)
	{
		sprintf(p, "edit-%05i%03i.tmp", (int)getpid(), i);
		fd = open(tmp, O_WRONLY | O_CREAT | O_EXCL, 0666);
		if (fd >= 0)
			break;
		if (errno != EEXIST)
		{
			char msg[128 + PATH_MAX];
			
			sprintf(msg, "Cannot create file \"%s\"", tmp);
			msgbox_perror(main_form, "Text Editor", msg, errno);
			return -1;
		}
	}
	if (i == 256)
	{
		msgbox_perror(main_form, "Text Editor", "The required temporary file could not be created.", errno);
		return -1;
	}
	
	len = strlen(main_edbox->text);
	if (write(fd, main_edbox->text, len) != len)
	{
		char msg[128 + PATH_MAX];
		
		sprintf(msg, "Cannot write \"%s\"", tmp);
		msgbox_perror(main_form, "Text Editor", msg, errno);
		close(fd);
		unlink(tmp);
		return -1;
	}
	close(fd);
	
	if (rename(tmp, pathname))
	{
		char msg[128 + PATH_MAX];
		
		sprintf(msg, "Cannot save file \"%s\"", pathname);
		msgbox_perror(main_form, "Text Editor", msg, errno);
		unlink(tmp);
		return -1;
	}
	
	text_modified = 0;
	win_unsaved(0);
	return 0;
}

static int ask_for_save(void)
{
	if (!text_modified)
		return 0;
	if (msgbox_ask(main_form, "Text Editor",
		       "The document has been modified.\n\n"
		       "Do you wish to save the changes?") != MSGBOX_YES)
		return 0;
	return save_file();
}

static void new_click(struct menu_item *mi)
{
	if (ask_for_save())
		return;
	
	form_set_title(main_form, "Text Editor");
	edbox_set_text(main_edbox, "");
	update_sbars();
	text_modified = 0;
	win_unsaved(0);
	*pathname = 0;
}

static void open_click(struct menu_item *mi)
{
	char p[PATH_MAX];
	
	if (ask_for_save())
		return;
	
	strcpy(p, pathname);
	if (dlg_open(main_form, "Open", p, sizeof p))
	{
		if (load_file(p))
			return;
		form_set_title(main_form, p);
		strcpy(pathname, p);
	}
}

static void save_click(struct menu_item *mi)
{
	save_file();
}

static void save_as_click(struct menu_item *mi)
{
	if (dlg_save(main_form, "Save as", pathname, sizeof pathname))
	{
		form_set_title(main_form, pathname);
		save_file();
	}
}

static void about(struct menu_item *mi)
{
	dlg_about7(main_form, NULL, "Text Editor v1.3", SYS_PRODUCT, SYS_AUTHOR, SYS_CONTACT, "text.pnm");
}

static void close_click(struct menu_item *mi)
{
	form_close(main_form);
}

static void font_click(struct menu_item *mi)
{
	config.ftd = mi->l_data;
	edbox_set_font(main_edbox, config.ftd);
	update_ranges();
	update_sbars();
}

static void cut_click(struct menu_item *mi)
{
	int ss;
	int se;
	
	edbox_sel_get(main_edbox, &ss, &se);
	if (ss == se)
	{
		msgbox(main_form, "Text Editor", "No text was selected.");
		return;
	}
	
	if (wbu_store(main_edbox->text + ss, se - ss))
	{
		msgbox_perror(main_form, "Text Editor", "Cannot cut", errno);
		return;
	}
	edbox_sel_delete(main_edbox);
}

static void copy_click(struct menu_item *mi)
{
	int ss;
	int se;
	
	edbox_sel_get(main_edbox, &ss, &se);
	if (ss == se)
	{
		msgbox(main_form, "Text Editor", "No text was selected.");
		return;
	}
	
	if (wbu_store(main_edbox->text + ss, se - ss))
	{
		msgbox_perror(main_form, "Text Editor", "Cannot copy", errno);
		return;
	}
}

static void paste_click(struct menu_item *mi)
{
	char *buf;
	
	buf = wbu_load();
	if (!buf)
		goto err;
	if (edbox_insert(main_edbox, buf))
		goto err;
	free(buf);
	return;
err:
	msgbox_perror(main_form, "Text Editor", "Cannot paste", errno);
}

static void delete_click(struct menu_item *mi)
{
	edbox_sel_delete(main_edbox);
}

static void update_vsbar(void)
{
	int limit;
	
	edbox_get_line_count(main_edbox, &limit);
	limit -= visible_line_count - 1;
	if (limit <= top_line)
		limit = top_line + 1;
	vsbar_set_limit(main_vsbar, limit);
}

static void update_hsbar(void)
{
	int limit;
	
	edbox_get_text_width(main_edbox, &limit);
	limit -= main_edbox->rect.w;
	if (limit <= horiz_scroll)
		limit = horiz_scroll + 1;
	hsbar_set_limit(main_hsbar, limit);
}

static void update_sbars(void)
{
	update_vsbar();
	update_hsbar();
}

static void update_ranges(void)
{
	int w, h;
	
	win_chr_size(config.ftd, &w, &h, 'X');
	visible_line_count = main_edbox->rect.h / h;
}

static void create_form(void)
{
	int sbw = wm_get(WM_SCROLLBAR);
	struct form_state fst;
	struct menu *file;
	struct menu *edit;
	struct menu *font;
	struct menu *options;
	struct menu *m;
	
	file = menu_creat();
	menu_newitem4(file, "New",		'N', new_click);
	menu_newitem (file, "-", NULL);
	menu_newitem4(file, "Open ...",		'O', open_click);
	menu_newitem (file, "-", NULL);
	menu_newitem4(file, "Save",		'S', save_click);
	menu_newitem4(file, "Save as ...",	'F', save_as_click);
	menu_newitem (file, "-", NULL);
	menu_newitem4(file, "Close",		'W', close_click);
	
	edit = menu_creat();
	menu_newitem5(edit, "Cut",		'X', WIN_SHIFT_CTRL,	cut_click);
	menu_newitem5(edit, "Copy",		'C', WIN_SHIFT_CTRL,	copy_click);
	menu_newitem5(edit, "Paste",		'V', WIN_SHIFT_CTRL,	paste_click);
	menu_newitem (edit, "-", NULL);
	menu_newitem5(edit, "Delete",		WIN_KEY_DEL, 0,		delete_click);
	
	font = menu_creat();
	menu_newitem4(font, "System",			'D', font_click)->l_data = WIN_FONT_SYSTEM;
	menu_newitem4(font, "Mono",			'M', font_click)->l_data = WIN_FONT_MONO;
	menu_newitem4(font, "Narrow Mono",		0,   font_click)->l_data = WIN_FONT_MONO_N;
	menu_newitem (font, "-", NULL);
	menu_newitem4(font, "Large System",		'L', font_click)->l_data = WIN_FONT_SYSTEM_L;
	menu_newitem4(font, "Large Mono",		0,   font_click)->l_data = WIN_FONT_MONO_L;
	menu_newitem4(font, "Large Narrow Mono",	0,   font_click)->l_data = WIN_FONT_MONO_LN;
	
	options = menu_creat();
	menu_newitem(options, "About ...", about);
	
	m = menu_creat();
	menu_submenu(m, "File",		file);
	menu_submenu(m, "Edit",		edit);
	menu_submenu(m, "Font",		font);
	menu_submenu(m, "Options",	options);
	
	main_form = form_creat(FORM_APPFLAGS | FORM_NO_BACKGROUND, 0, -1, -1, FW, FH, "Text Editor");
	form_set_menu(main_form, m);
	form_on_close(main_form, main_form_close);
	form_on_resize(main_form, main_form_resize);
	
	main_edbox = edbox_creat(main_form, 0, 0, FW - sbw, FH - sbw);
	edbox_set_font(main_edbox, config.ftd);
	edbox_on_edit(main_edbox, main_edbox_edit);
	edbox_on_hscroll(main_edbox, main_edbox_hscroll);
	edbox_on_vscroll(main_edbox, main_edbox_vscroll);
	main_edbox->menu = edit;
	
	main_vsbar = vsbar_creat(main_form, FW - sbw, 0, sbw, FH - sbw);
	main_hsbar = hsbar_creat(main_form, 0, FH - sbw, FW - sbw, sbw);
	vsbar_on_move(main_vsbar, main_vsbar_move);
	hsbar_on_move(main_hsbar, main_hsbar_move);
	
	sizebox_creat(main_form, sbw, sbw);
	
	if (!c_load("edit_fst", &fst, sizeof fst))
		form_set_state(main_form, &fst);
	update_ranges();
	update_sbars();
	
	gadget_focus(main_edbox);
	form_show(main_form);
}

static void on_save(void)
{
	if (ask_for_save())
		return;
	
	win_unsaved(0);
}

int main(int argc, char **argv)
{
	if (win_attach())
		err(255, NULL);
	
	win_on_save(on_save);
	load_config();
	create_form();
	
	if (argc > 2)
	{
		msgbox(NULL, "Text Editor", "usage:\n\n  edit [FILENAME]");
		return 255;
	}
	
	if (argc == 2)
	{
		if (strlen(argv[1]) >= sizeof pathname)
			msgbox(NULL, "Text Editor", "File name too long");
		else
		{
			form_set_title(main_form, argv[1]);
			strcpy(pathname, argv[1]);
			if (load_file(pathname))
				return 255;
		}
	}
	
	for (;;)
		win_wait();
}
