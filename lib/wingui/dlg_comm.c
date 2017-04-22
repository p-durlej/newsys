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
#include <wingui_dlg.h>
#include <wingui.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <fmthuman.h>
#include <newtask.h>
#include <mkcanon.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <paths.h>

#define RGBCOLOR_FORM	"/lib/forms/rgbcolor.frm"
#define MACHINE_FORM	"/lib/forms/machine.frm"
#define FILE_FORM	"/lib/forms/file.frm"
#define ABOUT_FORM	"/lib/forms/about.frm"
#define ACEK_PNM	"/lib/acek.pnm"

#define RESULT_OK	1
#define RESULT_CANCEL	2

struct dlg_file_item
{
	char name[NAME_MAX + 1];
	struct stat st;
};

struct dlg_file
{
	char			cwd[PATH_MAX];
	struct gadget *		name_input;
	struct gadget *		file_list;
	struct gadget *		path_label;
	struct form *		form;
	struct form *		pf;
	const char *		title;
	struct dlg_file_item *	items;
	int			item_cnt;
	int			flags;
};

static int dlg_file_cmp(const void *p1, const void *p2)
{
	const struct dlg_file_item *it1 = p1, *it2 = p2;
	
	if (S_ISDIR(it1->st.st_mode) && !S_ISDIR(it2->st.st_mode))
		return -1;
	
	if (!S_ISDIR(it1->st.st_mode) && S_ISDIR(it2->st.st_mode))
		return 1;
	
	return strcmp(it1->name, it2->name);
}

static void dlg_file_load(struct dlg_file *d)
{
	static struct dlg_file_item parent_item = { .name = ".." };
	
	char pn[PATH_MAX];
	struct dirent *de;
	DIR *dir;
	int cnt = 0;
	int i;
	
	free(d->items);
	d->item_cnt = 0;
	d->items = NULL;
	
	list_clear(d->file_list);
	form_lock(d->form);
	form_busy(NULL);
	if (*d->cwd)
		list_newitem(d->file_list, (void *)&parent_item);
	
	dir = opendir(*d->cwd ? d->cwd : "/");
	if (!dir)
	{
		msgbox_perror(d->form, d->title, "Cannot open directory", _get_errno());
		form_unlock(d->form);
		form_unbusy(NULL);
		return;
	}
	win_idle();
	
	while (de = readdir(dir), de)
	{
		win_idle();
		
		if (strlen(d->cwd) + strlen(de->d_name) + 1 >= sizeof pn)
			continue;
		strcpy(pn, d->cwd);
		strcat(pn, "/");
		strcat(pn, de->d_name);
		
		if (*de->d_name == '.')
			continue;
		
		cnt++;
	}
	
	d->items = calloc(cnt, sizeof *d->items);
	if (!d->items)
	{
		msgbox_perror(d->form, d->title, "Cannot allocate memory", _get_errno());
		closedir(dir);
		return;
	}
	
	rewinddir(dir);
	for (d->item_cnt = 0; de = readdir(dir), de; )
	{
		struct dlg_file_item *it = &d->items[d->item_cnt];
		
		if (d->item_cnt >= cnt)
			break;
		
		if (*de->d_name == '.')
			continue;
		
		strcpy(pn, d->cwd);
		strcat(pn, "/");
		strcat(pn, de->d_name);
		
		if (stat(pn, &it->st))
			continue;
		
		switch (it->st.st_mode & S_IFMT)
		{
		case S_IFREG:
			if (d->flags & DLG_FILE_WANT_REG)
				break;
			continue;
		case S_IFCHR:
			if (d->flags & DLG_FILE_WANT_CHR)
				break;
			continue;
		case S_IFBLK:
			if (d->flags & DLG_FILE_WANT_BLK)
				break;
			continue;
		case S_IFDIR:
			break;
		default:
			continue;
		}
		
		strcpy(it->name, de->d_name);
		d->item_cnt++;
	}
	
	closedir(dir);
	
	cnt = d->item_cnt;
	qsort(d->items, cnt, sizeof *d->items, dlg_file_cmp);
	
	for (i = 0; i < cnt; i++)
	{
		struct dlg_file_item *it = &d->items[i];
		
		list_newitem(d->file_list, (void *)it);
		
		if (!strcmp(d->name_input->text, it->name))
			list_set_index(d->file_list, d->file_list->list.item_count - 1);
	}
	form_unlock(d->form);
	form_unbusy(NULL);
}

