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
#include <wingui.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <newtask.h>
#include <stdlib.h>
#include <string.h>
#include <event.h>
#include <errno.h>
#include <stdio.h>
#include <paths.h>
#include <pwd.h>
#include <err.h>

static struct form *main_form;
static int	desk_w, desk_h;
static int	main_wd;
static pid_t	chld_pid = -1;

void mainwnd_proc(struct event *e)
{
	win_color bg;
	
	switch (e->win.type)
	{
	case WIN_E_PTR_DOWN:
		win_focus(e->win.wd);
		break;
	case WIN_E_REDRAW:
		bg = wc_get(WC_DESKTOP);
		
		win_paint();
		win_clip(e->win.wd,
			 e->win.redraw_x,
			 e->win.redraw_y,
			 e->win.redraw_w,
			 e->win.redraw_h,
			 0, 0);
		win_rect(e->win.wd, bg, 0, 0, desk_w, desk_h);
		win_end_paint();
		break;
	}
}

static void sig_chld(int nr)
{
	exit(0);
}

struct passwd *get_owner(void)
{
	struct passwd *pw;
	char buf[32];
	uid_t uid;
	FILE *f;
	
	f = fopen("owner", "r");
	if (!f)
	{
		msgbox_perror(NULL, "Secure Access", "Cannot read login record", errno);
		return NULL;
	}
	if (!fgets(buf, sizeof buf, f))
	{
		if (ferror(f))
			msgbox_perror(NULL, "Secure Access", "Cannot read login record", errno);
		else
			msgbox(NULL, "Secure Access", "Cannot read login record.");
		fclose(f);
		return NULL;
	}
	fclose(f);
	
	uid = atol(buf);
	pw  = getpwuid(uid);
	if (!pw)
	{
		msgbox(NULL, "Secure access", "No such user.");
		return NULL;
	}
	return pw;
}

void change_passwd(void)
{
	struct passwd *pw;
	
	pw = get_owner();
	if (pw)
		chld_pid = _newtaskl(_PATH_B_VTTY, _PATH_B_VTTY, "-Mr", "-c", "50", "-l", "16", "-w", "-T", "Password",
				     _PATH_B_CPWD, pw->pw_name, NULL);
	endpwent();
}

void taskman(void)
{
	struct passwd *pw;
	
	pw = get_owner();
	if (pw)
		chld_pid = _newtaskl(_PATH_B_TASKMAN, _PATH_B_TASKMAN, "-M",
			pw->pw_name, NULL);
	endpwent();
}

void lock(void)
{
	struct gadget *g_login;
	struct gadget *g_pass;
	struct form *f = NULL;
	struct passwd *pw;
	char *login;
	char *pass;
	
	pw = get_owner();
	if (!pw)
		goto fini;
	f = form_load("/lib/forms/unlock.frm");
	if (!f)
		goto fini;
	g_login = gadget_find(f, "login");
	g_pass	= gadget_find(f, "pass");
	
	for (;;)
	{
		input_set_text(g_login, pw->pw_name);
		input_set_text(g_pass, "");
		
		form_hide(main_form);
		form_wait(f);
		
		login = g_login->text;
		pass  = g_pass->text;
		if (!strcmp(login, pw->pw_name) && !_chkpass(login, pass))
			goto fini;
		if (!strcmp(login, "root") && !_chkpass("root", pass))
			goto fini;
		msgbox(f, "Unlock", "Login incorrect.");
	}
fini:
	if (f)
		form_close(f);
	endpwent();
}

int main(int argc, char **argv)
{
	if (win_attach())
		return errno;
	win_desktop_size(&desk_w, &desk_h);
	win_signal(SIGCHLD, sig_chld);
	
	if (win_creat(&main_wd, 1, 0, 0, desk_w, desk_h, mainwnd_proc, NULL))
		return errno;
	win_raise(main_wd);
	
	main_form = form_load("/lib/forms/secure.frm");
	if (!main_form)
	{
		win_killdesktop(SIGKILL);
		return errno;
	}
	switch (form_wait(main_form))
	{
	case 1:
		return 0;
	case 2:
		win_killdesktop(SIGKILL);
		return 0;
	case 3:
		lock();
		return 0;
	case 4:
		change_passwd();
		break;
	case 5:
		taskman();
		break;
	default:
		msgbox(NULL, "Secure Access", "Function not implemented.");
		return 0;
	}
	form_close(main_form);
	
	if (chld_pid > 0)
		for (;;)
			win_wait();
	return 0;
}
