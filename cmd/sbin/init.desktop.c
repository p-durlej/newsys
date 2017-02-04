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
#include <priv/wingui.h>
#include <wingui_msgbox.h>
#include <wingui_color.h>
#include <prefs/dmode.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <newtask.h>
#include <stdlib.h>
#include <wingui.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <confdb.h>
#include <errno.h>
#include <event.h>
#include <os386.h>
#include <paths.h>
#include <pwd.h>
#include <err.h>

#define DESKTOP_DIR	"/etc/desktops"
#define SHUTDOWN	"/sbin/shutdown"
#define WLOGIN		"/sbin/wlogin"
#define INIT_DEVS	_PATH_B_IDEVS
#define SECURE		"/sbin/secure"

#define SECURE_MSG	"Press CTRL+ALT+RETURN to access this machine."
#define RESTART_MSG	"Restarting session..."
#define SHUTDOWN_MSG	"Shutting down..."

static char *	copyright_msg;
static char *	secure_msg;
static char *	desktop_dir;
static int	desktop_w;
static int	desktop_h;
static int	main_wd;
static pid_t	login_pid = -1;
static pid_t	user_pid  = -1;
static int	lock_wd   = -1;

void mainwnd_proc(struct event *e)
{
	/* static int px, py; */
	
	win_color fg;
	win_color bg;
	
	switch (e->win.type)
	{
	case WIN_E_PTR_DOWN:
		win_focus(e->win.wd);
		break;
	case WIN_E_REDRAW:
		fg = wc_get(WC_DESKTOP_FG);
		bg = wc_get(WC_DESKTOP);
		
		win_paint();
		win_clip(e->win.wd, e->win.redraw_x,
				    e->win.redraw_y,
				    e->win.redraw_w,
				    e->win.redraw_h,
				    0,
				    0);
		win_rect(e->win.wd, bg, 0, 0, desktop_w, desktop_h);
		win_text(e->win.wd, fg, 0, 0, copyright_msg);
		if (login_pid <= 0)
		{
			int w, h;
			
			win_text_size(WIN_FONT_DEFAULT, &w, &h, secure_msg);
			win_text(e->win.wd, fg, (desktop_w - w) >> 1, (desktop_h - h) >> 1, secure_msg);
		}
		win_end_paint();
		break;
	}
}

void redraw(void)
{
	struct event e;
	
	e.type		= E_WINGUI;
	e.win.type	= WIN_E_REDRAW;
	e.win.redraw_x	= 0;
	e.win.redraw_y	= 0;
	e.win.redraw_w	= desktop_w;
	e.win.redraw_h	= desktop_h;
	e.win.wd	= main_wd;
	mainwnd_proc(&e);
	if (lock_wd >= 0)
	{
		e.win.wd = lock_wd;
		mainwnd_proc(&e);
	}
}

void clear_owner(void)
{
	char *p;
	
	p = getenv("SPEAKER");
	if (p != NULL)
	{
		if (chown(p, 0, 0))
			warn("revoke: %s", p);
		if (revoke(p))
			warn("revoke: %s", p);
	}
	while (unlink("owner") && errno != ENOENT && errno != EROFS)
		sleep(1);
	win_owner(-1);
}

struct passwd *get_owner(void)
{
	char buf[32];
	uid_t uid;
	FILE *f;
	
	f = fopen("owner", "r");
	if (!f)
		return NULL;
	
	if (!fgets(buf, sizeof buf, f))
		return NULL;
	uid = atol(buf);
	
	fclose(f);
	
	return getpwuid(uid);
}

void sig_chld(int nr)
{
	struct passwd *pwd;
	int status;
	pid_t pid;
	
	while (pid = waitpid(-1, &status, WNOHANG), pid > 0)
	{
		if (pid == login_pid)
		{
			pwd = get_owner();
			
			win_killdesktop(SIGKILL);
			login_pid = -1;
			user_pid  = -1;
			clear_owner();
			redraw();
			
			win_reset();
			wc_load_cte();
			wp_load();
			
			switch (WEXITSTATUS(status))
			{
			case 253:
				secure_msg = win_break(WIN_FONT_DEFAULT, RESTART_MSG, desktop_w);
				redraw();
				usleep2(1000000, 0);
				
				if (pwd)
					login_pid = _newtaskl(WLOGIN, WLOGIN, "-la", pwd->pw_name, NULL);
				
				secure_msg = win_break(WIN_FONT_DEFAULT, SECURE_MSG, desktop_w);
				redraw();
				break;
			case 254:
				secure_msg = win_break(WIN_FONT_DEFAULT, SHUTDOWN_MSG, desktop_w);
				redraw();
				usleep2(1000000, 0);
				
				execl(_PATH_B_SHUTDOWN, _PATH_B_SHUTDOWN, (void *)NULL);
				break;
			}
			
			endpwent();
		}
		else if (pid == user_pid)
		{
			if (status)
				win_killdesktop(SIGKILL);
			user_pid = -1;
			win_sec_unlock();
		}
	}
	if (pid < 0 && errno != ECHILD)
		warn("sig_chld: waitpid");
	evt_signal(SIGCHLD, sig_chld);
}