static void dlg_file_update_path(struct dlg_file *d)
{
	const char *path = d->cwd;
	char str[PATH_MAX + 16];
	
	if (!*path)
		path = "/";
	
	sprintf(str, "Directory: %s", path);
	label_set_text(d->path_label, str);
}

static void dlg_file_enter_dir(struct dlg_file *d, const char *n)
{
	char *p;
	
	if (!strcmp(n, "."))
	{
		dlg_file_load(d);
		return;
	}
	if (!strcmp(n, ".."))
	{
		p = strrchr(d->cwd, '/');
		if (p)
		{
			*p = 0;
			dlg_file_update_path(d);
			dlg_file_load(d);
		}
		return;
	}
	
	if (strlen(d->cwd) + strlen(n) + 1 >= sizeof d->cwd)
	{
		msgbox(d->form, d->title, "Pathname too long.");
		return;
	}
	
	if (d->flags & DLG_FILE_WANT_DIR)
		input_set_text(d->name_input, "");
	strcat(d->cwd, "/");
	strcat(d->cwd, n);
	dlg_file_update_path(d);
	dlg_file_load(d);
}

static void mkmstr(char *buf, mode_t mode)
{
	strcpy(buf, "?---------");
	
	switch (mode & S_IFMT)
	{
	case S_IFREG:
		buf[0] = '-';
		break;
	case S_IFDIR:
		buf[0] = 'd';
		break;
	case S_IFCHR:
		buf[0] = 'c';
		break;
	case S_IFBLK:
		buf[0] = 'b';
		break;
	case S_IFIFO:
		buf[0] = 'p';
		break;
	case S_IFSOCK:
		buf[0] = 's';
		break;
	default:
		;
	}
	
	if (mode & S_IRUSR)
		buf[1] = 'r';
	
	if (mode & S_IWUSR)
		buf[2] = 'w';
	
	if (mode & S_IXUSR)
		buf[3] = 'x';
	
	if (mode & S_ISUID)
		buf[3] = 's';
	
	if (mode & S_IRGRP)
		buf[4] = 'r';
	
	if (mode & S_IWGRP)
		buf[5] = 'w';
	
	if (mode & S_IXGRP)
		buf[6] = 'x';
	
	if (mode & S_ISGID)
		buf[6] = 's';
	
	if (mode & S_IROTH)
		buf[7] = 'r';
	
	if (mode & S_IWOTH)
		buf[8] = 'w';
	
	if (mode & S_IXOTH)
		buf[9] = 'x';
}

static void dlg_file_draw_item(struct gadget *g, int index, int wd, int x, int y, int w, int h)
{
	struct dlg_file_item *it = (void *)list_get_text(g, index);
	win_color bg, fg;
	char mode[16];
	int parent;
	
	int cw, ch;
	
	if (list_get_index(g) == index)
	{
		bg = wc_get(WC_SEL_BG);
		fg = wc_get(WC_SEL_FG);
	}
	else
	{
		bg = wc_get(WC_BG);
		fg = wc_get(WC_FG);
	}
	
	mkmstr(mode, it->st.st_mode);
	
	parent = !strcmp(it->name, "..");
	
	if (parent)
		mode[0] = '^';
	
	win_chr_size(WIN_FONT_DEFAULT, &cw, &ch, 'X');
	
	win_rect(wd, bg, x, y, w, h);
	win_btext(wd, bg, fg, x + 18 * cw, y, it->name);
	win_btext(wd, bg, fg, x + 2, y, mode);
	
	if (!parent)
	{
		const char *s;
		int tw, th;
		
		s = fmthumanoffs(it->st.st_size, 1);
		
		win_text_size(WIN_FONT_DEFAULT, &tw, &th, s);
		win_btext(wd, bg, fg, x + 17 * cw - tw, y, s);
	}
}

static void dlg_file_request(struct gadget *g, int i, int b)
{
	struct dlg_file *d = g->p_data;
	char pathname[PATH_MAX];
	char msg[NAME_MAX + 32];
	struct stat st;
	const char *s;
	
	s = list_get_text(g, i);
	if (!s)
	{
		input_set_text(d->name_input, "");
		return;
	}
	
	snprintf(pathname, sizeof pathname, "%s/%s", d->cwd, s);
	pathname[sizeof pathname - 1] = 0;
	
	if (stat(pathname, &st))
	{
		int se = _get_errno();
		
		sprintf(msg, "Cannot stat %s", s);
		msgbox_perror(d->form, d->title, msg, se);
		return;
	}
	
	if (S_ISDIR(st.st_mode))
	{
		dlg_file_enter_dir(d, s);
		return;
	}
	
	input_set_text(d->name_input, s);
	form_set_result(d->form, RESULT_OK);
}

