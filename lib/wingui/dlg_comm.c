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

#include <wingui_msgbox.h>
#include <wingui_form.h>
#include <wingui_dlg.h>
#include <wingui.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <mkcanon.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#define RGBCOLOR_FORM	"/lib/forms/rgbcolor.frm"
#define MACHINE_FORM	"/lib/forms/machine.frm"
#define FILE_FORM	"/lib/forms/file.frm"
#define ABOUT_FORM	"/lib/forms/about.frm"
#define ACEK_PNM	"/lib/acek.pnm"

#define RESULT_OK	1
#define RESULT_CANCEL	2

struct dlg_file
{
	char		cwd[PATH_MAX];
	struct gadget *	name_input;
	struct gadget *	file_list;
	struct gadget *	dir_list;
	struct form *	form;
	struct form *	pf;
	const char *	title;
};

static void dlg_file_load(struct dlg_file *d)
{
	char pn[PATH_MAX];
	struct dirent *de;
	struct stat st;
	DIR *dir;
	int cnt = 0;
	int i;
	
	list_clear(d->file_list);
	list_clear(d->dir_list);
	form_lock(d->form);
	form_busy(NULL);
	if (*d->cwd)
		list_newitem(d->dir_list, "..");
	
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
		if (stat(pn, &st))
			continue;
		if (S_ISDIR(st.st_mode))
			list_newitem(d->dir_list, de->d_name);
		else if (S_ISREG(st.st_mode))
		{
			list_newitem(d->file_list, de->d_name);
			cnt++;
		}
	}
	closedir(dir);
	
	list_sort(d->file_list);
	list_sort(d->dir_list);
	for (i = 0; i < cnt; i++)
		if (!strcmp(d->name_input->text, list_get_text(d->file_list, i)))
		{
			list_set_index(d->file_list, i);
			break;
		}
	list_set_index(d->dir_list, 0);
	form_unlock(d->form);
	form_unbusy(NULL);
}

static void dlg_file_request(struct gadget *g, int i, int b)
{
	struct dlg_file *d = g->p_data;
	
	input_set_text(d->name_input, list_get_text(g, i));
	form_set_result(d->form, RESULT_OK);
}

static void dlg_file_select(struct gadget *g, int i, int b)
{
	struct dlg_file *d = g->p_data;
	char *s;
	
	s = list_get_text(g, i);
	if (!s)
		s = "";
	
	input_set_text(d->name_input, s);
}

static void dlg_dir_request(struct gadget *g, int i, int b)
{
	struct dlg_file *d = g->p_data;
	char *p;
	char *n;
	
	n = list_get_text(g, i);
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
			dlg_file_load(d);
		}
		return;
	}
	
	if (strlen(d->cwd) + strlen(n) + 1 >= sizeof d->cwd)
	{
		msgbox(d->form, d->title, "Pathname too long.");
		return;
	}
	strcat(d->cwd, "/");
	strcat(d->cwd, n);
	dlg_file_load(d);
}

int dlg_file(struct form *pf, const char *title, const char *button, char *pathname, int maxlen)
{
	char cwd[PATH_MAX];
	struct dlg_file d;
	struct stat st;
	char *p;
	int r;
	
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
	d.dir_list   = gadget_find(d.form, "dir_list");
	d.name_input = gadget_find(d.form, "name_input");
	d.pf	     = pf;
	d.title	     = title;
	form_set_title(d.form, title);
	list_on_request(d.file_list, dlg_file_request);
	list_on_request(d.dir_list, dlg_dir_request);
	list_on_select(d.file_list, dlg_file_select);
	d.file_list->p_data = &d;
	d.dir_list->p_data  = &d;
	
	if (stat(d.cwd, &st) || !S_ISDIR(st.st_mode))
	{
		p = strrchr(d.cwd, '/');
		if (p)
		{
			input_set_text(d.name_input, p + 1);
			*p = 0;
		}
	}
	
	dlg_file_load(&d);
	form_set_dialog(pf, d.form);
retry:
	r = form_wait(d.form);
	if (r == RESULT_OK)
	{
		if (strlen(d.cwd) + strlen(d.name_input->text) + 1 >= maxlen)
		{
			msgbox(d.form, title, "Pathname too long.");
			goto retry;
		}
		strcpy(pathname, d.cwd);
		strcat(pathname, "/");
		strcat(pathname, d.name_input->text);
	}
	form_set_dialog(pf, NULL);
	form_close(d.form);
	return r == RESULT_OK;
}

int dlg_open(struct form *pf, const char *title, char *pathname, int maxlen)
{
	return dlg_file(pf, title, "Open", pathname, maxlen);
}

int dlg_save(struct form *pf, const char *title, char *pathname, int maxlen)
{
	return dlg_file(pf, title, "Save", pathname, maxlen);
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
		const char *icd = "/lib/icons";
		
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
