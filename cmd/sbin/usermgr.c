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

#define _LIB_INTERNALS // for DIR internals in <dirent.h>

#include <priv/copyright.h>
#include <wingui_msgbox.h>
#include <wingui_color.h>
#include <wingui_form.h>
#include <wingui_menu.h>
#include <wingui_dlg.h>
#include <sys/stat.h>
#include <newtask.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <dirent.h>
#include <confdb.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <paths.h>
#include <pwd.h>
#include <grp.h>
#include <err.h>

#define SKEL	"/etc/skel"

void copy_dir(char *d, char *s, uid_t uid, gid_t gid);
void rmv_dir(char *p);
void newusr_click();
void delusr_click();
void chpwd_click();
void edusr_click();
void newgrp_click();
void delgrp_click();
int  main_form_close(struct form *f);
void create_form();
void load_info();

struct form *main_form;
struct gadget *usr_list;
struct gadget *grp_list;

void copy_dir(char *d, char *s, uid_t uid, gid_t gid)
{
	struct dirent *de;
	struct stat st;
	DIR *dir;
	
	dir = opendir(s);
	if (!dir)
	{
		msgbox(main_form, s, strerror(errno));
		return;
	}
	fstat(dir->fd, &st);
	if (mkdir(d, st.st_mode))
	{
		msgbox(main_form, d, strerror(errno));
		closedir(dir);
		return;
	}
	chown(d, uid, gid);
	while ((de = readdir(dir)))
	{
		char s_path[PATH_MAX];
		char d_path[PATH_MAX];
		
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;
		win_idle();
		
		sprintf(s_path, "%s/%s", s, de->d_name);
		sprintf(d_path, "%s/%s", d, de->d_name);
		if (stat(s_path, &st))
		{
			msgbox(main_form, s_path, strerror(errno));
			continue;
		}
		
		if (S_ISDIR(st.st_mode))
			copy_dir(d_path, s_path, uid, gid);
		else
		{
			char buf[32768];
			int s_fd;
			int d_fd;
			int cnt;
			
			s_fd = open(s_path, O_RDONLY);
			if (s_fd < 0)
			{
				msgbox(main_form, s_path, strerror(errno));
				continue;
			}
			d_fd = open(d_path, O_WRONLY|O_CREAT|O_EXCL, st.st_mode);
			if (d_fd<0)
			{
				msgbox(main_form, d_path, strerror(errno));
				close(s_fd);
				continue;
			}
			
			while ((cnt = read(s_fd, buf, sizeof buf)))
			{
				if (cnt < 0)
				{
					msgbox(main_form, s_path, strerror(errno));
					close(s_fd);
					close(d_fd);
					continue;
				}
				if (write(d_fd, buf, cnt) != cnt)
				{
					msgbox(main_form, d_path, strerror(errno));
					close(s_fd);
					close(d_fd);
					continue;
				}
				win_idle();
			}
			close(s_fd);
			close(d_fd);
			chown(d_path, uid, gid);
		}
	}
	closedir(dir);
}

void rmv_dir(char *p)
{
	if (_newtaskl(_PATH_B_FILEMGR_SLAVE, _PATH_B_FILEMGR_SLAVE, "del", p, (void *)NULL) < 0)
		msgbox_perror(main_form, "User Manager", "Cannot start filemgr slave", errno);
}

void newusr_click()
{
	struct passwd *pw;
	char name[LOGNAME_MAX + 1];
	char home[PATH_MAX];
	uid_t uid = 1000;
	
restart:
	setpwent();
	while ((pw = getpwent()))
		if (pw->pw_uid == uid)
		{
			uid++;
			goto restart;
		}
	endpwent();
	
	*name = 0;
	if (dlg_input5(main_form, "New user name:", name, sizeof name, DLG_INPUT_ALLOW_CANCEL) != 1)
		return;
	
	if (!*name)
		return;
	
	if (_newuser(name, uid, 1000))
		msgbox_perror(main_form, "User Manager", "Could not create the new account", errno);
	else
	{
		sprintf(home, "/home/%s", name);
		while (!access(home, 0))
			strcat(home, "_");
		
		_setshell(name, _PATH_B_DEFSHELL);
		_sethome(name, home);
		copy_dir(home, SKEL, uid, 1000);
	}
	load_info();
}

void delusr_click()
{
	const char *n;
	char msg[256];
	int i;
	
	i = list_get_index(usr_list);
	if (i < 0)
		return;
	n = list_get_text(usr_list, i);
	if (msgbox_ask(main_form, "Delete Account", "Are you sure you want to delete selected account (including home dir)?") == MSGBOX_YES)
	{
		struct passwd *pw;
		
		pw = getpwnam(n);
		if (!pw)
		{
			form_lock(main_form);
			sprintf(msg, "Could not delete user:\n\n%m");
			msgbox(main_form, "User Manager", msg);
			form_unlock(main_form);
			return;
		}
		rmv_dir(pw->pw_dir);
		endpwent();
		
		if (_deluser(n))
		{
			form_lock(main_form);
			sprintf(msg, "Could not delete user:\n\n%m");
			msgbox(main_form, "User Manager", msg);
			form_unlock(main_form);
		}
	}
	load_info();
}

