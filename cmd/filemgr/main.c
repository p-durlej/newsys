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

#define _LIB_INTERNALS

#include <config/defaults.h>

#include <priv/copyright.h>
#include <priv/filemgr.h>
#include <priv/wingui.h>

#include <wingui_cgadget.h>
#include <wingui_metrics.h>
#include <wingui_msgbox.h>
#include <wingui_bitmap.h>
#include <wingui_color.h>
#include <wingui_form.h>
#include <wingui_menu.h>
#include <wingui_dlg.h>
#include <wingui.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fmthuman.h>
#include <confdb.h>
#include <newtask.h>
#include <mkcanon.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <event.h>
#include <timer.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <os386.h>
#include <pwd.h>
#include <err.h>
#include <prefs/filemgr.h>

#include "filemgr.h"

struct pref_filemgr *config;

struct timer *	refresh_timer;
struct gadget *	main_icbox;
struct gadget *	main_sbar;
struct form *	main_form;
struct form *	bg_form;
struct gadget *	bg;
int		backdrop_tile;
void *		backdrop;

int		listfd = -1;
int		dflag;
int		osdemo;

char		dirname[NAME_MAX + 1];
char		cwd[PATH_MAX];
int		full_screen;
int		desktop;
int		list_mode;
int		show_menu = 1;
int		do_refresh;
int		logout;
int		do_save_pos;

#define MAX_SLAVES	1024

pid_t slaves[MAX_SLAVES];

struct file_info
{
	char	name[NAME_MAX + 1];
	char	icon[PATH_MAX];
	char	type_desc[64];
	struct	stat st;
	int	with_stat;
	char	owner[LOGNAME_MAX + 1];
	char	group[LOGNAME_MAX + 1];
};

struct file_info *file_info;
int file_cnt;

static void icbox_redraw_list_item(struct gadget *g, int i, int wd, int x, int y, int w, int h, int sel_only);
static void icbox_size_list_item(struct gadget *g, int i, int *w, int *h);

static void on_resize(void);

static void save_pos(int complain);
static void autosave_pos(void);
static void load_pos(void);

static void update_title(void)
{
	char title[sizeof dirname + sizeof cwd + 2];
	char *p;
	
	if (desktop)
	{
		if (full_screen)
			form_set_title(main_form, "Desktop");
		else
			form_set_title(main_form, "Session Manager");
		return;
	}
	
	if (listfd >= 0)
	{
		form_set_title(main_form, "Search Results");
		return;
	}
	
	if (config->show_path && *dirname)
	{
		sprintf(title, "%s [%s]", dirname, cwd);
		form_set_title(main_form, title);
		return;
	}
	
	if (config->show_path)
	{
		form_set_title(main_form, cwd);
		return;
	}
	
	if (*dirname)
	{
		form_set_title(main_form, dirname);
		return;
	}
	
	p = strrchr(cwd, '/');
	if (!p)
		p = cwd;
	p++;
	
	if (*p)
		form_set_title(main_form, p);
	else
		form_set_title(main_form, cwd);
}

void load_config(void)
{
	config = pref_filemgr_get();
}

void launch(const char *pathname, int follow_link)
{
	struct file_type ft;
	struct stat st;
	pid_t pid;
	
	form_busy(main_form);
	
	if (stat(pathname, &st))
	{
		char msg[64 + PATH_MAX];
		int err = errno;
		
		sprintf(msg, "Cannot stat \"%s\"", pathname);
		
		msgbox_perror(main_form, "File Manager", msg, err);
		goto fini;
	}
	
	if (S_ISREG(st.st_mode) && st.st_mode & 0111)
		pid = _newtaskl(pathname, pathname, (void *)NULL);
	else
	{
		if (ft_getinfo(&ft, pathname))
		{
			char msg[128 + PATH_MAX];
			int err = errno;
			
			sprintf(msg, "Type information of \"%s\" is not available", pathname);
			msgbox_perror(main_form, "File Manager", msg, err);
			goto fini;
		}
		
		if (follow_link && !strcmp(ft.type, "shlink"))
		{
			char msg[64 + PATH_MAX];
			char exec[PATH_MAX] = "";
			char wd[PATH_MAX] = "";
			char buf[PATH_MAX];
			char *nl;
			FILE *f;
			
			f = fopen(pathname, "r");
			if (!f)
			{
				int err = errno;
				
				sprintf(msg, "Cannot open \"%s\"", pathname);
				msgbox_perror(main_form, "File Manager", msg, err);
			}
			
			while (fgets(buf, sizeof buf, f))
			{
				nl = strchr(buf, '\n');
				if (nl)
					*nl = 0;
				
				if (strlen(buf) > 5 && !memcmp(buf, "exec=", 5))
					strcpy(exec, buf + 5);
				
				if (strlen(buf) > 3 && !memcmp(buf, "wd=", 3))
					strcpy(wd, buf + 3);
			}
			fclose(f);
			
			if (*exec)
			{
				if (*wd)
				{
					if (*wd == '$')
					{
						const char *home = getenv("HOME");
						
						if (!home)
							home = _PATH_H_DEFAULT;
						
						if (strlen(home) + strlen(wd) >= sizeof wd)
						{
							msgbox(main_form, "File Manager", "Path too long.");
							goto fini;
						}
						
						memmove(wd + strlen(home) - 1, wd, strlen(wd));
						memcpy(wd, home, strlen(home));
					}
					
					if (chdir(wd))
					{
						sprintf(msg, "Cannot change into \"%s\"", wd);
						msgbox_perror(main_form, "File Manager", msg, errno);
						goto fini;
					}
				}
				launch(exec, 0);
				
				while (*wd && _chdir(cwd, CHDIR_OBSERVE))
				{
					sprintf(msg, "Cannot change into \"%s\"", cwd);
					msgbox_perror(main_form, "File Manager", msg, errno);
					strcpy(cwd, "/");
					goto fini;
				}
				goto fini;
			}
			
			sprintf(msg, "Invalid shell link \"%s\".", pathname);
			msgbox(main_form, "File Manager", msg);
			goto fini;
		}
		
		if (!*ft.exec)
		{
			msgbox(main_form, pathname, "No handler installed for this file type.");
			goto fini;
		}
		
		pid = _newtaskl(ft.exec, ft.exec, pathname, (void *)NULL);
	}
	
	if (pid < 0)
		msgbox_perror(main_form, "File Manager", "Task creation failed", errno);
	
fini:
	form_unbusy(main_form);
}

