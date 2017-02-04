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

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#define GROW_BY 64

static struct inode
{
	ino_t ino;
	dev_t dev;
} *inodes;
static int ino_cnt;
static int ino_cap;

static int all	 = 0;
static int kbyte = 0;
static int psub	 = 1;
static int xdev	 = 1;

static int errn;

static void procopt(int argc, char **argv)
{
	int opt;
	
	while (opt = getopt(argc, argv, "aksx"), opt > 0)
		switch (opt)
		{
		case 'a':
			all = 1;
			break;
		case 'k':
			kbyte = 1;
			break;
		case 's':
			psub = 0;
			break;
		case 'x':
			xdev = 0;
			break;
		default:
			exit(255);
		}
}

static int add_ino(dev_t dev, ino_t ino)
{
	struct inode *e;
	struct inode *p;
	
	e = inodes + ino_cnt;
	for (p = inodes; p < e; p++)
		if (p->dev == dev && p->ino == ino)
			return 1;
	
	if (ino_cnt >= ino_cap)
	{
		ino_cap += GROW_BY;
		inodes = realloc(inodes, ino_cap * sizeof *inodes);
		if (!inodes)
			err(1, "realloc");
	}
	p = &inodes[ino_cnt++];
	p->dev = dev;
	p->ino = ino;
	return 0;
}

static blkcnt_t do_du(const char *path, dev_t dev, int sub)
{
	struct stat st;
	blkcnt_t cnt = 0;
	
	if (stat(path, &st))
	{
		if (!errn)
			errn = errno;
		warn("%s: stat", path);
		return 0;
	}
	
	if (add_ino(st.st_dev, st.st_ino))
		return 0;
	if (st.st_dev != dev && dev != -1)
		return 0;
	if (!xdev)
		dev = st.st_dev;
	
	cnt += st.st_blocks;
	
	if (S_ISDIR(st.st_mode))
	{
		char sub[PATH_MAX];
		struct dirent *de;
		DIR *dir;
		
		dir = opendir(path);
		if (!dir)
		{
			if (!errn)
				errn = errno;
			warn("opendir", path);
		}
		else
		{
			errno = 0;
			while ((de = readdir(dir)))
			{
				if (!strcmp(de->d_name, "."))
					continue;
				if (!strcmp(de->d_name, ".."))
					continue;
				if (strlen(de->d_name) + strlen(path) + 2 >=
				    PATH_MAX)
				{
					warnx("%s/%s: name too long",
					       path, de->d_name);
					break;
				}
				sprintf(sub, "%s/%s", path, de->d_name);
				cnt += do_du(sub, dev, 1);
				errno = 0;
			}
			if (errno)
			{
				if (!errn)
					errn = errno;
				warn("%s: readdir", path);
			}
			closedir(dir);
		}
	}
	
	if ((S_ISDIR(st.st_mode) && psub) || !sub || all)
	{
		if (!kbyte)
			printf("%-10i %s\n", cnt, path);
		else
			printf("%-10i %s\n", cnt / 2, path);
	}
	return cnt;
}

static void usage(void)
{
	printf("\nUsage: du [-aksx] [NAME...]\n"
		"Show how many 512-byte blocks does file NAME use.\n\n"
		" -a  show all files, not only directiories\n"
		" -k  use kbytes for printing sizes\n"
		" -s  do not show info on subdirectories\n"
		" -x  do not cross device boundaries\n\n"
		);
	exit(0);
}

int main(int argc, char **argv)
{
	int i;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
		usage();
	procopt(argc, argv);
	
	if (optind >= argc)
		do_du(".", -1, 0);
	else
		for (i = optind; i < argc; i++)
			do_du(argv[i], -1, 0);
	return errn;
}