void chpwd_click()
{
	const char *n;
	char msg[256];
	char pass[64];
	int i;
	
	i = list_get_index(usr_list);
	if (i<0)
		return;
	n = list_get_text(usr_list, i);
	if (dlg_newpass(main_form, pass, sizeof pass) && _setpass(n, pass))
	{
		sprintf(msg, "Could not change password:\n\n%m");
		msgbox(main_form, "User Manager", msg);
	}
}

void edusr_click()
{
	struct passwd *pw;
	struct group *gr;
	struct gadget *ni;
	struct gadget *hi;
	struct gadget *si;
	struct gadget *gr_label;
	struct form *f;
	const char *n;
	int res;
	int i;
	
	i = list_get_index(usr_list);
	if (i<0)
		return;
	n = list_get_text(usr_list, i);
	pw=getpwnam(n);
	if (!pw)
	{
		form_lock(main_form);
		msgbox(main_form, n, "This account is no longer valid.");
		form_unlock(main_form);
	}
	gr=getgrgid(pw->pw_gid);
	
	f = form_load("/lib/forms/user.frm");
	
	ni = gadget_find(f, "name");
	hi = gadget_find(f, "home");
	si = gadget_find(f, "shell");
	gr_label = gadget_find(f, "group");
	
	if (gr)
		label_set_text(gr_label, gr->gr_name);
	else
		label_set_text(gr_label, "INVALID");
	
	input_set_text(ni, n);
	input_set_text(hi, pw->pw_dir);
	input_set_text(si, pw->pw_shell);
	
	form_set_dialog(main_form, f);
	do
	{
		res = form_wait(f);
		if (res == 3 && dlg_gid(f, "Set Group", &pw->pw_gid))
		{
			_setgroup(n, pw->pw_gid);
			gr = getgrgid(pw->pw_gid);
			if (gr)
				label_set_text(gr_label, gr->gr_name);
			else
				label_set_text(gr_label, "INVALID");
		}
	} while (res != 1 && res != 2);
	form_set_dialog(main_form, NULL);
	
	if (res == 1)
	{
		if (strcmp(hi->text, pw->pw_dir))
			_sethome(n, hi->text);
		if (strcmp(si->text, pw->pw_shell))
			_setshell(n, si->text);
	}
	
	form_close(f);
	endpwent();
	endgrent();
}

void newgrp_click()
{
	struct group *gr;
	char name[LOGNAME_MAX + 1];
	char msg[256];
	gid_t gid = 1000;
	
restart:
	setgrent();
	while ((gr = getgrent()))
		if (gr->gr_gid == gid)
		{
			gid++;
			goto restart;
		}
	endgrent();
	
	*name = 0;
	if (dlg_input5(main_form, "New group name:", name, sizeof name, DLG_INPUT_ALLOW_CANCEL) != 1)
		return;
	
	if (!*name)
		return;
	
	if (_newgroup(name, gid))
	{
		sprintf(msg, "Could not create the new user group:\n\n%m");
		msgbox(main_form, "User Manager", msg);
	}
	load_info();
}

void delgrp_click()
{
	const char *n;
	char msg[256];
	int i;
	
	i = list_get_index(grp_list);
	if (i<0)
		return;
	n = list_get_text(grp_list, i);
	if (msgbox_ask(main_form, "Delete Group", "Are you sure you want to delete the selected "
	    "group?") == MSGBOX_YES && _delgroup(n))
	{
		form_lock(main_form);
		sprintf(msg, "Could not delete group:\n\n%m");
		msgbox(main_form, "User Manager", msg);
		form_unlock(main_form);
	}
	load_info();
}

static int set_splash(void)
{
	char pathname[PATH_MAX];
	char theme[NAME_MAX + 1];
	char buf[256];
	char *p;
	FILE *f;
	
	if (unlink("/lib/splash/splash.pnm") && errno != ENOENT)
		return -1;
	
	if (c_load("/user/w_theme", theme, sizeof theme))
		return -1;
	sprintf(pathname, "/lib/w_themes/%s/info", theme);
	
	f = fopen(pathname, "r");
	if (f == NULL)
		return 0;
	
	while (fgets(buf, sizeof buf, f))
	{
		p = strchr(buf, '\n');
		if (p)
			*p = 0;
		
		if (*buf == 'S')
			if (link(buf + 1, "/lib/splash/splash.pnm"))
				goto fail;
	}
	
	fclose(f);
	return 0;
fail:
	fclose(f);
	return -1;
}

