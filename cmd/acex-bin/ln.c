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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <err.h>

static int force = 0;

static char *basename(const char *name)
{
	char *s;
	
	s = strrchr(name, '/');
	if (s)
		return s + 1;
	return (char *)name;
}

static void procopt(int argc, char **argv)
{
	int opt;
	
	while (opt = getopt(argc, argv, "f"), opt > 0)
		switch (opt)
		{
		case 'f':
			force = 0;
			break;
		default:
			exit(255);
		}
}

static void usage(void)
{
	printf("\nUsage: ln [-f] OLDNAME... NEWNAME\n"
		"Link OLDNAME to NEWNAME.\n\n"
		"  -f  unlink NEWNAME if already exists\n\n"
		);
	exit(0);
}

int main(int argc, char **argv)
{
	struct stat st;
	char *src;
	char *dst;
	int e = 0;
	int i;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
		usage();
	procopt(argc, argv);
	
	if (optind > argc - 2)
		errx(255, "too few arguments");
	
	for (i = optind; i < argc - 1; i++)
	{
		dst = argv[argc - 1];
		src = argv[i];
		
		if (!stat(dst, &st) && S_ISDIR(st.st_mode))
			if (asprintf(&dst, "%s/%s", dst, basename(src)) < 0)
				err(errno, "asprintf");
		
		if (force && remove(dst) && errno != ENOENT)
		{
			warn("%s: remove", dst);
			e = errno;
			continue;
		}
		
		if (link(src, dst))
		{
			warn("'%s' -> '%s': link", src, dst);
			e = errno;
		}
	}
	return e;
}