static void init_font_map(int hidpi)
{
	static const int map[] =
	{
		WIN_FONT_MONO_L,
		WIN_FONT_SYSTEM_L,
		WIN_FONT_MONO_L,
		WIN_FONT_SYSTEM_L,
		WIN_FONT_MONO_LN,
		WIN_FONT_MONO_LN,
	};
	
	if (hidpi)
		win_set_font_map(map, sizeof map);
	else
		win_set_font_map(map, 0);
}

static void init_dpi(void)
{
	struct dmode *dm = dm_get();
	
	if (!dm)
		return;
	
	win_set_dpi_class(dm->hidpi);
	init_font_map(dm->hidpi);
}

static void init_mode(void)
{
	struct win_modeinfo mi;
	struct dmode *dm;
	
	dm = dm_get();
	if (!dm || dm->nr < 0)
		return;
	
	if (win_modeinfo(dm->nr, &mi) || dm->xres != mi.width || dm->yres != mi.height || dm->nclr != mi.ncolors)
	{
		char msg[256];
		
		init_dpi();
		
		sprintf(msg, "Configured display mode\n\n"
			     "  #%i: %i * %i, %i colors\n\n"
			     " is not valid with this driver",
			     dm->nr, dm->xres, dm->yres, dm->nclr);
		msgbox(NULL, "Mode incorrect", msg);
		return;
	}
	
	win_setmode(dm->nr, 0);
	init_dpi();
}

static void init_env(void)
{
	char buf[4096];
	char *p;
	FILE *f;
	
	f = fopen("environ", "r");
	if (f == NULL)
		return;
	
	while (fgets(buf, sizeof buf, f))
	{
		p = strchr(buf, '\n');
		if (p != NULL)
			*p = 0;
		
		p = strdup(buf);
		if (p == NULL)
			err(1, "strdup");
		putenv(p);
	}
	
	fclose(f);
}

void on_setmode(void)
{
	win_desktop_size(&desktop_w, &desktop_h);
	win_change(main_wd, 1, 0, 0, desktop_w, desktop_h);
}

void on_lock(void)
{
	if (login_pid <= 0)
	{
		login_pid = _newtaskl(WLOGIN, WLOGIN, "-l", NULL);
		redraw();
		return;
	}
	
	if (access("owner", R_OK))
		return;
	if (user_pid > 0)
		return;
	user_pid = _newtaskl(SECURE, SECURE, NULL);
	if (user_pid < 0)
		win_killdesktop(SIGKILL);
}

static void try_autologin(void)
{
	char buf[LOGNAME_MAX + 2];
	char *p;
	FILE *f;
	char *al;
	
	if (asprintf(&al, "%s/autologin", desktop_dir) < 0)
		err(255, "init.desktop: asprintf");
	
	if (access(al, R_OK))
		return;
	
	f = fopen(al, "r");
	if (f == NULL)
		return;
	p = fgets(buf, sizeof buf, f);
	fclose(f);
	
	if (p == NULL)
		return;
	
	p = strchr(buf, '\n');
	if (p)
		*p = 0;
	
	login_pid = _newtaskl(WLOGIN, WLOGIN, "-la", buf, NULL);
	if (login_pid < 0)
		perror(WLOGIN);
}

int main(int argc, char **argv)
{
	int status;
	char *p;
	int i;
	
	for (i = 1; i <= NSIG; i++)
		signal(i, SIG_IGN);
	signal(SIGCHLD, SIG_DFL);
	
	if (argc != 2)
		errx(255, "wrong number of arguments");
	
	desktop_dir = argv[1];
	
	if (*desktop_dir != '/')
		errx(255, "%s: Not an absolute path", desktop_dir);
	
	if (chdir(desktop_dir))
		err(errno, "%s", desktop_dir);
	
	setenv("DESKTOP_DIR", desktop_dir, 1);
	clear_owner();
	
	p = strrchr(argv[1], '/');
	
	if (win_newdesktop(p + 1))
		err(errno, "cannot create a new desktop");
	
	init_dpi();
	
	_newtaskl(INIT_DEVS, INIT_DEVS, p + 1, NULL);
	wait(&status);
	
	win_desktop_size(&desktop_w, &desktop_h);
	win_creat(&main_wd, 1, 0, 0, desktop_w, desktop_h, mainwnd_proc, NULL);
	win_on_setmode(on_setmode);
	win_on_lock(on_lock);
	copyright_msg = win_break(WIN_FONT_DEFAULT, COPYRIGHT, desktop_w);
	if (!copyright_msg)
		err(errno, "win_break");
	secure_msg = win_break(WIN_FONT_DEFAULT, SECURE_MSG, desktop_w);
	if (!secure_msg)
		err(errno, "win_break");
	
	wc_load_cte();
	init_mode();
	wc_load_cte();
	wp_load();
	init_env();
	
	evt_signal(SIGCHLD, sig_chld);
	try_autologin();
	
	for (;;)
		win_wait();
}
