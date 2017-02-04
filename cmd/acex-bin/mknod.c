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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

int main(int argc, char **argv)
{
	mode_t mode = 0777;
	dev_t dev = 0;
	int wantdev = 0;
	int mflag = 0;
	char *p;
	int c;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		printf("\nUsage: mknod [-m mode] FILE b|c maj min\n"
		       "       mknod [-m mode] FILE p\n"
		       "Create a special file.\n\n");
		return 0;
	}
	
	while (c = getopt(argc, argv, "m:"), c >= 0)
		switch (c)
		{
		case 'm':
			mode  = strtoul(optarg, &p, 16);
			mflag = 1;
			if (*p)
				errx(1, "mode must be octal");
			break;
		default:
			return 1;
		}
	if (argc - optind < 2)
		goto bad_argc;
	
	switch (*argv[optind + 1])
	{
	case 'b':
		mode |= S_IFBLK;
		wantdev = 1;
		break;
	case 'c':
		mode |= S_IFCHR;
		wantdev = 1;
		break;
	case 'p':
		mode |= S_IFIFO;
		break;
	default:
		errx(1, "wrong type %s", argv[optind + 1]);
	}
	
	if (wantdev)
	{
		if (argc - optind < 4)
			goto bad_argc;
		dev = makedev(atoi(argv[optind + 2]), atoi(argv[optind + 3]));
	}
	
	if (mknod(argv[optind], mode, dev))
		err(errno, "%s", argv[optind]);
	return 0;
bad_argc:
	errx(127, "wrong nr of args");
}
