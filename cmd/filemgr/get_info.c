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
#include <wingui_form.h>
#include <wingui_dlg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fmthuman.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <event.h>
#include <stdio.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

#include "filemgr.h"

#define PERM_FORM	"/lib/forms/perm_info.frm"
#define STAT_FORM	"/lib/forms/file_info.frm"

static void perm_info(struct form *lf, const char *name)
{
	struct form *form;
	
	struct gadget *s_irusr_chkbox;
	struct gadget *s_iwusr_chkbox;
	struct gadget *s_ixusr_chkbox;
	
	struct gadget *s_irgrp_chkbox;
	struct gadget *s_iwgrp_chkbox;
	struct gadget *s_ixgrp_chkbox;
	
	struct gadget *s_iroth_chkbox;
	struct gadget *s_iwoth_chkbox;
	struct gadget *s_ixoth_chkbox;
	
	struct gadget *s_isuid_chkbox;
	struct gadget *s_isgid_chkbox;
	
	struct stat st;
	
	if (stat(name, &st))
	{
		char msg[256 + PATH_MAX];
		
		sprintf(msg, "Cannot stat '%s':\n\n%m", name);
		msgbox(NULL, "Get Info", msg);
		return;
	}
	
	form = form_load(PERM_FORM);
	
	s_irusr_chkbox = gadget_find(form, "irusr");
	s_iwusr_chkbox = gadget_find(form, "iwusr");
	s_ixusr_chkbox = gadget_find(form, "ixusr");
	
	s_irgrp_chkbox = gadget_find(form, "irgrp");
	s_iwgrp_chkbox = gadget_find(form, "iwgrp");
	s_ixgrp_chkbox = gadget_find(form, "ixgrp");
	
	s_iroth_chkbox = gadget_find(form, "iroth");
	s_iwoth_chkbox = gadget_find(form, "iwoth");
	s_ixoth_chkbox = gadget_find(form, "ixoth");
	
	s_isuid_chkbox = gadget_find(form, "isuid");
	s_isgid_chkbox = gadget_find(form, "isgid");
	
	chkbox_set_state(s_irusr_chkbox, st.st_mode & S_IRUSR);
	chkbox_set_state(s_iwusr_chkbox, st.st_mode & S_IWUSR);
	chkbox_set_state(s_ixusr_chkbox, st.st_mode & S_IXUSR);
	
	chkbox_set_state(s_irgrp_chkbox, st.st_mode & S_IRGRP);
	chkbox_set_state(s_iwgrp_chkbox, st.st_mode & S_IWGRP);
	chkbox_set_state(s_ixgrp_chkbox, st.st_mode & S_IXGRP);
	
	chkbox_set_state(s_iroth_chkbox, st.st_mode & S_IROTH);
	chkbox_set_state(s_iwoth_chkbox, st.st_mode & S_IWOTH);
	chkbox_set_state(s_ixoth_chkbox, st.st_mode & S_IXOTH);
	
	chkbox_set_state(s_isuid_chkbox, st.st_mode & S_ISUID);
	chkbox_set_state(s_isgid_chkbox, st.st_mode & S_ISGID);
	
	form_set_dialog(lf, form);
	if (form_wait(form) == 1)
	{
		char msg[256 + PATH_MAX];
		mode_t newmode;
		
		newmode = st.st_mode & ~S_IRIGHTS;
		
		if (chkbox_get_state(s_irusr_chkbox)) newmode |= S_IRUSR;
		if (chkbox_get_state(s_iwusr_chkbox)) newmode |= S_IWUSR;
		if (chkbox_get_state(s_ixusr_chkbox)) newmode |= S_IXUSR;
		
		if (chkbox_get_state(s_irgrp_chkbox)) newmode |= S_IRGRP;
		if (chkbox_get_state(s_iwgrp_chkbox)) newmode |= S_IWGRP;
		if (chkbox_get_state(s_ixgrp_chkbox)) newmode |= S_IXGRP;
		
		if (chkbox_get_state(s_iroth_chkbox)) newmode |= S_IROTH;
		if (chkbox_get_state(s_iwoth_chkbox)) newmode |= S_IWOTH;
		if (chkbox_get_state(s_ixoth_chkbox)) newmode |= S_IXOTH;
		
		if (chkbox_get_state(s_isuid_chkbox)) newmode |= S_ISUID;
		if (chkbox_get_state(s_isgid_chkbox)) newmode |= S_ISGID;
		
		if (newmode != st.st_mode)
		{
			if (chmod(name, newmode))
			{
				sprintf(msg, "Cannot chmod '%s' to 0%05o:\n\n%m", name, newmode);
				msgbox(form, "File Manager", msg);
			}
		}
	}
	form_set_dialog(lf, NULL);
	
	form_close(form);
}