static void dlg_file_select(struct gadget *g, int i, int b)
{
	struct dlg_file *d = g->p_data;
	char pathname[PATH_MAX];
	char msg[NAME_MAX + 32];
	struct stat st;
	const char *s;
	
	s = list_get_text(g, i);
	if (!s)
	{
		input_set_text(d->name_input, "");
		return;
	}
	
	snprintf(pathname, sizeof pathname, "%s/%s", d->cwd, s);
	pathname[sizeof pathname - 1] = 0;
	
	if (stat(pathname, &st))
	{
		int se = _get_errno();
		
		sprintf(msg, "Cannot stat %s", s);
		msgbox_perror(d->form, d->title, msg, se);
		return;
	}
	
	if (S_ISDIR(st.st_mode))
		return;
	
	input_set_text(d->name_input, s);
}

int dlg_file6(struct form *pf, const char *title, const char *button, char *pathname, int maxlen, int flags)
{
	char cwd[PATH_MAX];
	struct dlg_file d;
	struct stat st;
	char *p;
	int r;
	
	if (flags & DLG_FILE_WANT_ANY)
		flags |= 31;
	
	bzero(&d, sizeof d);
	d.flags = flags;
	
	if (!getcwd(cwd, sizeof cwd))
	{
		msgbox_perror(pf, title, "Getcwd failed", _get_errno());
		return 0;
	}
	if (strlen(pathname) >= PATH_MAX)
	{
		msgbox(pf, title, "Path too long.");
		return 0;
	}
	strcpy(d.cwd, pathname);
	if (_mkcanon(cwd, d.cwd))
	{
		msgbox_perror(pf, title, "_mkcanon failed", _get_errno());
		return 0;
	}
	
	d.form = form_load(FILE_FORM);
	if (!d.form)
	{
		msgbox_perror(pf, title, "Cannot load " FILE_FORM, _get_errno());
		return 0;
	}
	d.file_list  = gadget_find(d.form, "file_list");
	d.path_label = gadget_find(d.form, "path_label");
	d.name_input = gadget_find(d.form, "name_input");
	d.pf	     = pf;
	d.title	     = title;
	form_set_title(d.form, title);
	list_on_request(d.file_list, dlg_file_request);
	list_on_select(d.file_list, dlg_file_select);
	list_set_draw_cb(d.file_list, dlg_file_draw_item);
	d.file_list->p_data = &d;
	
	if (stat(d.cwd, &st) || !S_ISDIR(st.st_mode))
	{
		p = strrchr(d.cwd, '/');
		if (p)
		{
			input_set_text(d.name_input, p + 1);
			*p = 0;
		}
	}
	
	if (((d.flags & ~DLG_FILE_WANT_DIR) & 31) == 0)
	{
		gadget_hide(gadget_find(d.form, "name_label"));
		gadget_hide(d.name_input);
	}
	
	dlg_file_update_path(&d);
	dlg_file_load(&d);
	form_set_dialog(pf, d.form);
retry:
	r = form_wait(d.form);
	if (r == RESULT_OK)
	{
		if (!*d.name_input->text && !(d.flags & DLG_FILE_WANT_DIR))
			goto retry;
		
		if (strlen(d.cwd) + strlen(d.name_input->text) + 1 >= maxlen)
		{
			msgbox(d.form, title, "Pathname too long.");
			goto retry;
		}
		strcpy(pathname, d.cwd);
		strcat(pathname, "/");
		strcat(pathname, d.name_input->text);
		
		if ((d.flags & DLG_FILE_CONFIRM_OVERWRITE) && !access(pathname, 0))
			if (msgbox_ask(d.form, title, "The file already exists.\n\nOverwrite?") == MSGBOX_NO)
				goto retry;
	}
	form_set_dialog(pf, NULL);
	form_close(d.form);
	
	free(d.items);
	return r == RESULT_OK;
}

int dlg_file(struct form *pf, const char *title, const char *button, char *pathname, int maxlen)
{
	return dlg_file6(pf, title, button, pathname, maxlen, DLG_FILE_WANT_REG);
}

