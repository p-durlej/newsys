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
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <systat.h>
#include <stdio.h>
#include <mount.h>
#include <errno.h>
#include <event.h>
#include <os386.h>
#include <paths.h>
#include <time.h>
#include <err.h>

#define TIME_LIMIT	5

static char *systask[] =
{
	"/sbin/init",
	"/sbin/init.desktop",
	"/sbin/shutdown",
	"/sbin/vtty-con",
};

static pid_t		ignpids[1024];
static int		igncnt;

static struct _mtab	mtab[MOUNT_MAX];

static int is_systask(pid_t pid, char *pathname)
{
	int i;
	
	if (*pathname != '/')
		return 1;
	
	for (i = 0; i < igncnt; i++)
		if (ignpids[i] == pid)
			return 1;
	
	for (i = 0; i < sizeof systask / sizeof *systask; i++)
		if (!strcmp(pathname, systask[i]))
			return 1;
	return 0;
}

static int tasks_exited(int interactive)
{
	static char msg[64 + PATH_MAX];
	static struct taskinfo *tab;
	static int max;
	int cnt;
	int i;
	
	while (waitpid(-1, &i, WNOHANG) > 0);
	
	if (!tab)
	{
		max = _taskmax();
		tab = malloc(sizeof *tab * max);
		if (!tab)
			return 0;
	}
	cnt = _taskinfo(tab);
	
	if (interactive)
		for (i = 0; i < cnt; i++)
		{
			if (!tab[i].pid || is_systask(tab[i].pid, tab[i].pathname))
				continue;
			sprintf(msg, "shutdown: %s is still running\n", tab[i].pathname);
			_sysmesg(msg);
		}
	
	for (i = 0; i < cnt; i++)
	{
		if (!tab[i].pid || is_systask(tab[i].pid, tab[i].pathname))
			continue;
		return 0;
	}
	return 1;
}

static int umountr(struct _mtab *mte)
{
	int cnt = 3;
	
	for (;;)
	{
		if (!_umount(mte->prefix))
			return 0;
		if (errno != EBUSY)
			break;
		
		if (!--cnt)
			break;
		
		_sysmesg("shutdown: ");
		_sysmesg(mte->device);
		_sysmesg(": busy, retrying\n");
		
		usleep2(1000000, 0);
	}
	
	return -1;
}

int main(int argc, char **argv)
{
	int type = SHUTDOWN_POWER;
	int sig  = SIGUSR2;
	int uflag = 0;
	char *p;
	time_t t;
	int i;
	int c;
	
	if (geteuid())
		errx(1, "You are not root");
	
	p = strrchr(argv[0], '/');
	if (p != NULL)
		p++;
	else
		p = argv[0];
	
	if (!strcmp(p, "reboot"))
	{
		type = SHUTDOWN_REBOOT;
		sig  = SIGINT;
	}
	else if (!strcmp(p, "pwoff"))
	{
		type = SHUTDOWN_POWER;
		sig  = SIGUSR2;
	}
	else if (!strcmp(p, "halt"))
	{
		type = SHUTDOWN_HALT;
		sig  = SIGUSR1;
	}
	
	while (c = getopt(argc, argv, "rphu"), c >= 0)
		switch (c)
		{
		case 'r':
			type = SHUTDOWN_REBOOT;
			sig  = SIGINT;
			break;
		case 'p':
			type = SHUTDOWN_POWER;
			sig  = SIGUSR2;
			break;
		case 'h':
			type = SHUTDOWN_HALT;
			sig  = SIGUSR1;
			break;
		case 'u':
			uflag = 1;
			break;
		default:
			return 1;
		}
	
	if (uflag)
		for (;;)
			_shutdown(type);
	
	if (getpid() != 1)
	{
		kill(1, sig);
		return 0;
	}
	
	if (_boot_flags() & BOOT_VERBOSE)
	{
		_sysmesg("shutdown: shutting down\n");
		_sysmesg("shutdown: broadcasting SIGTERM\n");
	}
	
	kill(-1, SIGTERM);
	for (t = time(NULL); t + TIME_LIMIT > time(NULL) && !tasks_exited(0); )
		sleep(1);
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("shutdown: broadcasting SIGKILL\n");
	
	kill(-1, SIGKILL);
	for (t = time(NULL); t + TIME_LIMIT > time(NULL) && !tasks_exited(0); )
		sleep(1);
	
	for (i = 0; i < OPEN_MAX; i++)
		close(i);
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("shutdown: unmounting filesystems\n");
	
	if (_mtab(mtab, sizeof mtab))
	{
		_sysmesg("shutdown: unable to retrieve mtab\n");
		sync();
		for (;;)
			pause();
	}
	
	for (i = 0; i < MOUNT_MAX; i++)
		if (mtab[i].mounted)
		{
			if (!strcmp(mtab[i].prefix, _PATH_M_PTY))
				continue;
			if (!strcmp(mtab[i].prefix, _PATH_M_DEV))
				continue;
			
			if (umountr(&mtab[i]))
			{
				_sysmesg("shutdown: unable to unmount ");
				_sysmesg(mtab[i].device);
				_sysmesg("\n");
			}
		}
	
	sync();
	
	for (;;)
		_shutdown(type);
}