void main_form_move(struct form *form, int x, int y)
{
}

void main_form_resize(struct form *form, int w, int h)
{
	int sbw = wm_get(WM_SCROLLBAR);
	int sw, sh;
	int pos;
	
	if (!full_screen)
	{
		config->form_w = w;
		config->form_h = h;
		
		gadget_resize(main_icbox, w - sbw, h);
		gadget_move(main_sbar, w - sbw, 0);
		gadget_resize(main_sbar, sbw, h);
		
		icbox_get_scroll_range(main_icbox, &sw, &sh);
		vsbar_set_limit(main_sbar, sh);
		pos = vsbar_get_pos(main_sbar);
		if (sh && pos >= sh)
			vsbar_set_pos(main_sbar, sh - 1);
	}
}

void main_icbox_select(struct gadget *g, int index, int button)
{
}

static int enter_dir(const char *pathname)
{
	char msg[128 + PATH_MAX];
	
	if (_chdir(pathname, CHDIR_OBSERVE))
	{
		int err = errno;
		
		sprintf(msg, "Could not change into directory \"%s\"", pathname);
		msgbox_perror(main_form, "File Manager", msg, err);
		
		errno = err;
		return -1;
	}
	
	getcwd(cwd, sizeof cwd);
	
	if (main_form != NULL)
	{
		update_title();
		load_dir();
	}
	
	do_save_pos = 0;
	return 0;
}

void main_icbox_request(struct gadget *g, int index, int button)
{
	char msg[128 + PATH_MAX];
	struct stat st;
	const char *name;
	
	if (index < 0)
		return;
	
	name = icbox_get_text(g, index);
	
	if (stat(name, &st))
	{
		sprintf(msg, "Cannot stat \"%s\":\n\n%m", name);
		msgbox(NULL, "File Manager", msg);
		return;
	}
	
	if (!desktop && S_ISDIR(st.st_mode) && listfd < 0)
		enter_dir(name);
	else
		launch(name, 1);
}

void main_icbox_scroll(struct gadget *g, int x, int y)
{
	if (!full_screen)
		vsbar_set_pos(main_sbar, y);
}

void main_sbar_move(struct gadget *g, int pos)
{
	icbox_scroll(main_icbox, 0, pos);
}

int cmp_file_info(const void *a, const void *b)
{
	const struct file_info *f1 = a;
	const struct file_info *f2 = b;
	int r;
	
	if (S_ISDIR(f1->st.st_mode) != S_ISDIR(f2->st.st_mode))
		return S_ISDIR(f1->st.st_mode) ? -1 : 1;
	
	if (config->sort_order == SORT_SIZE && f1->st.st_size != f2->st.st_size)
		return f1->st.st_size > f2->st.st_size ? 1 : -1;
	else if (config->sort_order == SORT_TYPE)
	{
		r = strcmp(f1->type_desc, f2->type_desc);
		if (r)
			return r;
	}
	
	return strcmp(f1->name, f2->name);
}

void load_list(void)
{
	char msg[128 + PATH_MAX];
	char buf[PATH_MAX + 1];
	struct file_type ft;
	FILE *lfile;
	int nd;
	char *p;
	int i;
	
	icbox_clear(main_icbox);
	form_lock(main_form);
	form_busy(NULL);
restart:
	free(file_info);
	file_info = NULL;
	file_cnt = 0;
	
	nd = dup(listfd);
	if (nd < 0)
	{
		msgbox_perror(main_form, "File Manager", "Could not dup listing fd", errno);
		form_unlock(main_form);
		form_unbusy(NULL);
		return;
	}
	
	lfile = fdopen(nd, "r");
	if (lfile == NULL)
	{
		msgbox_perror(main_form, "File Manager", "Could not fdopen listing file", errno);
		form_unlock(main_form);
		form_unbusy(NULL);
		return;
	}
	
	rewind(lfile);
	while (fgets(buf, sizeof buf, lfile))
		file_cnt++;
	
	file_info = calloc(sizeof *file_info, file_cnt);
	if (!file_info)
	{
		msgbox(main_form, "File Manager", "Memory exhausted");
		exit(errno);
	}
	
	rewind(lfile);
	i = 0;
	while (fgets(buf, sizeof buf, lfile))
	{
		p = strchr(buf, '\n');
		if (p != NULL)
			*p = 0;
		
		if (i >= file_cnt)
		{
			fclose(lfile);
			goto restart;
		}
		
		strcpy(file_info[i].name, buf);
		if (stat(buf, &file_info[i].st))
		{
			file_info[i].st.st_mode = S_IFREG;
			file_info[i].st.st_size = -1;
		}
		else
			file_info[i].with_stat = 1;
		
		if (ft_getinfo(&ft, file_info[i].name))
		{
			sprintf(msg, "Type information of \"%s\"\n"
				     "is not available:\n\n%m",
				file_info[i].name);
			
			msgbox(main_form, "File Manager", msg);
			continue;
		}
		else
		{
			strcpy(file_info[i].type_desc, ft.desc);
			strcpy(file_info[i].icon, ft.icon);
		}
		
		i++;
	}
	fclose(lfile);
	file_cnt = i;
	
	qsort(file_info, file_cnt, sizeof *file_info, cmp_file_info);
	for (i = 0; i < file_cnt; i++)
	{
		const char *pathname = file_info[i].name;
		const char *iconname = file_info[i].icon;
		
		if (list_mode)
			iconname = NULL;
		
		if (icbox_newitem(main_icbox, pathname, iconname))
			icbox_newitem(main_icbox, pathname, DEFAULT_ICON);
		
		icbox_set_dd_data(main_icbox, i, pathname, strlen(pathname) + 1);
	}
	
	icbox_scroll(main_icbox, 0, 0);
	if (!full_screen)
	{
		int w, h;
		
		icbox_get_scroll_range(main_icbox, &w, &h);
		vsbar_set_limit(main_sbar, h);
		vsbar_set_pos(main_sbar, 0);
	}
	form_unlock(main_form);
	form_unbusy(NULL);
}

