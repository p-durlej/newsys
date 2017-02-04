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

#include <kern/block.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <os386.h>
#include <errno.h>
#include <comm.h>
#include <err.h>

static int xcode;

int main(int argc, char **argv)
{
	struct comm_stat cst;
	struct fxstat xst;
	struct stat st;
	off_t off;
	blk_t blk;
	int sflag = 0;
	int mflag = 0;
	int xflag = 0;
	char *p;
	int d;
	int i;
	int c;
	
	while (c = getopt(argc, argv, "sxm"), c > 0)
		switch (c)
		{
		case 's':
			sflag = 1;
			break;
		case 'x':
			xflag = 1;
			break;
		case 'm':
			mflag = 1;
			break;
		default:
			return 1;
		}
	
	if (!sflag && !xflag && !mflag)
	{
		p = strchr(argv[0], '/');
		if (p == NULL)
			p = argv[0];
		else
			p++;
		
		if (!strcmp(p, "siostat"))
			sflag = 1;
		else if (!strcmp(p, "bmap"))
			mflag = 1;
		else
			xflag = 1;
	}
	
	for (i = optind; i < argc; i++)
	{
		d = open(argv[i], O_NOIO);
		if (d < 0)
		{
			xcode = errno;
			warn("%s: open", argv[i]);
			continue;
		}
		
		if (mflag && fstat(d, &st))
		{
			xcode = errno;
			warn("%s: stat", argv[i]);
			close(d);
			continue;
		}
		
		if (xflag && ioctl(d, FIOCXSTAT, &xst))
		{
			xcode = errno;
			warn("%s: FIOCXSTAT", argv[i]);
			close(d);
			continue;
		}
		
		if (sflag && comm_stat(d, &cst))
		{
			xcode = errno;
			warn("%s: comm_stat", argv[i]);
			close(d);
			continue;
		}
		
		printf("%s: ", argv[i]);
		
		if (xflag)
			printf("%c%c %i\n",
				xst.state & R_OK ? 'r' : '-',
				xst.state & W_OK ? 'w' : '-',
				xst.refcnt);
		if (sflag)
			printf("  ibu %i, obu %i\n", cst.ibu, cst.obu);
		if (!xflag && !sflag)
			putchar('\n');
		
		if (mflag)
			for (off = 0; off < st.st_size; off += BLK_SIZE)
			{
				blk = off / BLK_SIZE;
				if (ioctl(d, FIOCBMAP, &blk))
				{
					xcode = errno;
					warn("\n%s: FIOCBMAP");
					goto next;
				}
				printf("  %lu\n", blk);
			}
		
next:
		close(d);
	}
	return xcode;
}
