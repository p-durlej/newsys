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

#include <priv/copyright.h>
#include <wingui_msgbox.h>
#include <wingui_menu.h>
#include <wingui_form.h>
#include <sys/signal.h>
#include <wingui_color.h>
#include <wingui_dlg.h>
#include <wingui.h>
#include <fmthuman.h>
#include <unistd.h>
#include <systat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <timer.h>
#include <time.h>
#include <pwd.h>
#include <err.h>

#define MAIN_FORM	"/lib/forms/taskman.frm"

static struct form *	main_form;
static struct gadget *	task_list;
static struct gadget *	term_btn;
static struct gadget *	kill_btn;

static int		tm, bm, rm;

static struct taskinfo *task_info;
static int		task_max;

static uid_t		uid;

static struct task
{
	char desc[16 + PATH_MAX];
	pid_t pid;
} *task;

static int task_count;

static void main_resize(struct form *f, int w, int h)
{
	struct win_rect tr, kr;
	
	gadget_get_rect(term_btn, &tr);
	gadget_get_rect(kill_btn, &kr);
	
	gadget_resize(task_list, w - 2 * rm, h - 2 * bm - tm - term_btn->rect.h);
	gadget_move(term_btn, tr.x, h - bm - term_btn->rect.h);
	gadget_move(kill_btn, kr.x, h - bm - kill_btn->rect.h);
}

static int main_close(struct form *f)
{
	exit(0);
}

static void term_click(struct gadget *g, int x, int y)
{
	int i;
	
	i = list_get_index(task_list);
	if (i < 0 || i >= task_count)
		return;
	
	if (kill(task[i].pid, SIGTERM))
	{
		char msg[256];
		
		sprintf(msg, "Cannot terminate %s:\n\n%m", task[i].desc);
		msgbox(main_form, "Task Manager", msg);
	}
}

static void kill_click(struct gadget *g, int x, int y)
{
	int i;
	
	i = list_get_index(task_list);
	if (i < 0 || i >= task_count)
		return;
	
	if (kill(task[i].pid, SIGKILL))
	{
		char msg[256];
		
		sprintf(msg, "Cannot terminate %s:\n\n%m", task[i].desc);
		msgbox(main_form, "Task Manager", msg);
	}
}

static void refresh(void)
{
	int i, n;
	
	task_count = _taskinfo(task_info);
	if (task_count < 0)
	{
		char msg[256];
		
		task_count = 0;
		
		sprintf(msg, "Cannot read task list:\n\n%m");
		msgbox(main_form, "Task Manager", msg);
		return;
	}
	
	list_clear(task_list);
	for (i = 0, n = 0; i < task_count; i++)
	{
		if (task_info[i].ruid != uid && uid)
			continue;
		if (task_info[i].pid)
		{
			sprintf(task[n].desc, "%s (%i)", task_info[i].pathname, task_info[i].pid);
			task[n++].pid = task_info[i].pid;
			
			list_newitem(task_list, (char *)&task_info[i]);
		}
	}
	task_count = n;
	
	i = list_get_index(task_list);
	if (i >= task_count)
		list_set_index(task_list, task_count);
}

static void timer_tick(void *data)
{
	refresh();
}

static void sig_click(struct menu_item *m)
{
	int i;
	
	i = list_get_index(task_list);
	if (i < 0 || i >= task_count)
		return;
	
	if (kill(task[i].pid, m->l_data))
	{
		char msg[256];
		
		sprintf(msg, "Cannot send signal to %s:\n\n%m", task[i].desc);
		msgbox(main_form, "Task Manager", msg);
	}
}

static void about_click(struct menu_item *m)
{
	dlg_about7(main_form, "Task Manager", "Task Manager v1", SYS_PRODUCT, SYS_AUTHOR, SYS_CONTACT, "warning.pnm");
}

static void draw_item(struct gadget *g, int i, int wd, int x, int y, int w, int h)
{
	struct taskinfo *ti;
	char size_str[32];
	char pid_str[32];
	win_color bg;
	win_color fg;
	int tw, th;
	
	win_chr_size(WIN_FONT_DEFAULT, &tw, &th, 'X');
	
	ti = (void *)list_get_text(g, i);
	if (i != list_get_index(g))
	{
		bg = wc_get(WC_BG);
		fg = wc_get(WC_FG);
	}
	else
	{
		if (form_get_focus(g->form) == g)
		{
			bg = wc_get(WC_SEL_BG);
			fg = wc_get(WC_SEL_FG);
		}
		else
		{
			bg = wc_get(WC_NFSEL_BG);
			fg = wc_get(WC_NFSEL_FG);
		}
	}
	
	strcpy(size_str, fmthumansz(ti->size, 0));
	sprintf(pid_str, "%u", ti->pid);
	win_rect(wd, bg, x,	      y, w, h);
	win_text(wd, fg, x + tw / 2,  y, pid_str);
	win_text(wd, fg, x + tw * 6,  y, size_str);
	win_text(wd, fg, x + tw * 15, y, ti->queue);
	win_text(wd, fg, x + tw * 25, y, ti->pathname);
}

