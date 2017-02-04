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
#include <wingui_bell.h>
#include <wingui_dlg.h>
#include <wingui.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#define RESULT_OK	1
#define RESULT_CANCEL	2

#define MAX_USERS	1024
#define MAX_GROUPS	64

struct us
{
	char name[LOGNAME_MAX + 1];
	uid_t uid;
};

struct gs
{
	char name[LOGNAME_MAX + 1];
	gid_t gid;
};

static void ok_click(struct gadget *g)
{
	g->form->result = RESULT_OK;
	form_hide(g->form);
}

static void cancel_click(struct gadget *g)
{
	g->form->result = RESULT_CANCEL;
	form_hide(g->form);
}

static int user_cmp(const void *a, const void *b)
{
	return strcmp(((struct us *)a)->name, ((struct us *)b)->name);
}

static int group_cmp(const void *a, const void *b)
{
	return strcmp(((struct gs *)a)->name, ((struct gs *)b)->name);
}

int dlg_uid(struct form *pf, const char *title, uid_t *uid)
{
	struct passwd *pw;
	struct gadget *li;
	struct form *f;
	struct us *user;
	char msg[256];
	int cnt;
	int i;
	
	setpwent();
	for (cnt = 0; _set_errno(0), pw = getpwent(), pw; cnt++);
	if (_get_errno())
	{
		msgbox_perror(NULL, title, "Unable to read user list", _get_errno());
		endpwent();
		return 0;
	}
	user = calloc(cnt, sizeof *user);
	if (!user)
	{
		msgbox_perror(NULL, title, "Unable to allocate memory", _get_errno());
		endpwent();
		return 0;
	}
	
	setpwent();
	for (i = 0; i < cnt; i++)
	{
		_set_errno(0);
		pw = getpwent();
		if (!pw)
		{
			if (_get_errno())
			{
				msgbox_perror(NULL, title, "Unable to read user list", _get_errno());
				endpwent();
				free(user);
				return 0;
			}
			cnt = i;
			break;
		}
		strcpy(user[i].name, pw->pw_name);
		user[i].uid = pw->pw_uid;
	}
	endpwent();
	qsort(user, cnt, sizeof *user, user_cmp);
	
	f = form_load("/lib/forms/list.frm");
	if (!f)
	{
		wb(WB_ERROR);
		free(user);
		return 0;
	}
	form_set_title(f, title);
	
	li = gadget_find(f, "list");
	for (i = 0; i < cnt; i++)
	{
		list_newitem(li, user[i].name);
		if (user[i].uid == *uid)
			list_set_index(li, i);
	}
	
ask:
	form_set_dialog(pf, f);
	if (form_wait(f) == RESULT_OK)
	{
		i = list_get_index(li);
		if (i < 0 || i >= cnt)
		{
			form_lock(f);
			msgbox(f, title, "Please select an user.");
			form_unlock(f);
			goto ask;
		}
		
		*uid = user[i].uid;
		form_set_dialog(pf, NULL);
		form_close(f);
		free(user);
		return 1;
	}
	form_set_dialog(pf, NULL);
	form_close(f);
	free(user);
	return 0;
}

int dlg_gid(struct form *pf, const char *title, gid_t *gid)
{
	struct gs *group;
	struct group *gr;
	struct gadget *li;
	struct form *f;
	char msg[256];
	int cnt;
	int i;
	
	setgrent();
	for (cnt = 0; _set_errno(0), gr = getgrent(), gr; cnt++);
	if (_get_errno())
	{
		msgbox_perror(NULL, title, "Unable to read group list", _get_errno());
		endgrent();
		return 0;
	}
	group = calloc(cnt, sizeof *group);
	if (!group)
	{
		msgbox_perror(NULL, title, "Unable to allocate memory", _get_errno());
		endgrent();
		return 0;
	}
	
	setgrent();
	for (i = 0; i < cnt; i++)
	{
		_set_errno(0);
		gr = getgrent();
		if (!gr)
		{
			if (_get_errno())
			{
				msgbox_perror(NULL, title, "Unable to read group list", _get_errno());
				endgrent();
				free(group);
				return 0;
			}
			cnt = i;
			break;
		}
		strcpy(group[i].name, gr->gr_name);
		group[i].gid = gr->gr_gid;
	}
	endgrent();
	qsort(group, cnt, sizeof *group, group_cmp);
	
	f = form_load("/lib/forms/list.frm");
	if (!f)
	{
		wb(WB_ERROR);
		free(group);
		return 0;
	}
	form_set_title(f, title);
	
	li = gadget_find(f, "list");
	for (i = 0; i < cnt; i++)
	{
		list_newitem(li, group[i].name);
		if (group[i].gid == *gid)
			list_set_index(li, i);
	}
	
ask:
	form_set_dialog(pf, f);
	if (form_wait(f) == RESULT_OK)
	{
		i = list_get_index(li);
		if (i < 0 || i >= cnt)
		{
			form_lock(f);
			msgbox(f, title, "Please select a group.");
			form_unlock(f);
			goto ask;
		}
		
		*gid = group[i].gid;
		form_set_dialog(pf, NULL);
		form_close(f);
		free(group);
		return 1;
	}
	form_set_dialog(pf, NULL);
	form_close(f);
	free(group);
	return 0;
}

int dlg_newpass(struct form *pf, char *pass, int maxlen)
{
	struct gadget *pi;
	struct gadget *ci;
	struct gadget *g;
	struct form *f;
	
	f = form_load("/lib/forms/chpass.frm");
	if (!f)
	{
		wb(WB_ERROR);
		return 0;
	}
	
	pi = gadget_find(f, "passwd1");
	ci = gadget_find(f, "passwd2");
	
restart:
	form_set_dialog(pf, f);
	if (form_wait(f) != 1)
	{
		form_set_dialog(pf, NULL);
		form_close(f);
		return 0;
	}
	form_set_dialog(pf, NULL);
	
	if (strcmp(pi->text, ci->text))
	{
		form_lock(f);
		msgbox(f, "Change Password", "The passwords do not match.\n\nPlease try again.");
		form_unlock(f);
		input_set_text(pi, "");
		input_set_text(ci, "");
		goto restart;
	}
	
	if (strlen(pi->text) >= maxlen)
	{
		form_lock(f);
		msgbox(f, "Change Password", "The password is too long.\n\nPlease try again.");
		form_unlock(f);
		input_set_text(pi, "");
		input_set_text(ci, "");
		goto restart;
	}
	
	strcpy(pass, pi->text);
	form_close(f);
	return 1;
}
