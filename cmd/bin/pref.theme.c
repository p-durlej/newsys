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

#include <config/defaults.h>
#include <priv/filemgr.h>
#include <wingui_metrics.h>
#include <wingui_msgbox.h>
#include <wingui_color.h>
#include <wingui_form.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <wingui.h>
#include <stdlib.h>
#include <stdio.h>
#include <confdb.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <paths.h>
#include <newtask.h>
#include <err.h>

#define MAIN_FORM	"/lib/forms/theme.frm"

static struct gadget *main_list;
static struct form *main_form;

static int need_sess_restart;

static int setbackdrop(const char *pathname, int tile)
{
	struct back_conf back_conf;
	
	bzero(&back_conf, sizeof back_conf);
	strcpy(back_conf.pathname, pathname);
	back_conf.tile = tile;
	
	if (c_save("/user/backdrop", &back_conf, sizeof back_conf))
		return -1;
	return 0;
}

static void restart_sess(void)
{
	if (!need_sess_restart)
		return;
	
	if (msgbox_ask(main_form, "Theme",
		"The current session needs to be restarted to fully apply the new theme.\n\n"
		"Do you want to restart the session?") == MSGBOX_YES)
	{
		char *p = getenv("SESSMGR");
		
		if (!p)
		{
			msgbox(main_form, "Theme", "Session manager not found.");
			return;
		}
		
		if (kill(atoi(p), SIGUSR1))
			msgbox(main_form, "Theme", "Failed to restart the session.");
	}
}

static void setfwidth(int w)
{
	static int wm_tab[WM_COUNT];
	int i;
	
	for (i = 0; i < WM_COUNT; i++)
		wm_tab[i] = wm_get(i);
	
	if (wm_tab[WM_FRAME] == w)
		return;
	wm_tab[WM_FRAME] = w;
	
	c_save("/user/metrics", &wm_tab, sizeof wm_tab);
	need_sess_restart = 1;
}

static int stheme(const char *name)
{
	char pname[NAME_MAX + 1] = "";
	char bpath[PATH_MAX] = "";
	char buf[PATH_MAX + 2];
	char pathname[PATH_MAX];
	char path[PATH_MAX];
	int fwidth = 5;
	int btile = 1;
	FILE *f;
	
	sprintf(path, "/lib/w_themes/%s", name);
	
	strcpy(pname, name);
	if (c_save("/user/w_theme", pname, sizeof pname))
		goto err;
	
	if (c_save("/user/w_colors", pname, sizeof pname))
		goto err;
	
	sprintf(pathname, "%s/info", path);
	f = fopen(pathname, "r");
	if (f != NULL)
	{
		while (fgets(buf, sizeof buf, f))
		{
			char *p;
			
			p = strchr(buf, '\n');
			if (p != NULL)
				*p = 0;
			
			switch (*buf)
			{
			case 'B':
				if (buf[1] == '/')
					strcpy(bpath, buf + 1);
				else
					sprintf(bpath, "%s/%s", path, buf + 1);
				break;
			case 'T':
				btile = atoi(buf + 1);
				break;
			case 'F':
				fwidth = atoi(buf + 1);
				break;
			}
		}
		
		fclose(f);
	}
	
	setbackdrop(bpath, btile);
	setfwidth(fwidth);
	
	win_update();
	win_redraw_all();
	
	if (!geteuid() && !getenv("OSDEMO"))
		if (msgbox_ask(main_form, "Themes", "Do you want to apply the current appearance settings to the login screen?") == MSGBOX_YES)
		{
			pid_t pid;
			int st;
			
			pid = _newtaskl(_PATH_B_USERMGR, _PATH_B_USERMGR, "-A", (void *)NULL);
			if (pid > 0)
				while (waitpid(pid, &st, 0) != pid)
					;
		}
	
	restart_sess();
	return 0;
err:
	msgbox_perror(main_form, "Themes", "Could not save settings", errno);
	return -1;
}

static int update(void)
{
	int i;
	
	i = list_get_index(main_list);
	if (i < 0)
		return -1;
	
	return stheme(list_get_text(main_list, i));
}

static int load_themes(void)
{
	struct dirent *de;
	const char *n;
	DIR *d;
	int cnt;
	int p;
	
	d = opendir(_PATH_L_THEMES);
	if (d == NULL)
	{
		msgbox_perror(main_form, "Themes", "Cannot load list of themes", errno);
		return -1;
	}
	
	p = cnt = 0;
	while (de = readdir(d), de != NULL)
	{
		n = de->d_name;
		
		if (*n == '.')
			continue;
		
		if (!strcmp(n, DEFAULT_THEME))
			p = 1;
		
		list_newitem(main_list, de->d_name);
		cnt++;
	}
	
	if (!p)
	{
		list_newitem(main_list, DEFAULT_THEME);
		cnt++;
	}
	
	return cnt;
}

int main(int argc, char **argv)
{
	char curr[NAME_MAX + 1];
	int cnt;
	int i;
	
	signal(SIGCHLD, SIG_DFL);
	
	if (win_attach())
		err(255, NULL);
	
	if (c_load("/user/w_theme", &curr, sizeof curr))
		strcpy(curr, DEFAULT_THEME);
	
	main_form = form_load(MAIN_FORM);
	main_list = gadget_find(main_form, "list");
	
	gadget_focus(main_list);
	
	cnt = load_themes();
	
	list_sort(main_list);
	for (i = 0; i < cnt; i++)
		if (!strcmp(list_get_text(main_list, i), curr))
			list_set_index(main_list, i);
	
	if (argc > 1)
		return stheme(argv[1]);
	
	while (form_wait(main_form) == 1 && update());
	return 0;
}
