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
#include <stdio.h>
#include <errno.h>
#include <err.h>

static int xcode;

static void do_cat(const char *name)
{
	static char buf[32768];
	
	int do_close = 0;
	int cnt;
	int fd;
	
	if (name)
	{
		fd = open(name, O_RDONLY);
		if (fd < 0)
		{
			if (!xcode)
				xcode = errno;
			warn("%s: open", name);
			return;
		}
	}
	else
	{
		fd   = STDIN_FILENO;
		name = "stdin";
	}
	
	while ((cnt = read(fd, buf, sizeof buf)))
	{
		if (cnt < 0)
		{
			if (!xcode)
				xcode = errno;
			warn("%s: read", name);
			goto fini;
		}
		
		if (write(STDOUT_FILENO, buf, cnt) != cnt)
		{
			if (!xcode)
				xcode = errno;
			warn("stdout");
		}
	}
fini:
	if (do_close)
		close(fd);
}

int main(int argc, char **argv)
{
	int i;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		printf("\nUsage: cat [FILE...]\n\n"
			"Concatenate FILEs to standard output; "
			"use standard input as input if no FILEs supplied.\n\n"
			);
		return 0;
	}
	
	if (argc == 1)
		do_cat(NULL);
	else
		for (i = 1; i < argc; i++)
			do_cat(argv[i]);
	return xcode;
}
