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

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <err.h>

static char *	buf;
static int	bs	= 512;
static char *	infile	= NULL;
static char *	outfile	= NULL;
static int	count	= -1;
static int	ifd	= STDIN_FILENO;
static int	ofd	= STDOUT_FILENO;
static long	seek	= 0;
static long	skip	= 0;

static int do_dd(void)
{
	struct stat st;
	int ic  = 0;
	int pic = 0;
	int oc  = 0;
	int poc = 0;
	int cnt;
	
	if (fstat(ifd, &st))
		err(1, "%s: fstat", infile);
	if (skip && S_ISREG(st.st_mode))
	{
		if (lseek(ifd, bs * skip, SEEK_SET) < 0)
			err(1, "dd: %s: lseek", infile);
		skip = 0;
	}
	if (seek && lseek(ofd, bs * seek, SEEK_SET) < 0)
		err(1, "%s: lseek", outfile);
	
	errno = 0;
	while (count)
	{
		cnt = read(ifd, buf, bs);
		if (cnt < 0)
			err(1, "%s: read", infile);
		if (!cnt)
		{
			errno = 0;
			break;
		}
		if (cnt == bs)
			ic++;
		else
			pic++;
		if (skip)
		{
			skip--;
			continue;
		}
		if (write(ofd, buf, cnt) != cnt)
			err(1, "%s: write", outfile);
		if (cnt == bs)
			oc++;
		else
			poc++;
		if (count != -1)
			count--;
		if (!count)
			errno = 0;
	}
	
	fprintf(stderr, "%i+%i blocks in\n",  ic, pic);
	fprintf(stderr, "%i+%i blocks out\n", oc, poc);
	return errno;
}

int main(int argc, char **argv)
{
	int i;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		printf("\nUsage: dd [if=INPUT] [of=OUTPUT] [bs=BLKSIZE] "
			"[count=COUNT] [seek=BLOCKS] [skip=BLOCKS]\n\n"
			"Copy blocks from INPUT to OUTPUT.\n\n"
			" if=INPUT    use INPUT instead of the default stdin\n"
			" of=OUTPUT   use OUTPUT instead of the default stdout\n"
			" bs=BLKSIZE  set block size to BLKSIZE (default is 512 bytes)\n"
			" count=COUNT limit the number of blocks transferred to COUNT blocks\n"
			" seek=BLOCKS seek on output to BLOCKS*BLKSIZE\n"
			" skip=BLOCKS skip BLOCKS blocks on input\n\n"
			);
		return 0;
	}
	
	for (i = 1; i < argc; i++)
	{
		if (!strncmp("if=", argv[i], 3))
		{
			infile = argv[i] + 3;
			continue;
		}
		
		if (!strncmp("of=", argv[i], 3))
		{
			outfile = argv[i] + 3;
			continue;
		}
		
		if (!strncmp("bs=", argv[i], 3))
		{
			bs = strtoul(argv[i] + 3, NULL, 0);
			continue;
		}
		
		if (!strncmp("count=", argv[i], 6))
		{
			count = strtoul(argv[i] + 6, NULL, 0);
			continue;
		}
		
		if (!strncmp("seek=", argv[i], 5))
		{
			seek = strtoul(argv[i] + 5, NULL, 0);
			continue;
		}
		
		if (!strncmp("skip=", argv[i], 5))
		{
			skip = strtoul(argv[i] + 5, NULL, 0);
			continue;
		}
		
		errx(1, "invalid argument: %s", argv[i]);
	}
	
	buf = malloc(bs);
	if (buf == NULL)
		err(1, "malloc");
	
	if (infile != NULL)
	{
		ifd = open(infile, O_RDONLY);
		if (ifd < 0)
			err(1, "%s: open", infile);
	}
	else
		infile = "stdin";
	
	if (outfile != NULL)
	{
		ofd = open(outfile, O_WRONLY | O_CREAT, 0666);
		if (ofd < 0)
			err(1, "%s: open", outfile);
	}
	else
		outfile = "stdout";
	
	return do_dd();
}