static void v_mode_list(void)
{
	list_mode = 1;
	
	icbox_set_item_cb(main_icbox, icbox_redraw_list_item);
	icbox_set_size_cb(main_icbox, icbox_size_list_item);
}

static void v_mode_icons(void)
{
	list_mode = 0;
	
	icbox_set_item_cb(main_icbox, NULL);
	icbox_set_size_cb(main_icbox, NULL);
}

static void load_dirinfo(void)
{
	char buf[256];
	char *p;
	FILE *f;
	int ut = 0;
	int smul = 1;
	
	if (config->large_icons)
		smul = 2;
	
	if (*dirname)
	{
		*dirname = 0;
		ut = 1;
	}
	
	show_menu = 1;
	
	f = fopen(".dirinfo", "r");
	if (f != NULL)
	{
		while (fgets(buf, sizeof buf, f))
		{
			p = strchr(buf, '\n');
			if (p != NULL)
				*p = 0;
			
			switch (*buf)
			{
			case 'T':
				strncpy(dirname, buf + 1, sizeof dirname - 1);
				ut = 1;
				break;
			case 'V':
				if (!main_form)
					break;
				
				if (!strcmp(buf + 1, "list"))
					v_mode_list();
				else if (!strcmp(buf + 1, "icons"))
					v_mode_icons();
				break;
			case 'W':
				if (!main_form)
					config->form_w = atoi(buf + 1) * smul;
				break;
			case 'H':
				if (!main_form)
					config->form_h = atoi(buf + 1) * smul;
				break;
			case 'M':
				show_menu = atoi(buf + 1);
				break;
			default:
				;
			}
		}
		fclose(f);
	}
	
	if (ut && main_form)
		update_title();
}

void load_dir(void)
{
	char msg[128 + PATH_MAX];
	struct file_type ft;
	struct dirent *de;
	struct stat st;
	DIR *dir;
	int i;
	
	if (listfd >= 0)
	{
		load_list();
		return;
	}
	
	icbox_clear(main_icbox);
	form_lock(main_form);
	form_busy(NULL);
	
	load_dirinfo();
restart:
	free(file_info);
	file_info = NULL;
	file_cnt = 0;
	
	dir = opendir(".");
	if (!dir)
	{
		char msg[128 + PATH_MAX];
		int err = errno;
		
		sprintf(msg, "Could not open directory \"%s\"", cwd);
		
		msgbox_perror(main_form, "File Manager", msg, errno);
		form_unlock(main_form);
		form_unbusy(NULL);
		return;
	}
	
	while ((de = readdir(dir)))
	{
		if ((!config->show_dotfiles || desktop) && *de->d_name == '.')
			continue;
		file_cnt++;
	}
	file_info = calloc(sizeof *file_info, file_cnt);
	if (!file_info)
	{
		msgbox(main_form, "File Manager", "Memory Exhausted");
		exit(errno);
	}
	
	rewinddir(dir);
	i = 0;
	errno = 0;
	while (de = readdir(dir), de)
	{
		if ((!config->show_dotfiles || desktop) && *de->d_name == '.')
			continue;
		
		if (i >= file_cnt)
		{
			closedir(dir);
			goto restart;
		}
		
		strcpy(file_info[i].name, de->d_name);
		if (stat(de->d_name, &file_info[i].st))
		{
			file_info[i].st.st_mode = S_IFREG;
			file_info[i].st.st_size = -1;
		}
		else
			file_info[i].with_stat = 1;
		
		if (ft_getinfo(&ft, file_info[i].name))
		{
			sprintf(msg, "Type information of \"%s\"\n"
				     "is not available:\n\n%m",
				file_info[i].name);
			
			msgbox(main_form, "File Manager", msg);
			continue;
		}
		else
		{
			strcpy(file_info[i].type_desc, ft.desc);
			strcpy(file_info[i].icon, ft.icon);
		}
		
		i++;
	}
	closedir(dir);
	file_cnt = i;
	
	qsort(file_info, file_cnt, sizeof *file_info, cmp_file_info);
	for (i = 0; i < file_cnt; i++)
	{
		const char *iconname = file_info[i].icon;
		char pathname[PATH_MAX];
		size_t len;
		
		if (list_mode)
			iconname = NULL;
		
		if (icbox_newitem(main_icbox, file_info[i].name, iconname))
			icbox_newitem(main_icbox, file_info[i].name, DEFAULT_ICON);
		
		len = strlen(cwd);
		if (cwd[len - 1] == '/')
			sprintf(pathname, "%s%s", cwd, file_info[i].name);
		else
			sprintf(pathname, "%s/%s", cwd, file_info[i].name);
		
		icbox_set_dd_data(main_icbox, i, pathname, strlen(pathname) + 1);
	}
	
	icbox_scroll(main_icbox, 0, 0);
	if (!full_screen)
	{
		int w, h;
		
		icbox_get_scroll_range(main_icbox, &w, &h);
		vsbar_set_limit(main_sbar, h);
		vsbar_set_pos(main_sbar, 0);
	}
	
	form_unlock(main_form);
	form_unbusy(NULL);
}

void new_dir(struct menu_item *m)
{
	char pathname[PATH_MAX];
	
	*pathname = 0;
	if (dlg_input(main_form, "Create Directory", pathname, sizeof pathname))
	{
		if (!*pathname)
			return;
		
		if (mkdir(pathname, 0777))
		{
			char msg[256 + PATH_MAX];
			
			sprintf(msg, "Cannot create \"%s\":\n\n%m", pathname);
			msgbox(main_form, "MkDir", msg);
		}
		
		load_dir();
	}
}

void new_file(struct menu_item *m)
{
	char pathname[PATH_MAX];
	int fd;
	
	*pathname = 0;
	if (dlg_input(main_form, "Create File", pathname, sizeof pathname))
	{
		if (!*pathname)
			return;
		
		fd = open(pathname, O_CREAT | O_WRONLY | O_EXCL, 0666);
		if (fd < 0)
		{
			char msg[256 + PATH_MAX];
			
			sprintf(msg, "Cannot create \"%s\":\n\n%m", pathname);
			msgbox(main_form, "Create File", msg);
		}
		else
			close(fd);
		
		load_dir();
	}
}

