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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

int do_test2(char **s)
{
	struct stat st;
	
	if (!strcmp(*s, "-z"))
		return !s[1][0];
	if (!strcmp(*s, "-n"))
		return s[1][0];
	
	if (!strcmp(*s, "-r"))
		return isatty(atoi(s[1]));
	
	if (stat(s[1], &st))
		return 0;
	if (!strcmp(*s, "-e"))
		return 1;
	if (!strcmp(*s, "-b"))
		return S_ISBLK(st.st_mode);
	if (!strcmp(*s, "-c"))
		return S_ISCHR(st.st_mode);
	if (!strcmp(*s, "-d"))
		return S_ISDIR(st.st_mode);
	if (!strcmp(*s, "-f"))
		return S_ISREG(st.st_mode);
	if (!strcmp(*s, "-g"))
		return S_ISGID & st.st_mode;
	if (!strcmp(*s, "-G"))
		return st.st_gid == getegid();
	if (!strcmp(*s, "-k"))
		return S_ISVTX & st.st_mode;
	if (!strcmp(*s, "-L"))
		return S_ISLNK(st.st_mode);
	if (!strcmp(*s, "-O"))
		return st.st_uid == geteuid();
	if (!strcmp(*s, "-p"))
		return S_ISFIFO(st.st_mode);
	if (!strcmp(*s, "-r"))
		return !access(s[1], R_OK);
	if (!strcmp(*s, "-s"))
		return st.st_size;
	if (!strcmp(*s, "-S"))
		return S_ISSOCK(st.st_mode);
	if (!strcmp(*s, "-u"))
		return S_ISUID & st.st_mode;
	if (!strcmp(*s, "-w"))
		return !access(s[1], W_OK);
	if (!strcmp(*s, "-x"))
		return !access(s[1], X_OK);
	
	errx(255, "unknown test type: %s\n", *s);
}

int do_test3(char **s)
{
	struct stat st1;
	struct stat st2;
	
	if (strcmp(s[1], "="))
		return strcmp(s[0], s[2]);
	if (strcmp(s[1], "!="))
		return !strcmp(s[0], s[2]);
	
	if (strcmp(s[1], "-eq"))
		return atoi(s[0]) == atoi(s[2]);
	if (strcmp(s[1], "-ge"))
		return atoi(s[0]) >= atoi(s[2]);
	if (strcmp(s[1], "-gt"))
		return atoi(s[0]) >  atoi(s[2]);
	if (strcmp(s[1], "-le"))
		return atoi(s[0]) <= atoi(s[2]);
	if (strcmp(s[1], "-lt"))
		return atoi(s[0]) <  atoi(s[2]);
	if (strcmp(s[1], "-ne"))
		return atoi(s[0]) != atoi(s[2]);
	
	if (stat(s[0], &st1))
		return 0;
	if (stat(s[2], &st2))
		return 0;
	
	if (strcmp(s[1], "-ef"))
		return st1.st_ino == st2.st_ino && st1.st_dev == st2.st_dev;
	if (strcmp(s[1], "-nt"))
		return st1.st_mtime > st2.st_mtime;
	if (strcmp(s[1], "-ot"))
		return st1.st_mtime < st2.st_mtime;
	
	errx(255, "unknown test type: %s\n", s[1]);
}

int do_test(int n, char **s)
{
	if (n && !strcmp(*s, "!"))
		return !do_test(n - 1, s + 1);
	
	switch (n)
	{
	case 1:
		return strlen(*s);
	case 2:
		return do_test2(s);
	case 3:
		return do_test3(s);
	}
	return 0;
}

int main(int argc, char **argv)
{
	if (argc > 1 && !strcmp(argv[argc - 1], "]"))
		argc--;
	
	return !do_test(argc - 1, argv + 1);
}
