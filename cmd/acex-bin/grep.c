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
#include <regexp.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

static int	found;
static int	xcode;
static int	hflag;
static int	vflag;
static int	bigD;
static int	bigF;
static char *	pat;
static regexp *	rx;

static void do_grep_f(FILE *f, const char *pathname)
{
	char buf[4096];
	char *p;
	int r;
	
	while (fgets(buf, sizeof buf, f))
	{
		p = strchr(buf, '\n');
		if (p != NULL)
			*p = 0;
		
		if (bigF)
			r = strstr(buf, pat) != NULL;
		else
			r = regexec(rx, buf);
		if (vflag)
			r = !r;
		
		if (r)
		{
			if (hflag)
				printf("%s: ", pathname);
			puts(buf);
			found = 1;
		}
	}
	if (ferror(f))
	{
		xcode = errno;
		warn("%s", pathname);
	}
}

static void do_grep(const char *pathname)
{
	FILE *f;
	
	f = fopen(pathname, "r");
	if (f == NULL)
	{
		xcode = errno;
		warn("%s", pathname);
		return;
	}
	do_grep_f(f, pathname);
	fclose(f);
}

int main(int argc, char **argv)
{
	char *p;
	int i;
	int c;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		printf("\nUsage: grep [-DFhv] [FILE...]\n"
		       "       fgrep [-Dhv] [FILE...]\n"
		       "Select lines by regular expression.\n\n"
		       "  -D output the regular expression in compiled form\n"
		       "  -F the pattern is a fixed string (this is the default if the program is\n"
		       "     called as fgrep)\n"
		       "  -h output filenames\n"
		       "  -v invert the pattern\n\n");
		return 0;
	}
	
	p = strchr(argv[0], '/');
	if (p == NULL)
		p = argv[0];
	else
		p++;
	
	if (!strcmp(p, "fgrep"))
		bigF = 1;
	
	while (c = getopt(argc, argv, "DFhv"), c > 0)
		switch (c)
		{
		case 'D':
			bigD = 1;
			break;
		case 'F':
			bigF = 1;
			break;
		case 'h':
			hflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		default:
			return 1;
		}
	
	if (optind >= argc)
		errx(1, "Pattern not specified");
	pat = argv[optind];
	
	if (!bigF)
	{
		rx = regcomp(argv[optind]);
		if (rx == NULL)
			errx(1, "Invalid regular expression");
	}
	optind++;
	
	if (bigD)
		regdump(rx);
	
	if (optind == argc)
		do_grep_f(stdin, "stdin");
	
	if (optind < argc - 1)
		hflag = 1;
	
	for (i = optind; i < argc; i++)
		do_grep(argv[i]);
	
	if (!found && !xcode)
		xcode = 1;
	return xcode;
}