void copy_file(struct menu_item *m)
{
	char newname[PATH_MAX];
	const char *name;
	pid_t pid;
	int i;
	
	i = icbox_get_index(main_icbox);
	if (i < 0)
	{
		msgbox(main_form, "Copy", "Please select a file");
		return;
	}
	name = icbox_get_text(main_icbox, i);
	
	strcpy(newname, name);
	
	if (dlg_input(main_form, "Copy", newname, sizeof newname))
	{
		if (!strcmp(newname, name))
			return;
		
		pid = _newtaskl(SLAVE, SLAVE, "copy", name, newname, NULL);
		if (pid < 1)
		{
			msgbox(main_form, "File Manager", "Could not run slave.");
			return;
		}
	}
	
	for (i = 0; i < MAX_SLAVES; i++)
		if (!slaves[i])
		{
			slaves[i] = pid;
			return;
		}
}

void rename_file(struct menu_item *m)
{
	char newname[PATH_MAX + 1];
	const char *name;
	pid_t pid;
	int i;
	
	i = icbox_get_index(main_icbox);
	if (i < 0)
	{
		msgbox(main_form, "Rename/Move", "Please select a file");
		return;
	}
	name = icbox_get_text(main_icbox, i);
	
	strcpy(newname, name);
	if (dlg_input(main_form, "Rename", newname, sizeof newname))
	{
		if (!strcmp(newname, name))
			return;
		
		pid = _newtaskl(SLAVE, SLAVE, "move", name, newname, NULL);
		if (pid < 1)
		{
			msgbox(main_form, "File Manager", "Could not run slave.");
			return;
		}
	}
	
	for (i = 0; i < MAX_SLAVES; i++)
		if (!slaves[i])
		{
			slaves[i] = pid;
			return;
		}
}

void delete_file(struct menu_item *m)
{
	char msg[128 + PATH_MAX];
	const char *name;
	pid_t pid;
	int i;
	
	i = icbox_get_index(main_icbox);
	if (i < 0)
		msgbox(main_form, "Delete", "Please select a file");
	else
	{
		name = icbox_get_text(main_icbox, i);
		
		sprintf(msg, "Are you sure you want to delete\n\"%s\"?", name);
		
		if (msgbox_ask(main_form, "File Manager", msg) == MSGBOX_YES)
		{
			pid = _newtaskl(SLAVE, SLAVE, "del", name, NULL);
			if (pid < 1)
			{
				msgbox(main_form, "File Manager", "Could not run slave.");
				return;
			}
		}
	}
	
	for (i = 0; i < MAX_SLAVES; i++)
		if (!slaves[i])
		{
			slaves[i] = pid;
			return;
		}
}

void getinfo_click(struct menu_item *m)
{
	const char *name;
	int i;
	
	i = icbox_get_index(main_icbox);
	if (i >= 0)
		name = icbox_get_text(main_icbox, i);
	else
	{
		if (listfd >= 0)
			return;
		name = ".";
	}
	
	get_info(main_form, name);
}

static void progitem_click(struct menu_item *m)
{
	_newtaskl(m->p_data, m->p_data, (void *)NULL);
}

void about(struct menu_item *m)
{
	const char *name = desktop ? "Desktop v1.4" : "File Manager v1.4";
	const char *icon = desktop ? "desktop.pnm"  : "filemgr.pnm";
	
	if (desktop && !full_screen)
		name = "Session Manager v1.3";
	
	dlg_about7(main_form, NULL, name, SYS_PRODUCT, SYS_AUTHOR, SYS_CONTACT, icon);
}

static void prepare_logout(void)
{
	logout = 1;
	
	form_busy(main_form);
	evt_idle();
	
	win_killdesktop(SIGTERM);
	usleep2(1000000, 0);
}

static void restart_sess(void)
{
	prepare_logout();
	autosave_pos();
	exit(253);
}

void restart_click(struct menu_item *m)
{
	if (msgbox_ask(main_form, "Session restart", "Are you sure you want to restart the session?") == MSGBOX_YES)
		restart_sess();
}

void logout_click(struct menu_item *m)
{
	int result;
	
	result = msgbox_ask(main_form, "Logout", "Are you sure you want to end the session?");
	win_idle();
	
	if (result == MSGBOX_YES)
	{
		prepare_logout();
		exit(0);
	}
}

void shutdown_click(struct menu_item *m)
{
	struct gadget *c1;
	struct gadget *c2;
	struct gadget *c3;
	struct form *f;
	
	if (geteuid())
	{
		if (msgbox_ask(main_form, "Shutdown", "Are you sure you want to shut down this machine?") == MSGBOX_YES)
		{
			prepare_logout();
			exit(254);
		}
		return;
	}
	
	f = form_load(SHUTDOWN_FORM);
	c1 = gadget_find(f, "c1");
	c2 = gadget_find(f, "c2");
	c3 = gadget_find(f, "c3");
	
	chkbox_set_group(c1, 1);
	chkbox_set_group(c2, 1);
	chkbox_set_group(c3, 1);
	chkbox_set_state(c1, 1);
	
	form_set_dialog(main_form, f);
	if (form_wait(f) == 1)
	{
		form_hide(f);
		prepare_logout();
		
		if (chkbox_get_state(c1)) execl(SHUTDOWN, SHUTDOWN, "-p", NULL);
		if (chkbox_get_state(c2)) execl(SHUTDOWN, SHUTDOWN, "-r", NULL);
		execl(SHUTDOWN, SHUTDOWN, "-h", NULL);
		
		msgbox_perror(f, "File Manager", "Unable to exec \"" SHUTDOWN "\"", errno);
		form_unbusy(main_form);
	}
	form_close(f);
}

void kill_click(struct menu_item *m)
{
	int result;
	
	result = msgbox_ask(main_form, "File Manager",
				"Are you sure you want to kill all programs\n"
				"on the desktop?");
	
	if (result == MSGBOX_YES)
		exit(0);
}

