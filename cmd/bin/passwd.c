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

#include <sys/signal.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <os386.h>
#include <stdio.h>
#include <pwd.h>
#include <err.h>

static struct termios tio_saved, tio_necho;

static void check_tty()
{
	int i;
	
	for (i = 0; i < 3; i++)
		if (tcgetattr(i, &tio_saved))
			exit(255);
	
	tio_necho = tio_saved;
	tio_necho.c_lflag &= ~(ECHO | ECHONL);
}

void get_str(char *msg, char *buf, int len)
{
	char *p;
	int cnt;
	
	write(1, msg, strlen(msg));
	
	cnt = read(0, buf, len - 1);
	if (cnt < 0)
	{
		tcsetattr(0, TCSANOW, &tio_saved);
		exit(255);
	}
	buf[cnt] = 0;
	
	p = strchr(buf, '\n');
	if (p)
		*p = 0;
	
	write(1, "\n", 1);
}

static void disable_echo()
{
	tcsetattr(0, TCSANOW, &tio_necho);
}

static void enable_echo()
{
	tcsetattr(0, TCSANOW, &tio_saved);
}

static void sig_int_hup(int nr)
{
	write(1, "\n", 1);
	tcsetattr(0, TCSANOW, &tio_saved);
	raise(nr);
}

int main(int argc, char **argv)
{
	char pass1[PASSWD_MAX + 1];
	char pass2[PASSWD_MAX + 1];
	struct passwd *pw;
	int oflag = 0;
	int i;
	int c;
	
	check_tty();
	
	while (c = getopt(argc, argv, "o"), c > 0)
		switch (c)
		{
		case 'o':
			oflag = 1;
			break;
		default:
			return 1;
		}
	
	argc -= optind;
	argv += optind;
	
	for (i = 1; i <= NSIG; i++)
		signal(i, SIG_IGN);
	signal(SIGHUP, sig_int_hup);
	signal(SIGINT, sig_int_hup);
	
	if (argc > 1)
		errx(255, "wrong numer of arguments");
	
	if (getuid() && argc > 0)
		errx(255, "wrong numer of arguments");
	
	if (argc == 1)
		pw = getpwnam(argv[0]);
	else
		pw = getpwuid(getuid());
	
	if (!pw)
		errx(255, "account not found");
	
	disable_echo();
	
	printf("Changing local password for %s\n", pw->pw_name);
	if (getuid() || oflag)
	{
		get_str("Old password: ", pass1, sizeof pass1);
		if (_chkpass(pw->pw_name, pass1))
		{
			warnx("the password is incorrect");
			enable_echo();
			return 255;
		}
	}
	get_str("New password: ",	 pass1, sizeof pass1);
	get_str("Retype new password: ", pass2, sizeof pass2);
	enable_echo();
	
	if (strcmp(pass1, pass2))
		errx(255, "the passwords do not match");
	
	if (_setpass(pw->pw_name, pass1))
		errx(255, "could not change password");
	
	fprintf(stderr, "Password changed.\n");
	return 0;
}
