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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <os386.h>
#include <err.h>

static int nlin = 25;
static int ncol = 80;
static int lin, col;
static int tty_fd;
static int xcode;

static void pputc(unsigned char ch)
{
	if (ch == '\t')
	{
		int i;
		int t;
		
		t = col & ~0x07;
		t += 8;
		
		for (i = col; i < t; i++)
			pputc(' ');
		return;
	}
	if ((ch < 0x20 || ch > 0x7f) && ch != '\n')
	{
		char hex[3];
		
		pputc('\\');
		pputc('x');
		sprintf(hex, "%02x", (unsigned int)ch);
		pputc(hex[0]);
		pputc(hex[1]);
		return;
	}
	
	fputc(ch, stdout);
	
	if (ch == '\n')
	{
		lin++;
		col = 0;
	}
	else
		col++;
	if (col == ncol)
	{
		col = 0;
		lin++;
	}
	
	if (lin == nlin - 1)
	{
		int cnt;
		char c;
		
		lin = 0;
		printf("(more)");
		fflush(stdout);
		
		do
		{
			cnt = read(tty_fd, &c, 1);
			if (cnt < 0)
			{
				perror("more: stdin: read");
				exit(errno);
			}
		} while(cnt && c != '\n');
	}
}

static void do_more(const char *name)
{
	char buf[512];
	int cnt;
	int fd;
	int i;
	
	if (name)
	{
		fd = open(name, O_RDONLY);
		if (fd < 0)
		{
			xcode = errno;
			warn("%s: open", name);
			return;
		}
	}
	else
	{
		fd = STDIN_FILENO;
		name = "stdin";
	}
	
	while ((cnt = read(fd, buf, sizeof buf)))
	{
		if (cnt < 0)
		{
			xcode = errno;
			warn("%s: read", name);
			if (fd != STDIN_FILENO)
				close(fd);
			return;
		}
		
		for (i = 0; i < cnt; i++)
			pputc(buf[i]);
	}
}

int main(int argc, char **argv)
{
	struct pty_size psz;
	int i;
	int c;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		printf("\nUsage: more [-l LINES] [-c COLUMNS] [FILE...]\n"
			"Show FILE contents page by page.\n\n"
			" -l  assume the terminal to have LINES lines\n"
			" -c  assume the terminal to have COLUMNS columns\n\n"
			);
		return 0;
	}
	
	if (getenv("LINES"))
		nlin = strtoul(getenv("LINES"), NULL, 0);
	if (getenv("COLUMNS"))
		ncol = strtoul(getenv("COLUMNS"), NULL, 0);
	
	tty_fd = open("/dev/tty", O_RDONLY);
	if (tty_fd < 0)
	{
		perror("/dev/tty");
		return errno;
	}
	
	if (!ioctl(tty_fd, PTY_GSIZE, &psz))
	{
		nlin = psz.h;
		ncol = psz.w;
	}
	
	while (c = getopt(argc, argv, "c:l:"), c > 0)
		switch (c)
		{
		case 'c':
			ncol = atoi(optarg);
			break;
		case 'l':
			nlin = atoi(optarg);
			break;
		default:
			return 1;
		}
	
	argv += optind;
	argc -= optind;
	
	if (!argc)
	{
		do_more(NULL);
		return 0;
	}
	
	for (i = 0; i < argc; i++)
		do_more(argv[i]);
	return xcode;
}