void updir_click(struct menu_item *m)
{
	enter_dir("..");
}

void dotfile_click(struct menu_item *m)
{
	config->show_dotfiles = !config->show_dotfiles;
	load_dir();
}

void save_click(struct menu_item *m)
{
	char msg[256];
	
	if (pref_filemgr_save())
		msgbox_perror(main_form, "File Manager", "Cannot save configuration", errno);
}

static void save_pos(int complain)
{
	struct form_state ost;
	struct form_state fst;
	int fd = -1;
	
	if (listfd >= 0)
		return;
	
	bzero(&fst, sizeof fst);
	form_get_state(main_form, &fst);
	
	if (unlink(WINPOS) && errno != ENOENT)
		goto fail;
	
	fd = open(WINPOS, O_CREAT | O_TRUNC | O_EXCL | O_RDWR, 0600);
	if (fd < 0)
		goto fail;
	
	if (read(fd, &ost, sizeof fst) != sizeof ost || memcmp(&fst, &ost, sizeof fst))
	{
		errno = EINVAL;
		if (write(fd, &fst, sizeof fst) != sizeof fst)
			goto fail;
	}
	
	if (close(fd))
		goto fail;
	return;

fail:
	if (complain)
		msgbox_perror(main_form, "File Manager", "Cannot save window state", errno);
	if (fd >= 0)
		close(fd);
}

static void autosave_pos(void)
{
	char cwd[PATH_MAX];
	size_t hlen;
	char *h;
	
	if (!do_save_pos)
		return;
	
	if (!getcwd(cwd, sizeof cwd))
		return;
	
	h = getenv("HOME");
	if (!h)
		return;
	hlen = strlen(h);
	
	if (strncmp(cwd, h, hlen))
		return;
	
	if (cwd[hlen] && cwd[hlen] != '/')
		return;
	
	save_pos(0);
}

static void load_pos(void)
{
	struct form_state fst;
	struct stat st;
	int fd = -1;
	
	if (listfd >= 0)
		return;
	
	fd = open(WINPOS, O_RDONLY);
	if (fd < 0)
		return;
	
	if (fstat(fd, &st))
		goto clean;
	
	if (st.st_uid != getuid())
		goto clean;
	if (!S_ISREG(st.st_mode))
		goto clean;
	
	if (read(fd, &fst, sizeof fst) != sizeof fst)
		goto clean;
	
	close(fd);
	
	form_set_state(main_form, &fst);
	return;

clean:
	close(fd);
}

static void save_pos_click(struct menu_item *m)
{
	save_pos(1);
}

void passwd_click(struct menu_item *m)
{
	if (osdemo)
	{
		msgbox(main_form, "Change Password",
			"Passwords cannot be changed in the live CD/USB mode.\n\n"
			"Install the operating system to set a password.");
		return;
	}
	
	_newtaskl(VTTY, VTTY, "-wTPassword", PASSWD, NULL);
}

void by_name_click(struct menu_item *m)
{
	config->sort_order = SORT_NAME;
	load_dir();
}

void by_size_click(struct menu_item *m)
{
	config->sort_order = SORT_SIZE;
	load_dir();
}

void by_type_click(struct menu_item *m)
{
	config->sort_order = SORT_TYPE;
	load_dir();
}

void refresh_click(struct menu_item *m)
{
	load_dir();
}

void edit_click(struct menu_item *m)
{
	int i;
	
	i = icbox_get_index(main_icbox);
	if (i < 0)
		return;
	
	if (file_info[i].with_stat && !S_ISREG(file_info[i].st.st_mode))
	{
		msgbox(main_form, "Edit as Text", "Not a regular file.");
		return;
	}
	if (_newtaskl(EDITOR, EDITOR, file_info[i].name, NULL) < 0)
		msgbox(main_form, "Edit as Text", "Cannot launch editor.");
}

int main_form_key_down(struct form *f, unsigned ch, unsigned shift)
{
	if (ch == '\b')
	{
		updir_click(NULL);
		return 0;
	}
	return 1;
}

void load_back(void)
{
	struct back_conf bc;
	
	if (c_load("backdrop", &bc, sizeof bc))
	{
		strcpy(bc.pathname, DEFAULT_BACKGROUND_PATHNAME);
		bc.tile = DEFAULT_BACKGROUND_TILE;
	}
	
	if (backdrop)
	{
		bmp_free(backdrop);
		backdrop = NULL;
	}
	
	if (!*bc.pathname)
		return;
	
	backdrop_tile = bc.tile;
	backdrop      = bmp_load(bc.pathname);
	if (backdrop)
	{
		bmp_set_bg(backdrop, 0, 0, 0, 255);
		bmp_conv(backdrop);
	}
}

static void icbox_redraw_item(struct gadget *g, int i, int wd,
			      int x, int y, int w, int h, int sel_only)
{
	struct icbox_item *im = &g->icbox.items[i];
	win_color text_bg;
	win_color text;
	win_color bg;
	int text_x;
	int text_y;
	int text_w;
	int text_h;
	
	win_text_size(WIN_FONT_DEFAULT, &text_w, &text_h, im->text);
	text_x = x + (w - text_w) / 2;
	text_y = y + g->icbox.icon_height + 1;
	
	if (i == g->icbox.item_index)
	{
		text_bg	= wc_get(WC_SEL_BG);
		text	= wc_get(WC_SEL_FG);
	}
	else
	{
		text_bg	= wc_get(WC_DESKTOP);
		text	= wc_get(WC_DESKTOP_FG);
	}
	bg = wc_get(WC_DESKTOP);
	
	if (!sel_only)
		bmp_draw(wd, im->icon, x + (w - g->icbox.icon_width) / 2, y + 1);
	win_rect(wd, text_bg, text_x - 2, text_y - 1, text_w + 3, text_h + 2);
	win_text(wd, text, text_x, text_y, im->text);
}

