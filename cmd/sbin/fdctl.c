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

#include <sys/ioctl.h>
#include <dev/fd.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>

static char *	pathname = "/dev/fd0,0";
static int	fd;

int main(int argc, char **argv)
{
	int rflag = 0, cflag = 0, gflag = 0;
	int seek = -1;
	int xst = 0;
	int c;
	
	while (c = getopt(argc, argv, "rcs:g"), c > 0)
		switch (c)
		{
		case 'r':
			rflag =1;
			break;
		case 'c':
			cflag = 1;
			break;
		case 's':
			seek = atoi(optarg);
			break;
		case 'g':
			gflag = 1;
			break;
		default:
			return 1;
		};
	
	if (optind < argc)
		pathname = argv[optind];
	
	fd = open(pathname, O_RDWR);
	if (fd < 0)
		err(errno, "%s: open", pathname);
	
	if (rflag && ioctl(fd, FDIOCRESET, NULL))
	{
		warn("%s: FDIOCRESET", pathname);
		xst = errno;
	}
	
	if (cflag && ioctl(fd, FDIOCSEEK0, NULL))
	{
		warn("%s: FDIOCSEEK0", pathname);
		xst = errno;
	}
	
	if (seek >= 0 && ioctl(fd, FDIOCSEEK, &seek))
	{
		warn("%s: FDIOCSEEK", pathname);
		xst = errno;
	}
	
	if (gflag)
	{
		if (ioctl(fd, FDIOCGCCYL, &seek))
		{
			warn("%s: FDIOCGCCYL", pathname);
			xst = errno;
		}
		else
			printf("Cylinder %i\n", seek);
	}
	
	return xst;
}
