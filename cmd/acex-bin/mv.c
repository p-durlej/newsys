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

#include <sys/stat.h>
#include <sys/wait.h>
#include <newtask.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <paths.h>
#include <err.h>

static int confirm = 1;
static int xcode   = 0;

static char *basename(const char *name);
static void  procopt(int argc, char **argv);
static int   overwrite(const char *name);
static void  do_xmv(const char *src, const char *dst);
static void  do_mv(const char *src, const char *dst);

static char *basename(const char *name)
{
	char *s;
	
	s = strrchr(name, '/');
	if (s)
		return s + 1;
	return (char *)name;
}

static char *scwd(const char *name)
{
	char *p;
	
	p = (char *)name;
	while (p[0] == '.' && p[1] == '/')
		p += 2;
	return p;
}

static void procopt(int argc, char **argv)
{
	int opt;
	
	while (opt = getopt(argc, argv, "fi"), opt > 0)
		switch (opt)
		{
		case 'f':
			confirm = 0;
			break;
		case 'i':
			confirm = 1;
			break;
		default:
			exit(255);
		}
}

static void setxcode(int st)
{
	if (xcode)
		return;
	xcode = st;
}

static int overwrite(const char *name)
{
	if (confirm)
	{
		char buf[MAX_CANON];
		int cnt;
		
		do
		{
			fprintf(stderr, "Overwrite %s? ", scwd(name));
			
			cnt = read(STDIN_FILENO, buf, MAX_CANON);
			if (cnt < 1)
				exit(255);
		}
		while (cnt != 2);
		
		if (*buf != 'y')
			return -1;
	}
	
	if (remove(name))
	{
		setxcode(errno);
		warn("%s: remove", scwd(name));
		return -1;
	}
	return 0;
}

static void do_xmv(const char *src, const char *dst)
{
	pid_t pid;
	int st;
	
	pid = _newtaskl(_PATH_B_CP, _PATH_B_CP, confirm ? "-irp" : "-frp", "--", src, dst, NULL);
	if (pid < 0)
		err(errno, _PATH_B_CP);
	
	while (wait(&st) != pid);
	if (st)
	{
		if (WTERMSIG(st))
		{
			setxcode(128 + WTERMSIG(st));
			return;
		}
		setxcode(WEXITSTATUS(st));
		return;
	}
	
	pid = _newtaskl(_PATH_B_RM, _PATH_B_RM, "-rf", "--", src, NULL);
	if (pid < 0)
		err(errno, _PATH_B_RM);
	
	while (wait(&st) != pid);
	if (st)
	{
		if (WTERMSIG(st))
		{
			setxcode(128 + WTERMSIG(st));
			return;
		}
		setxcode(WEXITSTATUS(st));
	}
}

static void do_mv(const char *src, const char *dst)
{
	if (!access(dst, 0) && overwrite(dst))
		return;
	
retry:
	if (rename(src, dst))
	{
		if (errno == EEXIST && !overwrite(dst))
			goto retry;
		if (errno == EXDEV)
		{
			do_xmv(src, dst);
			return;
		}
		setxcode(errno);
		warn("%s -> %s: link", scwd(src), scwd(dst));
	}
}

static void usage(void)
{
	printf("\nUsage: mv [-fi] SOURCE... DEST\n"
		"Move SOURCE to DEST\n\n"
		" -f  force (do not ask for confirmation)\n"
		" -i  interactive mode (ask for confirmation)\n\n"
		);
	exit(0);
}

int main(int argc, char **argv)
{
	int target_exists = 1;
	struct stat st;
	char *target;
	int i;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
		usage();
	procopt(argc, argv);
	
	if (optind > argc - 2)
		errx(255, "too few arguments");
	
	signal(SIGCHLD, SIG_DFL);
	
	target = argv[argc - 1];
	if (stat(target, &st))
	{
		if (errno != ENOENT)
			err(errno, "%s: stat", scwd(target));
		if (optind < argc - 2)
			errx(255, "too many arguments");
		target_exists = 0;
	}
	
	for (i = optind; i < argc - 1; i++)
	{
		char buf[PATH_MAX];
		
		if (target_exists && S_ISDIR(st.st_mode))
		{
			strcpy(buf, target);
			strcat(buf, "/");
			strcat(buf, basename(argv[i]));
		}
		else
			strcpy(buf, target);
		
		do_mv(argv[i], buf);
	}
	return 0;
}