static void icbox_redraw_list_item(struct gadget *g, int i, int wd, int x, int y, int w, int h, int sel_only)
{
	struct icbox_item *im = &g->icbox.items[i];
	char sstr[32];
	char mstr[16];
	struct file_info *fi;
	win_color text_bg;
	win_color text_fg;
	int chw, chh;
	int text_x, text_y;
	int text_w, text_h;
	int size_w, size_h;
	
	win_text_size(WIN_FONT_DEFAULT, &text_w, &text_h, im->text);
	win_text_size(WIN_FONT_DEFAULT, &chw, &chh, "X");
	text_x = x + 2;
	text_y = y + 1;
	
	if (i == g->icbox.item_index)
	{
		text_bg	= wc_get(WC_SEL_BG);
		text_fg	= wc_get(WC_SEL_FG);
	}
	else
	{
		text_bg	= wc_get(WC_BG);
		text_fg	= wc_get(WC_FG);
	}
	
	fi = &file_info[i];
	if (fi->with_stat)
	{
		strcpy(sstr, fmthumanoffs(fi->st.st_size, 1));
		
		mkmstr(mstr, fi->st.st_mode);
		if (!*fi->owner)
			mkostr(fi->owner, fi->group, fi->st.st_uid, fi->st.st_gid);
	}
	else
	{
		strcpy(mstr, "??????????");
		strcpy(sstr, "???");
	}
	
	win_text_size(WIN_FONT_DEFAULT, &size_w, &size_h, sstr);
	
	win_rect(wd,  text_bg, x, y, w, h);
	win_btext(wd, text_bg, text_fg, text_x,		   text_y, mstr);
	win_btext(wd, text_bg, text_fg, text_x + chw * 11, text_y, fi->owner);
	win_btext(wd, text_bg, text_fg, text_x + chw * 20, text_y, fi->group);
	win_btext(wd, text_bg, text_fg, text_x + chw * 35 - size_w, text_y, sstr);
	win_btext(wd, text_bg, text_fg, text_x + chw * 36, text_y, im->text);
}

static void icbox_size_list_item(struct gadget *g, int i, int *w, int *h)
{
	struct icbox_item *im = &g->icbox.items[i];
	int tw, th;
	
	win_text_size(WIN_FONT_DEFAULT, &tw, &th, im->text);
	*w = g->rect.w;
	*h = th + 2;
}

static void redraw_bg(int wd, int x, int y, int w, int h, int yoff)
{
	int x0, y0, x1, y1;
	int cx, cy;
	int bw, bh;
	int dw, dh;
	
	if (backdrop == NULL || !backdrop_tile)
		win_rect(wd, wc_get(WC_DESKTOP), x, y, w, h);
	if (backdrop == NULL)
		return;
	bmp_size(backdrop, &bw, &bh);
	
	if (backdrop_tile)
	{
		x0 = x / bw;
		y0 = y / bh;
		x1 = (x + w + bw - 1) / bw;
		y1 = (y + h + bh - 1) / bh;
		for (cy = y0; cy < y1; cy++)
			for (cx = x0; cx < x1; cx++)
				bmp_draw(wd, backdrop, cx * bw, cy * bh);
	}
	else
	{
		win_desktop_size(&dw, &dh);
		
		x0  = (dw - bw) >> 1;
		y0  = (dh - bh) >> 1;
		y0 -= yoff;
		
		bmp_draw(wd, backdrop, x0, y0);
	}
}

static void icbox_redraw_bg(struct gadget *g, int wd, int x, int y, int w, int h)
{
	redraw_bg(wd, x, y, w, h, g->rect.y);
}

static void v_icons_click(struct menu_item *m)
{
	v_mode_icons();
	load_dir();
}

static void v_list_click(struct menu_item *m)
{
	v_mode_list();
	load_dir();
}

static void cli_click(struct menu_item *m)
{
	const char *shell = getenv("SHELL");
	const char *home = getenv("HOME");
	
	if (!shell)
		shell = _PATH_B_DEFSHELL;
	
	if (!home)
		home = _PATH_H_DEFAULT;
	
	chdir(home);
	
	if (_newtaskl(_PATH_B_VTTY, _PATH_B_VTTY, (void *)NULL) < 0)
		msgbox_perror(main_form, "File Manager", "Task creation failed", errno);
	
	if (_chdir(cwd, CHDIR_OBSERVE))
		msgbox_perror(main_form, "File Manager", "chdir failed", errno);
}

static void on_drop(struct gadget *g, const void *data, size_t len)
{
	char csrc[PATH_MAX];
	char cdst[PATH_MAX];
	char d[PATH_MAX];
	const char *s = data;
	const char *p;
	const char *fmt;
	pid_t pid;
	int dlen;
	int i;
	
	if (memchr(data, 0, len) == NULL)
		return;
	
	if (*s != '/')
		return;
	
	p = strrchr(s, '/') + 1;
	if (!*p)
		return;
	
	if (cwd[strlen(cwd) - 1] == '/')
		fmt = "%s%s";
	else
		fmt = "%s/%s";
	
	dlen = snprintf(d, sizeof d, fmt, cwd, p);
	if (dlen >= sizeof d)
		return;
	
	strcpy(csrc, s);
	strcpy(cdst, d);
	
	if (_mkcanon(cwd, csrc) || _mkcanon(cwd, cdst))
		return;
	
	if (!strcmp(csrc, cdst))
		return;
	
	pid = _newtaskl(SLAVE, SLAVE, "drag", s, d, (void *)NULL);
	if (pid < 1)
	{
		msgbox(main_form, "File Manager", "Could not run slave.");
		return;
	}
	
	for (i = 0; i < MAX_SLAVES; i++)
		if (!slaves[i])
		{
			slaves[i] = pid;
			return;
		}
}

static void on_drag(struct gadget *g)
{
}

int main_form_close(struct form *form)
{
	autosave_pos();
	
	if (desktop)
	{
		logout_click(NULL);
		return;
	}
	
	exit(0);
}

