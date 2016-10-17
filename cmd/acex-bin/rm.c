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
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <err.h>

static int force = 0;
static int recur = 0;
static int xcode = 0;

static void procopt(int argc, char **argv);
static void do_rmdir(const char *name);
static void do_rm(const char *name);
static void usage(void);

static void procopt(int argc, char **argv)
{
	int opt;
	
	while (opt = getopt(argc, argv, "firR"), opt > 0)
		switch (opt)
		{
		case 'f':
			force = 1;
			break;
		case 'i':
			force = 0;
			break;
		case 'r':
		case 'R':
			recur = 1;
			break;
		default:
			exit(255);
		}
}

static void do_rmdir(const char *name)
{
	struct dirent *de;
	DIR *dir;
	
	if (chdir(name))
	{
		if (!xcode)
			xcode = errno;
		warn("%s: chdir", name);
		return;
	}
	dir = opendir(".");
	errno = 0;
	while ((de = readdir(dir)))
	{
		if (!strcmp(de->d_name, "."))
			continue;
		if (!strcmp(de->d_name, ".."))
			continue;
		do_rm(de->d_name);
		errno = 0;
	}
	if (errno)
	{
		if (!xcode)
			xcode = errno;
		warn("%s: readdir", name);
	}
	closedir(dir);
	if (chdir(".."))
		err(errno, "..: chdir");
	if (rmdir(name))
	{
		if (!xcode)
			xcode = errno;
		warn("%s: rmdir", name);
	}
}

static void do_rm(const char *name)
{
	struct stat st;
	
	if (!access(name, 0) && !force)
	{
		char buf[MAX_CANON];
		int cnt;
		
		do
		{
			fprintf(stderr, "Remove %s? ", name);
			cnt = read(STDIN_FILENO, buf, MAX_CANON);
			if (!cnt)
				exit(255);
			if (cnt < 0)
				err(errno, "stdin");
		} while (cnt != 2);
		
		if (*buf != 'y')
			return;
	}
	
	if (stat(name, &st))
	{
		if (errno == ENOENT && force)
			return;
		if (!xcode)
			xcode = errno;
		warn("%s: stat", name);
		return;
	}
	
	if (S_ISDIR(st.st_mode))
	{
		if (recur)
		{
			do_rmdir(name);
			return;
		}
		if (!xcode)
			xcode = EISDIR;
		errno = EISDIR;
		warn("%s", name);
		return;
	}
	
	if (unlink(name))
	{
		if (!xcode)
			xcode = errno;
		warn("%s: unlink", name);
	}
}

static void usage(void)
{
	printf("\nUsage: rm [-firR] FILE...\n"
		"Remove FILE\n\n"
		" -f  force (do not ask for confirmation)\n"
		" -i  interactive mode (ask for confirmation)\n"
		" -r  process directiories recursively\n"
		" -R  same as -r\n\n"
		);
	exit(0);
}

int main(int argc, char **argv)
{
	int i;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
		usage();
	procopt(argc, argv);
	
	if (optind >= argc && !force)
		errx(255, "too few arguments");
	
	for (i = optind; i < argc; i++)
		do_rm(argv[i]);
	return xcode;
}
