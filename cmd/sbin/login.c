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

#include <termios.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <os386.h>
#include <paths.h>
#include <pwd.h>
#include <err.h>

static char user[LOGNAME_MAX + 1];
static char pass[PASSWD_MAX + 1];

static void incorrect(void)
{
	printf("Login incorrect\n");
	exit(255);
}

static void print_file(const char *pathname)
{
	char buf[256];
	ssize_t cnt;
	int d;
	
	d = open(pathname, O_RDONLY);
	if (d < 0)
		return;
	while (cnt = read(d, buf, sizeof buf), cnt > 0)
		write(1, buf, cnt);
	close(d);
}

static void do_login(void)
{
	char pathname[PATH_MAX];
	struct passwd *pwd;
	char *p;
	int i;
	
	if (_chkpass(user, pass))
		incorrect();
	
	pwd = getpwnam(user);
	if (!pwd)
		incorrect();
	
	p = getenv("SPEAKER");
	if (p != NULL)
		chown(p, pwd->pw_uid, pwd->pw_gid);
	chown(_PATH_TTY, pwd->pw_uid, pwd->pw_gid);
	setgid(pwd->pw_gid);
	setuid(pwd->pw_uid);
	chdir(pwd->pw_dir);
	setenv("HOME", pwd->pw_dir, 1);
	endpwent();
	
	if (setenv("SHELL", pwd->pw_shell, 1))
		err(255, "setenv");
	
	memset(user, 0, sizeof user);
	memset(pass, 0, sizeof pass);
	
	for (i = 1; i <= NSIG; i++)
		signal(i, SIG_DFL);
	
	if (access(".hushlogin", 0))
		print_file(_PATH_E_MOTD);
	
	if (strlen(pwd->pw_shell) >= sizeof pathname)
		errx(255, "Bad shell pathname");
	
	strcpy(pathname, pwd->pw_shell);
	
	p = strrchr(pathname, '/');
	if (p)
		*p = '-';
	else
		p = pathname;
	
	execl(pwd->pw_shell, p, NULL);
	perror(pwd->pw_shell);
	exit(255);
}

static void get_cred(void)
{
	struct termios ts, tn;
	int cnt;
	char *p;
	
	errno = 127;
	if (write(1, "login: ", 7) != 7)
		exit(errno);
	
	cnt = read(0, user, sizeof user - 1);
	if (cnt < 0)
		exit(errno);
	user[cnt] = 0;
	
	p = strchr(user, '\n');
	if (p)
		*p = 0;
	
	if (!*user)
		exit(0);
	
	tcgetattr(0, &ts);
	
	tn = ts;
	tn.c_lflag &= ~(ECHO | ECHONL);
	
	if (tcsetattr(0, TCSANOW, &tn))
		exit(errno);
	
	errno = 127;
	if (write(1, "Password: ", 10) != 10)
		exit(errno);
	
	cnt = read(0, pass, sizeof pass - 1);
	if (cnt < 0)
	{
		tcsetattr(0, TCSANOW, &ts);
		exit(errno);
	}
	write(1, "\n", 1);
	pass[cnt] = 0;
	
	if (tcsetattr(0, TCSANOW, &ts))
		exit(errno);
	
	p = strchr(pass, '\n');
	if (p)
		*p = 0;
}

int main(int argc, char **argv)
{
	int i;
	
	for (i = 0; i < OPEN_MAX; i++)
		close(i);
	if (open(_PATH_TTY, O_RDONLY))
		return 255;
	if (open(_PATH_TTY, O_WRONLY) != 1)
		return 255;
	if (open(_PATH_TTY, O_WRONLY) != 2)
		return 255;
	if (chown(_PATH_TTY, 0, 0))
		return 255;
	
	for (i = 1; i <= NSIG; i++)
		signal(i, SIG_IGN);
	signal(SIGCHLD, SIG_DFL);
	
	get_cred();
	do_login();
	return 127;
}