static void create_main_form(void)
{
	int sbw = wm_get(WM_SCROLLBAR);
	struct win_rect wsr;
	struct menu *options;
	struct menu *session;
	struct menu *view;
	struct menu *file;
	struct menu *m;
	
	win_ws_getrect(&wsr);
	
	if (config->form_w < 200)
		config->form_w = 200;
	if (config->form_h < 64)
		config->form_h = 64;
	
	if (config->form_w > wsr.w)
		config->form_w = wsr.w;
	if (config->form_h > wsr.h)
		config->form_h = wsr.h;
	
	file = menu_creat();
	if (listfd < 0)
	{
		menu_newitem4(file, "New File ...",	 'N',	new_file);
		menu_newitem4(file, "New Directory ...", 'M',	new_dir);
		menu_newitem (file, "-", NULL);
	}
	menu_newitem4(file, "Edit as Text", 'E', edit_click);
	if (listfd < 0)
	{
		menu_newitem (file, "-", NULL);
		menu_newitem4(file, "Copy ...",		'C',			copy_file);
		menu_newitem4(file, "Rename ...",	'V',			rename_file);
		menu_newitem5(file, "Delete ...",	WIN_KEY_DEL,	0,	delete_file);
	}
	menu_newitem (file, "-", NULL);
	menu_newitem5(file, "Get Info ...", '\n', WIN_SHIFT_ALT, getinfo_click);
	
	view = menu_creat();
	if (listfd < 0)
	{
		menu_newitem4(view, "Show/hide dotfiles ...", '.', dotfile_click);
		menu_newitem (view, "-", NULL);
	}
	menu_newitem4(view, "Sort by name", '1', by_name_click);
	menu_newitem4(view, "Sort by size", '2', by_size_click);
	menu_newitem4(view, "Sort by type", '3', by_type_click);
	if (listfd < 0)
	{
		menu_newitem (view, "-", NULL);
		menu_newitem4(view, "As icons", 'I', v_icons_click);
		menu_newitem4(view, "As list", 'U', v_list_click);
		menu_newitem (view, "-", NULL);
		menu_newitem4(view, "Refresh", 'R', refresh_click);
		menu_newitem (view, "-", NULL);
		menu_newitem4(view, "Save window position", 'W', save_pos_click);
		menu_newitem4(view, "Save settings", 'S', save_click);
	}
	
	options = menu_creat();
	if (desktop && !geteuid())
	{
		menu_newitem4(options, "Available resources ...", 'R',	progitem_click)->p_data = "/bin/avail";
		menu_newitem4(options, "System load ...",	  'S',	progitem_click)->p_data = "/sbin/showload";
		menu_newitem (options, "-", NULL);
		menu_newitem (options, "Format diskette ...",		progitem_click)->p_data = "/sbin/wfdfmt";
		menu_newitem (options, "-", NULL);
		menu_newitem (options, "Disk performance ...",		progitem_click)->p_data = "/sbin/diskperf";
		menu_newitem4(options, "Disk space ...",	  'D',	progitem_click)->p_data = "/sbin/diskfree";
		menu_newitem (options, "-", NULL);
	}
	menu_newitem(options, "About ...", about);
	
	session = menu_creat();
	menu_newitem4(session, "Change Password ...", 'X', passwd_click);
	menu_newitem (session, "-", NULL);
	menu_newitem4(session, "Command Line ...", 'L', cli_click);
	menu_newitem (session, "-", NULL);
	if (!osdemo)
		menu_newitem4(session, "Logout", WIN_KEY_LEFT, logout_click);
	if (dflag || !geteuid())
		menu_newitem4(session, "Shutdown", WIN_KEY_DOWN, shutdown_click);
	menu_newitem(session, "-", NULL);
	menu_newitem(session, "Restart", restart_click);
	if (!osdemo)
	{
		menu_newitem(session, "Kill", kill_click);
	}
	
	m = menu_creat();
	menu_submenu(m, "File", file);
	if (desktop)
		menu_submenu(m, "Session", session);
	else
	{
		menu_submenu(m, "View", view);
	}
	menu_submenu(m, "Options", options);
	if (!desktop && listfd < 0)
	{
		menu_newitem(m, "-", NULL);
		menu_newitem(m, "Up-Dir", updir_click);
	}
	if (listfd < 0)
	{
		menu_newitem(m, "-", NULL);
		menu_newitem(m, "Refresh", refresh_click);
		menu_newitem(m, "-", NULL);
	}
	
	if (full_screen)
	{
		struct win_rect ws;
		int w, h;
		
		win_ws_getrect(&ws);
		
		main_form = form_creat(FORM_BACKDROP | FORM_NO_BACKGROUND, 0, ws.x, ws.y, ws.w, ws.h, "Desktop");
		
		if (show_menu)
			form_set_menu(main_form, m);
		form_on_close(main_form, main_form_close);
		form_on_resize(main_form, main_form_resize);
		
		main_icbox = icbox_creat(main_form, 0, 0, ws.w, ws.h);
		icbox_set_item_cb(main_icbox, icbox_redraw_item);
		icbox_set_bg_cb(main_icbox, icbox_redraw_bg);
		icbox_on_select(main_icbox, main_icbox_select);
		icbox_on_request(main_icbox, main_icbox_request);
		icbox_on_scroll(main_icbox, main_icbox_scroll);
		
		on_resize();
		load_back();
	}
	else
	{
		main_form = form_creat(FORM_APPFLAGS | FORM_NO_BACKGROUND, 0, -1, -1, config->form_w, config->form_h, cwd);
		if (show_menu)
			form_set_menu(main_form, m);
		form_on_key_down(main_form, main_form_key_down);
		form_on_close(main_form, main_form_close);
		form_on_resize(main_form, main_form_resize);
		form_on_move(main_form, main_form_move);
		update_title();
		
		main_icbox = icbox_creat(main_form, 0, 0, config->form_w - sbw, config->form_h);
		icbox_on_select(main_icbox, main_icbox_select);
		icbox_on_request(main_icbox, main_icbox_request);
		icbox_on_scroll(main_icbox, main_icbox_scroll);
		
		main_sbar = vsbar_creat(main_form, config->form_w - sbw, 0, sbw, config->form_h);
		vsbar_on_move(main_sbar, main_sbar_move);
		vsbar_set_step(main_sbar, config->large_icons ? 32 : 16);
	}
	if (config->large_icons)
		icbox_set_icon_size(main_icbox, 64, 64);
	
	icbox_set_double_click(main_icbox, config->double_click);
	
	gadget_on_drop(main_icbox, on_drop);
	gadget_on_drag(main_icbox, on_drag);
	
	if (listfd >= 0)
		v_list_click(NULL);
	
	main_icbox->menu = file;
	gadget_focus(main_icbox);
	win_idle();
}

