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

#include <sys/ioctl.h>
#include <dev/fd.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <err.h>

static char *	pathname = "/dev/fd0,0";
static int	fd;

static void fmttrack(int track, int side)
{
	struct fmttrack fmt;
	int retries = 0;
	
	printf("\rFormatting track %i, side %i ...", track, side);
	fflush(stdout);
	
	bzero(&fmt, sizeof fmt);
	fmt.cyl  = track;
	fmt.head = side;
	
retry:
	if (ioctl(fd, FDIOCFMTTRK, &fmt))
	{
		if (errno == EIO && ++retries <= 5)
			goto retry;
		putchar('\n');
		err(errno, "%s: FDIOCFMTTRK", pathname);
	}
}

int main(int argc, char **argv)
{
	int i;
	
	if (argc > 1)
		pathname = argv[1];
	
	fd = open(pathname, O_RDWR);
	if (fd < 0)
		err(errno, "%s: open", pathname);
	
	for (i = 0; i < 80; i++)
	{
		fmttrack(i, 0);
		fmttrack(i, 1);
	}
	
	printf("\r                               \r");
	printf("Formatting completed.\n");
	return 0;
}