void mkmstr(char *buf, mode_t mode)
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

void mkostr(char *ubuf, char *gbuf, uid_t uid, gid_t gid)
{
	struct passwd *pw;
	struct group *gr;
	
	pw = getpwuid(uid);
	gr = getgrgid(gid);
	if (pw)
		strcpy(ubuf, pw->pw_name);
	else
		sprintf(ubuf, "(uid = %i)", uid);
	if (gr)
		strcpy(gbuf, gr->gr_name);
	else
		sprintf(gbuf, "(gid = %i)", gid);
	endpwent();
	endgrent();
}

char *mktstr(const time_t *t)
{
	static char buf[64];
	
	struct tm *tm;
	char *r, *p;
	
	tm = localtime(t);
	if (!tm)
	{
		sprintf(buf, "0x%llx", (long long)*t);
		return buf;
	}
	
	r = asctime(tm);
	p = strchr(r, '\n');
	if (p)
		*p = 0;
	return r;
}

void get_info(struct form *pf, const char *pathname)
{
	struct gadget *icon_pict;
	struct gadget *name_label;
	struct gadget *name_input;
	struct gadget *size_label;
	struct gadget *mode_label;
	struct gadget *ino_label;
	struct gadget *nlink_label;
	struct gadget *uidv_label;
	struct gadget *gidv_label;
	struct gadget *atime_label;
	struct gadget *ctime_label;
	struct gadget *mtime_label;
	struct gadget *uid_button;
	struct gadget *gid_button;
	struct form *form;
	char uname[LOGNAME_MAX + 1];
	char gname[LOGNAME_MAX + 1];
	char msg[256 + PATH_MAX];
	char size_str[256];
	char mode_str[256];
	char ino_str[256];
	char nlink_str[256];
	char atime_str[256];
	char ctime_str[256];
	char mtime_str[256];
	struct stat st;
	struct file_type ft;
	int no_rename = 0;
	int reload = 0;
	int result;
	char *p;
	
	if (!strcmp(pathname, ".") || !strcmp(pathname, ".."))
		no_rename = 1;
	
	if (stat(pathname, &st))
	{
		sprintf(msg, "Cannot stat \"%s\":\n\n%m", pathname);
		msgbox(NULL, "File Manager", msg);
		return;
	}
	sprintf(size_str, "File size: %s (%i blocks)", fmthumanoff(st.st_size, 1), st.st_blocks);
	sprintf(ino_str, "Index: %i", st.st_ino);
	sprintf(nlink_str, "Links: %i", st.st_nlink);
	strcpy(mode_str, "File mode: ");
	mkmstr(mode_str + 11, st.st_mode);
	
	mkostr(uname, gname, st.st_uid, st.st_gid);
	
	strcpy(atime_str, mktstr(&st.st_atime));
	strcpy(ctime_str, mktstr(&st.st_ctime));
	strcpy(mtime_str, mktstr(&st.st_mtime));
	
	form = form_load(STAT_FORM);
	
	icon_pict   = gadget_find(form, "icon");
	
	name_label  = gadget_find(form, "name_label");
	name_input  = gadget_find(form, "name_input");
	
	size_label  = gadget_find(form, "size");
	mode_label  = gadget_find(form, "mode");
	ino_label   = gadget_find(form, "ino");
	nlink_label = gadget_find(form, "nlink");
	
	uidv_label  = gadget_find(form, "uid_label");
	gidv_label  = gadget_find(form, "gid_label");
	
	uid_button  = gadget_find(form, "uid_btn");
	gid_button  = gadget_find(form, "gid_btn");
	
	atime_label = gadget_find(form, "atime");
	ctime_label = gadget_find(form, "ctime");
	mtime_label = gadget_find(form, "mtime");
	
	if (!ft_getinfo(&ft, pathname))
	{
		int isz;
		
		isz  = (name_input->rect.y - icon_pict->rect.y) << 1;
		isz +=  name_input->rect.h;
		
		pict_load(icon_pict, ft.icon);
		pict_scale(icon_pict, isz, isz);
	}
	
	if (no_rename)
	{
		gadget_remove(name_input);
		
		if (!strcmp(pathname, "."))
			label_set_text(name_label, "(current directory)");
		else if (!strcmp(pathname, ".."))
			label_set_text(name_label, "(parent directory)");
		else
			label_set_text(name_label, pathname);
	}
	else
	{
		gadget_remove(name_label);
		input_set_text(name_input, pathname);
	}
	
	if (geteuid())
	{
		gadget_remove(uid_button);
		gadget_remove(gid_button);
	}
	
	label_set_text(atime_label, atime_str);
	label_set_text(ctime_label, ctime_str);
	label_set_text(mtime_label, mtime_str);
	label_set_text(size_label, size_str);
	label_set_text(mode_label, mode_str);
	label_set_text(ino_label, ino_str);
	label_set_text(nlink_label, nlink_str);
	label_set_text(gidv_label, gname);
	label_set_text(uidv_label, uname);
	
	form_set_dialog(pf, form);
	for (;;)
	{
		result = form_wait(form);
		
		switch (result)
		{
		case 1:
		case 2:
			goto fini;
		case 3:
			form_lock(form);
			perm_info(form, pathname);
			if (!stat(pathname, &st))
			{
				strcpy(ctime_str, mktstr(&st.st_ctime));
				mkmstr(mode_str + 11, st.st_mode);
				
				label_set_text(ctime_label, ctime_str);
				label_set_text(mode_label, mode_str);
			}
			form_unlock(form);
			break;
		case 4:
			if (dlg_uid(form, "Change owner", &st.st_uid) && chown(pathname, st.st_uid, st.st_gid))
			{
				sprintf(msg, "Cannot chown \"%s\"", pathname);
				msgbox_perror(form, "Change owner", msg, errno);
			}
			if (!stat(pathname, &st))
			{
				mkostr(uname, gname, st.st_uid, st.st_gid);
				strcpy(ctime_str, mktstr(&st.st_ctime));
				
				label_set_text(ctime_label, ctime_str);
				label_set_text(uidv_label, uname);
				label_set_text(gidv_label, gname);
			}
			break;
		case 5:
			if (dlg_gid(form, "Change group", &st.st_gid) && chown(pathname, st.st_uid, st.st_gid))
			{
				sprintf(msg, "Cannot chown \"%s\"", pathname);
				msgbox_perror(form, "Change group", msg, errno);
			}
			if (!stat(pathname, &st))
			{
				mkostr(uname, gname, st.st_uid, st.st_gid);
				strcpy(ctime_str, mktstr(&st.st_ctime));
				
				label_set_text(ctime_label, ctime_str);
				label_set_text(uidv_label, uname);
				label_set_text(gidv_label, gname);
			}
			break;
		}	
	}
fini:
	form_set_dialog(pf, NULL);
	
	if (!no_rename && result == 1)
	{
		char *newname = name_input->text;
		
		if (strcmp(pathname, newname))
		{
			if (rename(pathname, newname))
			{
				sprintf(msg, "Cannot rename \"%s\"", pathname);
				msgbox_perror(form, "File Manager", msg, errno);
			}
			else
				reload = 1;
		}
	}
	
	form_close(form);
	if (reload)
		load_dir();
}
