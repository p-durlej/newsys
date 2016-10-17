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
#include <priv/wingui.h>
#include <sys/wait.h>
#include <newtask.h>
#include <signal.h>
#include <wingui.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <paths.h>
#include <pwd.h>
#include <err.h>

#define LOGIN_FORM	"/lib/forms/login.frm"

static struct form *	main_form;
static struct gadget *	li;
static struct gadget *	pi;

static char *		ddir;

static char *		autologin;
static int		lflag;

static pid_t osk_pid;

static void kill_osk(void)
{
	if (osk_pid > 0)
		kill(osk_pid, SIGTERM);
	while (wait(NULL) != osk_pid);
	osk_pid = -1;
}

static void clear_fds(const char *iop)
{
	int i;
	
	for (i = 0; i < OPEN_MAX; i++)
		close(i);
	if (open(iop, O_RDONLY))
		_exit(255);
	if (open(iop, O_WRONLY) != 1)
		_exit(255);
	if (open(iop, O_WRONLY) != 2)
		_exit(255);
}

static void write_owner(uid_t uid, gid_t gid)
{
	FILE *f;
	
	if (win_owner(uid))
	{
		msgbox_perror(main_form, "Login", "Cannot set desktop owner", errno);
		return;
	}
	
	if (!ddir)
		return;
	if (chdir(ddir))
		goto fail;
	f = fopen("owner.tmp", "w");
	if (!f)
		goto fail;
	if (fprintf(f, "%i", (int)uid) < 0)
		goto fail;
	if (fclose(f))
		goto fail;
	if (rename("owner.tmp", "owner"))
		goto fail;
	return;
fail:
	msgbox_perror(main_form, "Login", "Cannot write login record", errno);
	if (uid)
		exit(255);
}

static void incorrect(void)
{
	msgbox(main_form, "Login", "Login incorrect!");
	exit(255);
}

static void chk_shell(const char *sh)
{
	static char buf[PATH_MAX];
	
	char *p;
	FILE *f;
	
	f = fopen(_PATH_E_SHELLS, "r");
	while (fgets(buf, sizeof buf, f))
	{
		p = strchr(buf, '\n');
		if (p != NULL)
			*p = 0;
		
		if (!strcmp(sh, buf))
		{
			fclose(f);
			return;
		}
	}
	incorrect();
}

static void do_login(const char *name, int autologin)
{
	struct passwd *pwd;
	
	pwd = getpwnam(name);
	if (!pwd)
	{
		if (autologin)
			return;
		incorrect();
	}
	chk_shell(pwd->pw_shell);
	
	write_owner(pwd->pw_uid, pwd->pw_gid);
	win_sec_unlock();
	setgid(pwd->pw_gid);
	setuid(pwd->pw_uid);
	chdir(pwd->pw_dir);
	setenv("SHELL", pwd->pw_shell, 1);
	setenv("HOME", pwd->pw_dir, 1);
	endpwent();
	
	clear_fds("/dev/null");
	
	if (lflag)
		execl(_PATH_B_IPROFILE, _PATH_B_IPROFILE, "-l", (void *)NULL);
	else
		execl(_PATH_B_IPROFILE, _PATH_B_IPROFILE, (void *)NULL);
	
	exit(255);
}

static void login_click(struct gadget *g, int x, int y)
{
	kill_osk();
	
	if (_chkpass(li->text, pi->text))
		incorrect();
	do_login(li->text, 0);
}

static void cancel_click(struct gadget *g, int x, int y)
{
	kill_osk();
	exit(0);
}

static void shutdown_click(struct gadget *g, int x, int y)
{
	if (msgbox_ask(main_form, "Shutdown", "Are you sure you want to shutdown\nthis computer?") != MSGBOX_YES)
		return;
	kill_osk();
	exit(254);
}

static pid_t run_osk(void)
{
	struct win_rect wsr;
	char buf[64];
	int pp[2] = { -1, -1 };
	int sout = -1;
	FILE *f = NULL;
	pid_t pid = -1;
	int x, y;
	
	sout = dup(1);
	if (sout < 0)
		return -1;
	if (pipe(pp))
		goto fail;
	if (dup2(pp[1], 1) < 0)
		goto fail;
	pid = _newtaskl(_PATH_B_OSK, _PATH_B_OSK, "--taskbar", NULL);
	if (pid < 0)
		goto fail;
	dup2(sout, 1);
	close(pp[1]);
	pp[1] = -1;
	
	f = fdopen(pp[0], "r");
	if (f == NULL)
		goto fail;
	pp[0] = -1;
	
	if (fgets(buf, sizeof buf, f) == NULL)
		goto fail;
	if (strchr(buf, '\n') == NULL)
		goto fail;
	
	win_ws_getrect(&wsr);
	x = (wsr.w - main_form->win_rect.w) / 2;
	y = (wsr.h - main_form->win_rect.h - atoi(buf)) / 2;
	form_move(main_form, x, y);
	goto clean;
	
fail:
	if (pid > 0)
		kill(pid, 9);
	pid = -1;
clean:
	close(pp[0]);
	close(pp[1]);
	dup2(sout, 1);
	close(sout);
	if (f != NULL)
		fclose(f);
	return pid;
}

int main(int argc, char **argv)
{
	int c;
	int i;
	
	while (c = getopt(argc, argv, "a:l"), c > 0)
		switch (c)
		{
		case 'a':
			autologin = optarg;
			break;
		case 'l':
			lflag = 1;
			break;
		default:
			return 255;
		}
	
	if (win_attach())
		err(255, NULL);
	
	ddir = getenv("DESKTOP_DIR");
	
	clear_fds("/dev/console");
	for (i = 1; i <= NSIG; i++)
		signal(i, SIG_IGN);
	signal(SIGCHLD, SIG_DFL);
	wc_load_cte();
	
	win_reset();
	wp_load();
	
	if (autologin != NULL)
		do_login(autologin, 1);
	
	main_form = form_load(LOGIN_FORM);
	if (!main_form)
		return 1;
	li = gadget_find(main_form, "login");
	pi = gadget_find(main_form, "pass");
	
	button_on_click(gadget_find(main_form, "login_btn"), login_click);
	button_on_click(gadget_find(main_form, "cancel_btn"), cancel_click);
	if (lflag)
		button_on_click(gadget_find(main_form, "shut_btn"), shutdown_click);
	else
		gadget_remove(gadget_find(main_form, "shut_btn"));
	gadget_focus(li);
	
	osk_pid = run_osk();
	form_show(main_form);
	while (!win_wait());
	return 255;
}