int dlg_open(struct form *pf, const char *title, char *pathname, int maxlen)
{
	return dlg_file(pf, title, "Open", pathname, maxlen);
}

int dlg_save(struct form *pf, const char *title, char *pathname, int maxlen)
{
	return dlg_file6(pf, title, "Save", pathname, maxlen, DLG_FILE_WANT_REG | DLG_FILE_CONFIRM_OVERWRITE);
}

struct dlg_color_gptrs
{
	struct gadget *rsb;
	struct gadget *gsb;
	struct gadget *bsb;
	struct gadget *cs;
};

static void dlg_color_sbmove(struct gadget *g, int pos)
{
	struct dlg_color_gptrs *gp;
	struct win_rgba rgba;
	
	gp = g->form->p_data;
	rgba.r = hsbar_get_pos(gp->rsb);
	rgba.g = hsbar_get_pos(gp->gsb);
	rgba.b = hsbar_get_pos(gp->bsb);
	rgba.a = 255;
	colorsel_set(gp->cs, &rgba);
}

int dlg_color(struct form *pf, const char *title, struct win_rgba *rgba)
{
	struct dlg_color_gptrs gp;
	struct form *form;
	int r;
	
	form = form_load(RGBCOLOR_FORM);
	if (!form)
	{
		msgbox_perror(pf, "Error", "Cannot load " RGBCOLOR_FORM, _get_errno());
		return 0;
	}
	form_set_title(form, title);
	form->p_data = &gp;
	gp.rsb = gadget_find(form, "rsb");
	gp.gsb = gadget_find(form, "gsb");
	gp.bsb = gadget_find(form, "bsb");
	gp.cs  = gadget_find(form, "cs");
	gp.cs->read_only = 1;
	
	hsbar_set_limit(gp.rsb, 256);
	hsbar_set_limit(gp.gsb, 256);
	hsbar_set_limit(gp.bsb, 256);
	
	hsbar_set_pos(gp.rsb, rgba->r);
	hsbar_set_pos(gp.gsb, rgba->g);
	hsbar_set_pos(gp.bsb, rgba->b);
	dlg_color_sbmove(gp.rsb, 0);
	
	hsbar_on_move(gp.rsb, dlg_color_sbmove);
	hsbar_on_move(gp.gsb, dlg_color_sbmove);
	hsbar_on_move(gp.bsb, dlg_color_sbmove);
	
	form_set_dialog(pf, form);
	r = form_wait(form);
	if (r == 1)
	{
		rgba->r = hsbar_get_pos(gp.rsb);
		rgba->g = hsbar_get_pos(gp.gsb);
		rgba->b = hsbar_get_pos(gp.bsb);
		rgba->a = 255;
	}
	form_set_dialog(pf, NULL);
	form_close(form);
	return r == 1;
}

static void dlg_about6_icon_ptr_down(struct gadget *g, int x, int y, int button)
{
	int w, h;
	
	w = g->rect.w;
	h = g->rect.h;
	
	pict_load(g, "/lib/icons/wink.pnm");
	pict_scale(g, w, h);
}

static void dlg_about6_icon_ptr_up(struct gadget *g, int x, int y, int button)
{
	int w, h;
	
	w = g->rect.w;
	h = g->rect.h;
	
	pict_load(g, "/lib/icons/nles.pnm");
	pict_scale(g, w, h);
}