static void copy_settings(int silent)
{
	static char *names[] = { "form", "metrics", "ptr_conf", "w_colors", "w_theme", "vtty-color" };
	static char buf[32768];
	
	char spathname[PATH_MAX];
	char dpathname[PATH_MAX];
	ssize_t cnt;
	int sfd, dfd;
	int fail = 0;
	int i;
	
	mkdir("/etc/syshome", 0755);
	mkdir("/etc/syshome/.conf", 0755);
	
	for (i = 0; i < sizeof names / sizeof *names; i++)
	{
		sprintf(spathname, "%s/.conf/%s", getenv("HOME"), names[i]);
		sprintf(dpathname, "/etc/syshome/.conf/%s", names[i]);
		
		sfd = open(spathname, O_RDONLY);
		if (sfd < 0)
		{
			if (errno != ENOENT)
				fail = 1;
			continue;
		}
		
		dfd = open(dpathname, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (dfd < 0)
		{
			warn("%s: open", dpathname);
			close(sfd);
			fail = 1;
			continue;
		}
		
		while (cnt = read(sfd, buf, sizeof buf), cnt > 0)
			if (write(dfd, buf, cnt) != cnt)
			{
				warn("%s: write", dpathname);
				fail = 1;
				break;
			}
		if (cnt < 0)
		{
			warn("%s: read", spathname);
			fail = 1;
		}
		
		close(sfd);
		close(dfd);
	}
	
	if (set_splash())
		fail = 1;
	win_update();
	
	if (fail)
		msgbox(main_form, "User Manager", "Failed to copy some settings.");
	else if (!silent)
		msgbox(main_form, "User Manager", "Finished applying the settings.");
}

static void appearance_click(struct menu_item *mi)
{
	if (msgbox_ask(main_form, "User Manager", "Do you want to apply current appearance settings to the login screen?") != MSGBOX_YES)
		return;
	
	copy_settings(0);
}

static void autologin_click(struct menu_item *mi)
{
	char username[LOGNAME_MAX + 1];
	char pathname[PATH_MAX];
	struct gadget *l_name;
	struct form *form;
	struct passwd *pw;
	uid_t uid;
	char *dd;
	char *p;
	FILE *f;
	int r;
	
	dd = getenv("DESKTOP_DIR");
	if (dd == NULL)
	{
		msgbox(main_form, "User Manager", "Cannot determine desktop directory.");
		return;
	}
	
	strcpy(pathname, dd);
	strcat(pathname, "/autologin");
	
	*username = 0;
	
	f = fopen(pathname, "r");
	if (f)
	{
		if (fgets(username, sizeof username, f))
		{
			p = strchr(username, '\n');
			if (p)
				*p = 0;
		}
		else
			*username = 0;
		fclose(f);
	}
	
	form = form_load("/lib/forms/autologin.frm");
	l_name = gadget_find(form, "l_user");
	
	if (*username)
		label_set_text(l_name, username);
	else
		label_set_text(l_name, "(not set)");
	
	form_set_dialog(main_form, form);
	do
	{
		r = form_wait(form);
		switch (r)
		{
		case 1: // Change
			uid = 0;
			if (*username)
			{
				pw = getpwnam(username);
				if (pw)
					uid = pw->pw_uid;
			}
			if (dlg_uid(form, "Autologin", &uid))
			{
				pw = getpwuid(uid);
				if (pw)
				{
					strcpy(username, pw->pw_name);
					label_set_text(l_name, username);
				}
			}
			break;
		case 2: // OK
			f = fopen(pathname, "w");
			if (f)
			{
				fputs(username, f);
				fclose(f);
			}
		case 3: // Cancel
			break;
		case 4: // Reset
			label_set_text(l_name, "(not set)");
			*username = 0;
			break;
		}
	} while (r != 2 && r != 3);
	
	form_set_dialog(main_form, NULL);
	form_close(form);
}

static void about_click(struct menu_item *mi)
{
	dlg_about7(main_form, NULL, "User Account Manager v1.2", SYS_PRODUCT, SYS_AUTHOR, SYS_CONTACT, "users.pnm");
}

static void usr_draw_item(struct gadget *g, int i, int wd, int x, int y, int w, int h)
{
	struct passwd *pw;
	struct group *gr;
	win_color fg, bg;
	const char *name;
	int cw, ch;
	
	win_chr_size(WIN_FONT_DEFAULT, &cw, &ch, 'X');
	
	if (list_get_index(g) == i)
	{
		fg = wc_get(WC_SEL_FG);
		bg = wc_get(WC_SEL_BG);
	}
	else
	{
		fg = wc_get(WC_FG);
		bg = wc_get(WC_BG);
	}
	
	name = list_get_text(g, i);
	pw   = getpwnam(name);
	
	win_rect(wd, bg, x, y, w, h);
	win_text(wd, fg, x + 1, y, name);
	
	if (pw == NULL)
		return;
	
	win_text(wd, fg, x + 1 + 19 * cw, y, pw->pw_shell);
	win_text(wd, fg, x + 1 + 29 * cw, y, pw->pw_dir);
	
	gr = getgrgid(pw->pw_gid);
	if (gr != NULL)
		win_text(wd, fg, x + 1 + 9 * cw, y, gr->gr_name);
}

static void main_form_resize(struct form *f, int w, int h)
{
	int tw, th;
	int gw, gh;
	
	win_chr_size(WIN_FONT_DEFAULT, &tw, &th, 'X');
	
	gh = ((h - th - 6) >> 1) - 2;
	gw =   w - 4;
	gadget_resize(usr_list, gw, gh);
	gadget_resize(grp_list, gw, h - gh - th - 8);
	gadget_move(grp_list, 2, gh + th + 6);
}

int main_form_close(struct form *f)
{
	struct form_state fst;
	
	form_get_state(f, &fst);
	c_save("usermgr", &fst, sizeof fst);
	exit(0);
}

void create_form(void)
{
	struct form_state fst;
	struct menu *um;
	struct menu *gm;
	struct menu *om;
	struct menu *m;
	int tw, th;
	
	win_chr_size(WIN_FONT_DEFAULT, &tw, &th, 'X');
	
	um = menu_creat();
	menu_newitem(um, "Create ...", newusr_click);
	menu_newitem(um, "Delete ...", delusr_click);
	menu_newitem(um, "Change password ...", chpwd_click);
	menu_newitem(um, "Edit ...", edusr_click);
	
	gm = menu_creat();
	menu_newitem(gm, "Create ...", newgrp_click);
	menu_newitem(gm, "Delete ...", delgrp_click);
	
	om = menu_creat();
	menu_newitem(om, "Appearance ...", appearance_click);
	menu_newitem(om, "Autologin ...", autologin_click);
	menu_newitem(om, "-", NULL);
	menu_newitem(om, "About", about_click);
	
	m = menu_creat();
	menu_submenu(m, "User", um);
	menu_submenu(m, "Group", gm);
	menu_submenu(m, "Options", om);
	
	main_form = form_creat(FORM_APPFLAGS, 0, -1, -1, 400, 198, "User Account Manager");
	form_min_size(main_form, tw * 40, th * 8);
	form_set_menu(main_form, m);
	
	label_creat(main_form, 5	  , 2, "Name");
	label_creat(main_form, 5 +  9 * tw, 2, "Group");
	label_creat(main_form, 5 + 19 * tw, 2, "Shell");
	label_creat(main_form, 5 + 29 * tw, 2, "Home");
	
	usr_list = list_creat(main_form, 2, th + 4,  396, 83);
	grp_list = list_creat(main_form, 2, 100, 396, 96);
	list_set_flags(usr_list, LIST_FRAME | LIST_VSBAR);
	list_set_flags(grp_list, LIST_FRAME | LIST_VSBAR);
	list_set_draw_cb(usr_list, usr_draw_item);
	list_on_request(usr_list, edusr_click);
	
	form_on_resize(main_form, main_form_resize);
	form_on_close(main_form, main_form_close);
	
	if (!c_load("usermgr", &fst, sizeof fst))
		form_set_state(main_form, &fst);
	form_show(main_form);
}

void load_info(void)
{
	struct passwd *pw;
	struct group *gr;
	
	list_clear(usr_list);
	list_clear(grp_list);
	
	setpwent();
	while ((pw = getpwent()))
		list_newitem(usr_list, strdup(pw->pw_name)); /* leak */
	endpwent();
	
	setgrent();
	while ((gr = getgrent()))
		list_newitem(grp_list, strdup(gr->gr_name)); /* leak */
	endgrent();
	
	list_sort(usr_list);
	list_sort(grp_list);
}

int main(int argc, char **argv)
{
	if (win_attach())
		err(255, NULL);
	
	if (geteuid())
	{
		msgbox(NULL, "User Account Manager", "You are not root.");
		return 0;
	}
	
	if (getenv("OSDEMO"))
	{
		msgbox(NULL, "User Account Manager",
			"User Account Manager is not available in the live CD/USB mode.\n\n"
			"Install the operating system to manage user accounts.");
		return 0;
	}
	
	if (argc == 2 && !strcmp(argv[1], "-A"))
	{
		copy_settings(1);
		return 0;
	}
	
	create_form();
	load_info();
	for (;;)
		win_wait();
}