int main(int argc, char **argv)
{
	struct win_rect r0, r1;
	struct passwd *pwd;
	struct menu *sig_m;
	struct menu *opt_m;
	struct menu *m;
	int bigM = 0;
	int c;
	
	signal(SIGKILL, SIG_DFL);
	
	if (win_attach())
		err(255, NULL);
	
	while (c = getopt(argc, argv, "M"), c > 0)
		switch (c)
		{
		case 'M':
			bigM = 1;
			break;
		default:
			msgbox(NULL, "Task Manager", "usage: taskman [-M] [USER]");
			return 1;
		}
	
	if (argc > optind + 1)
	{
		msgbox(NULL, "Task Manager", "usage: taskman [-M] [USER]");
		return 255;
	}
	if (argc > optind)
	{
		pwd = getpwnam(argv[optind]);
		if (!pwd)
		{
			msgbox_perror(NULL, "Task Manager", "Cannot retrieve uid", errno);
			return errno;
		}
		uid = pwd->pw_uid;
		endpwent();
	}
	
	task_max = _taskmax();
	task_info = malloc(task_max * sizeof(struct taskinfo));
	task = malloc(task_max * sizeof(struct task));
	if (!task_info || !task)
	{
		msgbox(NULL, "Task Manager", "Memory allocation failed.");
		return errno;
	}
	
	sig_m = menu_creat();
	menu_newitem(sig_m, "1: SIGHUP", sig_click)->l_data   = SIGHUP;
	menu_newitem(sig_m, "2: SIGINT", sig_click)->l_data   = SIGINT;
	menu_newitem(sig_m, "3: SIGQUIT", sig_click)->l_data  = SIGQUIT;
	menu_newitem(sig_m, "4: SIGILL", sig_click)->l_data   = SIGILL;
	menu_newitem(sig_m, "5: SIGTRAP", sig_click)->l_data  = SIGTRAP;
	menu_newitem(sig_m, "6: SIGABRT", sig_click)->l_data  = SIGABRT;
	menu_newitem(sig_m, "7: SIGBUS", sig_click)->l_data   = SIGBUS;
	menu_newitem(sig_m, "8: SIGFPE", sig_click)->l_data   = SIGFPE;
	menu_newitem(sig_m, "9: SIGKILL", sig_click)->l_data  = SIGKILL;
	menu_newitem(sig_m, "10: SIGSTOP", sig_click)->l_data = SIGSTOP;
	menu_newitem(sig_m, "11: SIGSEGV", sig_click)->l_data = SIGSEGV;
	menu_newitem(sig_m, "12: SIGCONT", sig_click)->l_data = SIGCONT;
	menu_newitem(sig_m, "13: SIGPIPE", sig_click)->l_data = SIGPIPE;
	menu_newitem(sig_m, "14: SIGALRM", sig_click)->l_data = SIGALRM;
	menu_newitem(sig_m, "15: SIGTERM", sig_click)->l_data = SIGTERM;
	menu_newitem(sig_m, "16: SIGUSR1", sig_click)->l_data = SIGUSR1;
	menu_newitem(sig_m, "17: SIGUSR2", sig_click)->l_data = SIGUSR2;
	menu_newitem(sig_m, "18: SIGCHLD", sig_click)->l_data = SIGCHLD;
	menu_newitem(sig_m, "19: SIGPWR", sig_click)->l_data  = SIGPWR;
	menu_newitem(sig_m, "-", NULL);
	menu_newitem(sig_m, "29: SIGEVT", sig_click)->l_data  = SIGEVT;
	menu_newitem(sig_m, "30: SIGEXEC", sig_click)->l_data = SIGEXEC;
	menu_newitem(sig_m, "31: SIGSTK", sig_click)->l_data  = SIGSTK;
	menu_newitem(sig_m, "32: SIGOOM", sig_click)->l_data  = SIGOOM;
	
	opt_m = menu_creat();
	menu_newitem(opt_m, "About ...", about_click);
	
	m = menu_creat();
	menu_submenu(m, "Signal", sig_m);
	menu_submenu(m, "Options", opt_m);
	
	main_form = form_load(MAIN_FORM);
	if (bigM)
		main_form->flags &= ~FORM_ALLOW_MINIMIZE;
	
	task_list = gadget_find(main_form, "tasks");
	term_btn  = gadget_find(main_form, "term");
	kill_btn  = gadget_find(main_form, "kill");
	button_on_click(term_btn, term_click);
	button_on_click(kill_btn, kill_click);
	
	list_set_draw_cb(task_list, draw_item);
	
	gadget_get_rect(task_list, &r0);
	gadget_get_rect(kill_btn, &r1);
	tm = r0.y;
	bm = main_form->workspace_rect.h - (r1.y + r1.h);
	rm = main_form->workspace_rect.w - (r0.x + r0.w);
	
	form_on_resize(main_form, main_resize);
	form_on_close(main_form, main_close);
	form_min_size(main_form, kill_btn->rect.x + kill_btn->rect.w + main_form->sizebox->rect.w, 100);
	form_set_menu(main_form, m);
	
	refresh();
	
	tmr_creat(&(struct timeval){ 1, 0 }, &(struct timeval){ 1, 0 }, timer_tick, NULL, 1);
	
	for (;;)
		form_wait(main_form);
}