static void bg_redraw(struct gadget *g, int wd)
{
	redraw_bg(wd, 0, 0, g->rect.w, g->rect.h, 0);
}

static void create_bg_form(void)
{
	struct win_rect wsr;
	
	if (!desktop || full_screen)
		return;
	
	win_ws_getrect(&wsr);
	
	bg_form = form_creat(FORM_BACKDROP | FORM_EXCLUDE_FROM_LIST, 1, wsr.x, wsr.y, wsr.w, wsr.h, "");
	bg = gadget_creat(bg_form, 0, 0, wsr.w, wsr.h);
	
	bg->redraw = bg_redraw;
}

static void create_forms(void)
{
	create_bg_form();
	create_main_form();
}

static void on_resize(void)
{
	struct win_rect ws;
	
	win_ws_getrect(&ws);
	
	if (full_screen)
	{
		gadget_resize(main_icbox, ws.w, ws.h - main_form->menu_rect.h);
		form_resize(main_form, ws.w, ws.h - main_form->menu_rect.h);
		form_move(main_form, ws.x, ws.y);
	}
	
	if (bg_form)
	{
		gadget_resize(bg, ws.w, ws.h);
		form_resize(bg_form, ws.w, ws.h);
		form_move(bg_form, ws.x, ws.y);
	}
}

void on_setmode(void)
{
	int w, h;
	
	if (desktop)
	{
		wc_load_cte();
		wp_load();
		
		load_back();
	}
	
	load_dir();
}

void on_update(void)
{
	if (desktop)
	{
		load_back();
		load_dir();
		gadget_redraw(main_icbox);
		on_resize();
		wp_load();
	}
	else
		load_dir();
	
	if (bg)
		gadget_redraw(bg);
}

void startup(void)
{
	char msg[PATH_MAX + 256];
	char buf[PATH_MAX + 1];
	char *nl;
	FILE *f;
	
	f = fopen("../.startup", "r");
	if (!f)
		return;
	fcntl(f->fd, F_SETFD, 1);
	
	while (fgets(buf, sizeof buf, f))
	{
		nl = strchr(buf, '\n');
		if (nl)
			*nl = 0;
		
		if (access(buf, X_OK) || _newtaskl(buf, buf, NULL) < 0)
		{
			sprintf(msg, "Cannot load \"%s\"", buf);
			msgbox_perror(NULL, "File Manager", msg, errno);
		}
	}
	fclose(f);
}

void sig_usr1(int nr)
{
	load_dir();
	
	win_signal(SIGUSR1, sig_usr1);
}

void msgbox_chld(struct _xstatus *st)
{
	char msg[PATH_MAX + 256];
	
	if (!st->status)
		return;
	
	if (WTERMSIG(st->status))
	{
		if (WTERMSIG(st->status) == SIGTERM && logout)
			return;
		
		sprintf(msg, "\"%s\" - Abnormal termination:\n\n%s", st->exec_name, strsignal(WTERMSIG(st->status)));
		msgbox(main_form, "Abnormal termination", msg);
	}
}

void sig_chld(int nr)
{
	struct _xstatus st;
	
	while (!_xwait(-1, &st, WNOHANG) && st.pid)
	{
		if (!strcmp(st.exec_name, SLAVE))
			load_dir();
		msgbox_chld(&st);
	}
	win_signal(SIGCHLD, sig_chld);
}

static void usage(void)
{
	msgbox(NULL, "File Manager", "usage:\n"
				     "  filemgr [-l FD] [DIRECTORY]\n"
				     "  -filemgr [-d]");
	exit(255);
}

static void refresh_timer_fired(void *data)
{
	if (do_refresh)
	{
		do_refresh = 0;
		load_dir();
	}
}

static void on_fnotif(struct event *e)
{
	do_refresh = 1;
}

static void sig_usr(int nr)
{
	restart_sess();
}

int main(int argc, char **argv)
{
	struct timeval tv;
	char buf[32];
	int c;
	
	if (getenv("OSDEMO"))
		osdemo = 1;
	
	if (win_attach())
		err(255, NULL);
	
	win_signal(SIGCHLD, sig_chld);
	
	while (c = getopt(argc, argv, "dl:"), c >= 0)
		switch (c)
		{
		case 'l':
			listfd = atoi(optarg);
			break;
		case 'd':
			dflag = 1;
			break;
		default:
			usage();
		}
	
	if (*argv[0] == '-')
	{
		char *home = getenv("HOME");
		
		if (!home)
		{
			msgbox(NULL, "File Manager", "$HOME is not set");
			home = "/";
		}
		
		if (!_chdir(home, CHDIR_OBSERVE))
			enter_dir(".desktop");
		else
			_chdir("/", CHDIR_OBSERVE);
		
		desktop = 1;
	}
	
	argv += optind;
	argc -= optind;
	
	load_config();
	if (argc == 1)
	{
		if (strlen(argv[0]) >= sizeof cwd)
		{
			msgbox(NULL, "File Manager", "Path too long");
			return 255;
		}
		
		if (enter_dir(argv[0]))
			return 1;
	}
	if (listfd < 0)
		load_dirinfo();
	
	if (desktop && !config->win_desk)
		full_screen = 1;
	
	getcwd(cwd, sizeof cwd);
	create_forms();
	enter_dir(".");
	
	win_on_setmode(on_setmode);
	win_on_resize(on_resize);
	win_on_update(on_update);
	
	if (!full_screen)
	{
		do_save_pos = 1;
		load_pos();
	}
	
	tv.tv_sec   = 1;
	tv.tv_usec  = 0;
	refresh_timer = tmr_creat(&tv, &tv, refresh_timer_fired, NULL, 1);
	
	evt_on_fnotif(on_fnotif);
	
	if (desktop)
	{
		win_signal(SIGUSR1, sig_usr);
		
		sprintf(buf, "SESSMGR=%i", (int)getpid());
		putenv(buf);
		
		startup();
	}
	
	win_idle();
	form_show(main_form);
	while (!win_wait());
	msgbox(NULL, "File Manager", "win_wait error");
	return errno;
}
