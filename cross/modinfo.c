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

 /*
  * WARNING: This program is for i386 only.
  */

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "mgen/module.h"

void *mod;
struct modhead *head;

void dump_head()
{
	printf("Header info:\n");
	printf(" rel_start  = %i\n", head->rel_start);
	printf(" rel_count  = %i\n", head->rel_count);
	printf(" sym_start  = %i\n", head->sym_start);
	printf(" sym_count  = %i\n", head->sym_count);
	printf(" code_start = %i\n", head->code_start);
	printf(" code_size  = %i\n", head->code_size);
}

void dump_reloc(void)
{
	struct modrel *r = (void *)((char *)mod + head->rel_start);
	int i;
	
	printf("Relocation info:\n");
	for (i = 0; i < head->rel_count; i++)
		printf(" %4i 0x%08x %i %i\n", i, r[i].addr, r[i].type, r[i].size);
}

void dump_syms(void)
{
	struct modsym *s = (void *)((char *)mod + head->sym_start);
	int i;
	
	printf("Symbol info:\n");
	for (i = 0; i < head->sym_count; i++)
	{
		switch (s[i].flags)
		{
		case SYM_EXPORT:
			printf(" %-32s 0x%08x %i %2i SYM_EXPORT\n", s[i].name, s[i].addr, s[i].size, s[i].shift);
			break;
		case SYM_IMPORT:
			printf(" %-32s 0x%08x %i %2i SYM_IMPORT\n", s[i].name, s[i].addr, s[i].size, s[i].shift);
			break;
		default:
			printf(" %-32s 0x%08x %i %2i INVALID\n", s[i].name, s[i].addr, s[i].size, s[i].shift);
		}
	}
}

int main(int argc, char **argv)
{
	struct stat st;
	int cnt;
	int fd;
	
	if (argc != 2)
	{
		fprintf(stderr, "usage: modinfo MODULE\n");
		return 255;
	}
	
	fd = open(argv[1], O_RDONLY);
	if (fd < 0 || fstat(fd, &st))
	{
		perror(argv[1]);
		return 255;
	}
	
	head = mod = malloc(st.st_size);
	if (!mod)
	{
		perror(argv[1]);
		return 255;
	}
	cnt = read(fd, mod, st.st_size);
	if (cnt < 0)
	{
		perror(argv[1]);
		return 255;
	}
	if (cnt != st.st_size)
	{
		fprintf(stderr, "%s: short read\n", argv[1]);
		return 255;
	}
	
	dump_head();
	dump_reloc();
	dump_syms();
	
	close(fd);
}
