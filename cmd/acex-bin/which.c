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

#include <findexec.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

static int s_flag;

int main(int argc, char **argv)
{
	const char *x;
	int xcode = 0;
	int i;
	int c;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		printf("\nUsage: %s [-s] [CMD...]\n"
		       "Find a command.\n\n"
		       "  -s silent mode\n\n", argv[0]);
		return 0;
	}
	
	while (c = getopt(argc, argv, "s"), c > 0)
		switch (c)
		{
		case 's':
			s_flag = 1;
			break;
		default:
			return 127;
		}
	if (optind >= argc)
		errx(127, "wrong nr of args");
	
	for (i = optind; i < argc; i++)
	{
		x = _findexec(argv[i], NULL);
		if (x == NULL)
		{
			xcode = errno;
			warn("%s", argv[i]);
			continue;
		}
		if (!s_flag)
			puts(x);
	}
	return xcode;
}