void dlg_about7(struct form *pf,
	const char *title, const char *item, const char *product,
	const char *author, const char *contact, const char *icon)
{
	struct gadget *l_item;
	struct gadget *l_prod;
	struct gadget *l_auth;
	struct gadget *l_mail;
	struct gadget *b_mach;
	struct gadget *p_icon;
	struct form *form;
	
	if (title == NULL)	title	= "About";
	if (item == NULL)	item	= "";
	if (product == NULL)	product	= "";
	if (author == NULL)	author	= "";
	if (contact == NULL)	contact	= "";
	
	form = form_load(ABOUT_FORM);
	if (form == NULL)
		return;
	
	l_item = gadget_find(form, "l_item");
	l_prod = gadget_find(form, "l_prod");
	l_auth = gadget_find(form, "l_auth");
	l_mail = gadget_find(form, "l_mail");
	b_mach = gadget_find(form, "b_mach");
	p_icon = gadget_find(form, "p_icon");
	
	p_icon->ptr_down = dlg_about6_icon_ptr_down;
	p_icon->ptr_up	 = dlg_about6_icon_ptr_up;
	
	if (icon)
	{
		char pathname1[PATH_MAX];
		char pathname2[PATH_MAX];
		
		if (*icon != '/')
		{
			sprintf(pathname1, "/lib/icons64/%s", icon);
			sprintf(pathname2, "/lib/icons/%s", icon);
		}
		
		if (p_icon->rect.w > 48 && !access(pathname1, R_OK))
			pict_load(p_icon, pathname1);
		else if (!access(pathname2, R_OK))
			pict_load(p_icon, pathname2);
		else
			pict_load(p_icon, icon);
	}
	
	gadget_hide(b_mach); /* XXX */
	
	form_set_title(form, title);
	label_set_text(l_item, item);
	label_set_text(l_prod, product);
	label_set_text(l_auth, author);
	label_set_text(l_mail, contact);
	
	if (pf != NULL)
		form_set_dialog(pf, form);
	form_wait(form);
	if (pf != NULL)
		form_set_dialog(pf, NULL);
	form_close(form);
}

void dlg_about6(struct form *pf,
	const char *title, const char *item, const char *product,
	const char *author, const char *contact)
{
	dlg_about7(pf, title, item, product, author, contact, NULL);
}

void dlg_about(struct form *pf, const char *title, const char *desc)
{
	dlg_about6(pf, title, desc, NULL, NULL, NULL);
}

static int dlg_input_key_down(struct form *f, unsigned ch, unsigned shift)
{
	switch (ch)
	{
	case '\n':
		form_set_result(f, 1);
		return 0;
	case '\033':
		if (f->l_data & DLG_INPUT_ALLOW_CANCEL)
		{
			form_set_result(f, 2);
			return 0;
		}
		break;
	}
	return 1;
}

int dlg_input5(struct form *pf, const char *title, char *buf, int maxlen, int flags)
{
	struct gadget *label;
	struct gadget *input;
	struct gadget *cnc;
	struct form *form;
	int r;
	
	form = form_load("/lib/forms/input.frm");
	if (form == NULL)
		return 0;
	
	form_on_key_down(form, dlg_input_key_down);
	form->l_data = flags;
	
	label = gadget_find(form, "label");
	input = gadget_find(form, "in");
	cnc   = gadget_find(form, "cancel");
	
	label_set_text(label, title);
	input_set_text(input, buf);
	
	if (!(flags & DLG_INPUT_ALLOW_CANCEL))
		gadget_hide(cnc);
	
	if (pf != NULL)
		form_set_dialog(pf, form);
retry:
	gadget_focus(input);
	r = form_wait(form);
	if (strlen(input->text) >= maxlen)
	{
		msgbox(form, title, "Input too long.");
		goto retry;
	}
	strcpy(buf, input->text);
	
	if (pf != NULL)
		form_set_dialog(pf, NULL);
	form_close(form);
	return r;
}

int dlg_input(struct form *pf, const char *title, char *buf, int maxlen)
{
	return dlg_input5(pf, title, buf, maxlen, 0);
}

void dlg_reboot(struct form *pf, const char *title)
{
	int r;
	
	if (!title)
		title = "Reboot";
	
	if (getenv("OSDEMO"))
	{
		msgbox(pf, title, "This configuration change cannot be applied when\n"
				  "the system is running in the live CD/USB mode.");
		return;
	}
	
	if (!geteuid())
	{
		r = msgbox_ask(pf, title,
			"The configuration change requires a reboot.\n\n"
			"Do you want to reboot the system?");
		if (r == MSGBOX_YES)
			_newtaskl(_PATH_B_SHUTDOWN, _PATH_B_SHUTDOWN, "-r", (void *)NULL);
		return;
	}
	
	msgbox(pf, title, "Reboot to update the configuration.");
}

void dlg_newsess(struct form *pf, const char *title)
{
	if (!title)
		title = "Session Restart";
	
	if (msgbox_ask(pf, title,
		"The configuration change requires a session restart.\n\n"
		"Do you want to restart the session?") == MSGBOX_YES)
	{
		char *p = getenv("SESSMGR");
		
		if (!p)
		{
			msgbox(pf, title, "Session manager not found.");
			return;
		}
		
		if (kill(atoi(p), SIGUSR1))
			msgbox(pf, title, "Failed to restart the session.");
	}
}
