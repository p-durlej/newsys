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

#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

static int pflag;
static int xcode;

static void do_mkdir(char *path)
{
	if (pflag)
	{
		char *s;
		
		for (s = path ; ; s++)
		{
			s = strchr(s, '/');
			if (!s)
				break;
			*s = 0;
			if (mkdir(path, 0777) && errno != EEXIST)
			{
				if (!xcode)
					xcode = errno;
				warn("%s: mkdir", path);
				return;
			}
			*s = '/';
		}
	}
	
	if (mkdir(path, 0777))
	{
		if (!xcode)
			xcode = errno;
		warn("%s: mkdir", path);
	}
}

int main(int argc, char **argv)
{
	int i = 1;
	int c;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		printf("\nUsage: mkdir [-p] DIR...\n"
			"Create directiories.\n\n"
			" -p  create parent directories as needed\n\n"
			);
		return 0;
	}
	
	while (c = getopt(argc, argv, "p"), c >= 0)
		switch (c)
		{
		case 'p':
			pflag = 1;
			break;
		default:
			return 255;
		}
	
	argv += optind;
	argc -= optind;
	
	if (argc < 1)
		errx(255, "too few arguments");
	
	for (i = 0; i < argc; i++)
		do_mkdir(argv[i]);
	return xcode;
}
