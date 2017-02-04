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
#include <errno.h>
#include <stdio.h>

#define BYTES_PER_LINE	16

static int err;

static void hexdump(char *pathname)
{
	unsigned char buf[BYTES_PER_LINE];
	off_t off = 0;
	size_t cnt;
	FILE *f;
	int i;
	
	if (pathname)
	{
		f = fopen(pathname, "r");
		if (!f)
		{
			perror(pathname);
			err = errno;
			return;
		}
	}
	else
	{
		pathname = "stdin";
		f = stdin;
	}
	
	while (cnt = fread(buf, 1, sizeof buf, f), cnt)
	{
		printf("%08lx ", (long)off);
		for (i = 0; i < cnt; i++)
			printf("%02x ", buf[i]);
		for (; i < BYTES_PER_LINE; i++)
			fputs("   ", stdout);
		fputc('|', stdout);
		for (i = 0; i < cnt; i++)
		{
			unsigned char ch = buf[i];
			
			fputc(ch >= 0x20 && ch <= 0x7f ? ch : '.', stdout);
		}
		for (; i < BYTES_PER_LINE; i++)
			fputc(' ', stdout);
		fputs("|\n", stdout);
		
		off += cnt;
	}
	if (ferror(f))
	{
		perror(pathname);
		err = errno;
	}
	if (f != stdin)
		fclose(f);
}

int main(int argc, char **argv)
{
	int i;
	
	for (i = 1; i < argc; i++)
		hexdump(argv[i]);
	if (argc < 2)
		hexdump(NULL);
	return err;
}
