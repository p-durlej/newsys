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

#include <sys/signal.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <dev/console.h>
#include <newtask.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <os386.h>
#include <errno.h>
#include <event.h>
#include <wingui.h>
#include <limits.h>
#include <string.h>
#include <paths.h>
#include <stdio.h>
#include <vt100.h>
#include <err.h>

#define PTM_FD	3
#define CON_FD	4

static struct vt100 *	vt100;

static char *		shell = _PATH_B_SH;
static pid_t		sh_pid;

static struct cioinfo	cinfo;

static void event_handler(struct event *e)
{
	unsigned ch;

	switch (e->win.type)
	{
	case WIN_E_KEY_DOWN:
		ch = e->win.ch;
		if (ch >= 128)
			return;

		if (e->win.shift & WIN_SHIFT_CTRL)
			ch &= 0x1f;

		if (write(PTM_FD, &ch, 1) != 1)
			perror("vtty-con: writing to ptm");
		break;
	case WIN_E_SWITCH_FROM:
		win_focus(e->win.wd);
		break;
	}
}

static void init_desktop(void)
{
	int wd;
	
	if (win_newdesktop("text"))
	{
		perror("win_newdesktop");
		exit(errno);
	}
	
	if (_mod_load("/lib/drv/pckbd.drv", NULL, 0) < 0)
	{
		perror("/lib/drv/pckbd.drv");
		exit(errno);
	}
	
	win_creat(&wd, 1, 0, 0, 0, 0, event_handler, NULL);
	win_focus(wd);
}

static int spawn_shell(void)
{
	sh_pid = fork();
	
	if (sh_pid < 0)
	{
		warn("vtty-con: fork");
		return -1;
	}
	
	if (!sh_pid)
	{
		win_detach();
		
		execl(shell, shell, NULL);
		err(1, "%s", shell);
	}
	return 0;
}

static void restart(void)
{
	while (sync())
	{
		perror("vtty-con: sync");
		sleep(1);
	}
	while (spawn_shell())
		sleep(1);
}

static void sig_chld(int nr)
{
	pid_t pid;
	int st;
	
	while (pid = waitpid(-1, &st, WNOHANG), pid > 0)
		if (pid == sh_pid || sh_pid < 1)
			restart();
	signal(SIGCHLD, sig_chld);
}

static void output(const char *s)
{
	while (*s)
	{
		if (*s == '\n')
			vt100_putc(vt100, '\r');
		vt100_putc(vt100, *s++);
	}
	con_goto(CON_FD, vt100->x, vt100->y);
}

static void refresh(void)
{
	char buf[vt100->w * 2];
	struct vt100_cell *cl, *cle;
	char *dp;
	int fg, bg;
	int y;
	
	for (y = 0; y < vt100->h; y++)
		if (vt100->line_dirty[y])
		{
			cl  = vt100->cells + y * vt100->w;
			cle = cl + vt100->w;
			dp  = buf;
			while (cl < cle)
			{
				if (cl->attr & VT100A_INVERT)
				{
					fg = cl->bg;
					bg = cl->fg;
				}
				else
				{
					fg = cl->fg;
					bg = cl->bg;
				}
				*dp++ = cl->ch;
				*dp++ = fg | (bg << 4);
				cl++;
			}
			con_puta(CON_FD, 0, y, buf, vt100->w);
		}
}

int main(int argc, char **argv)
{
	char pts_name[PATH_MAX];
	struct pty_size psz;
	struct pollfd pfd;
	int c;
	int i;
	
	while (c = getopt(argc, argv, "ls:"), c > 0)
		switch (c)
		{
		case 'l':
			shell = "/sbin/login";
			break;
		case 's':
			shell = optarg;
			break;
		default:
			return 127;
		}
	
	init_desktop();
	
	for (i = 0; i < OPEN_MAX; i++)
		close(i);
	for (i = 1; i <= NSIG; i++)
		signal(i, SIG_DFL);
	signal(SIGCHLD, sig_chld);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT,  SIG_IGN);
	
	if (open(_PATH_D_CONSOLE, O_RDWR))
		_cerr(1, "%s", _PATH_D_CONSOLE);
	
	dup2(0, CON_FD);
	fcntl(CON_FD, F_SETFD, 1);
	close(0);
	
	if (con_info(CON_FD, &cinfo))
		_cerr(1, "%s", _PATH_D_CONSOLE);
	con_goto(CON_FD, 0, 0);
	
	if (open(_PATH_D_PTMX, O_RDWR))
		_cerr(1, "%s", _PATH_D_PTMX);
	
	vt100 = vt100_creat(cinfo.w, cinfo.h);
	if (vt100 == NULL)
		_cerr(1, "vt100_creat");
	
	dup2(0, PTM_FD);
	fcntl(PTM_FD, F_SETFD, 1);
	close(0);
	
	psz.w = cinfo.w;
	psz.h = cinfo.h;
	ioctl(PTM_FD, PTY_SSIZE, &psz);
	
	strcpy(pts_name, _PATH_M_PTY "/");
	ioctl(PTM_FD, PTY_PTSNAME, pts_name + sizeof _PATH_M_PTY);
	ioctl(PTM_FD, PTY_UNLOCK, NULL);
	
	_ctty(pts_name);
	open(pts_name, O_RDONLY);
	open(pts_name, O_WRONLY);
	open(pts_name, O_WRONLY);
	
	con_mute(CON_FD);
	
	spawn_shell();
	
	pfd.fd	   = PTM_FD;
	pfd.events = POLLIN;
	for (;;)
	{
		if (poll(&pfd, 1, -1) > 0)
		{
			if (pfd.revents & POLLIN)
			{
				char buf[257];
				int cnt;
				
				cnt = read(PTM_FD, buf, 256);
				if (cnt < 0)
					continue;
				buf[cnt] = 0;
				output(buf);
				refresh();
			}
		}
		win_idle();
	}
}
