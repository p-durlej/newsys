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

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <err.h>

#define CMPBUF		4096

static struct file
{
	const char *	pathname;
	off_t		off;
	int		fd;
} files[2];

static int sflag, lflag, zflag;
static int xst;

static void open_file(struct file *f, const char *pathname)
{
	f->pathname = pathname;
	f->fd = open(pathname, O_RDONLY);
	if (f->fd < 0)
		err(1, "%s: open", pathname);
	
	if (f->off && lseek(f->fd, f->off, SEEK_SET) < 0)
		err(1, "%s: lseek", pathname);
}

static void do_cmp(void)
{
	static char buf0[CMPBUF];
	static char buf1[CMPBUF];
	
	ssize_t cnt0, cnt1, cnt, i, off = 0;
	long ln = 1;
	
	for (;;)
	{
		cnt0 = read(files[0].fd, buf0, sizeof buf0);
		if (cnt0 < 0)
			err(1, "%s: read", files[0].pathname);
		
		cnt1 = read(files[1].fd, buf1, sizeof buf1);
		if (cnt1 < 0)
			err(1, "%s: read", files[1].pathname);
		
		cnt = cnt0;
		if (cnt > cnt1)
			cnt = cnt1;
		
		if (!cnt)
			break;
		
		for (i = 0; i < cnt; i++)
		{
			if (buf0[i] == buf1[i])
				goto next;
			xst = 1;
			
			if (sflag)
				return;
			
			if (lflag)
			{
				printf("%5li %3o %3o\n",
					(long)(off + i + 1 - zflag),
					buf0[i], buf1[i]);
				continue;
			}
			
			printf("Files %s and %s differ: char %li, line %li.\n",
				files[0].pathname,
				files[1].pathname,
				(long)(off + i + 1 - zflag),
				ln);
			return;
next:
			if (buf0[i] == '\n')
				ln++;
		}
		
		if (cnt0 != cnt1)
		{
			if (cnt0 > cnt1)
				warnx("EOF on %s.", files[1].pathname);
			else
				warnx("EOF on %s.", files[0].pathname);
			xst = 1;
			break;
		}
		
		off += cnt;
	}
}

int main(int argc, char **argv)
{
	int c;
	
	while (c = getopt(argc, argv, "slz"), c > 0)
		switch (c)
		{
		case 's':
			sflag = 1;
			break;
		case 'l':
			lflag = 1;
			break;
		case 'z':
			zflag = 1;
			break;
		default:
			return 1;
		}
	
	argv += optind;
	argc -= optind;
	
	if (argc < 2 || argc > 4)
		errx(1, "wrong nr of args");
	
	if (argc >= 3)
		files[0].off = atol(argv[2]);
	if (argc >= 4)
		files[1].off = atol(argv[3]);
	
	open_file(&files[0], argv[0]);
	open_file(&files[1], argv[1]);
	
	do_cmp();
	return xst;
}
