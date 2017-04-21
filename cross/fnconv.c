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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define MAX_WIDTH	128

char *out_name;
char *in_name;

FILE *out_file;
FILE *in_file;

int height;

void get_height(void)
{
	char buf[MAX_WIDTH + 2];
	char *p;
	
	height = 0;
	while (fgets(buf, sizeof buf, in_file))
	{
		p = strchr(buf, '\n');
		if (p)
			*p = 0;
		if (!*buf)
			return;
		height++;
	}
	if (ferror(in_file))
	{
		perror(in_name);
		exit(1);
	}
	fprintf(stderr, "%s: unexpected EOF\n", in_name);
	exit(1);
}

int conv_chr(void)
{
	char buf[MAX_WIDTH + 2];
	static int chr_code = 0;
	char ch;
	int i;
	
	fprintf(out_file, "glyph\t%i\n", chr_code++);
	
	for (i = 0; i < height; i++)
	{
		if (!fgets(buf, sizeof buf, in_file))
		{
			if (ferror(in_file))
			{
				perror(in_name);
				exit(1);
			}
			fprintf(stderr, "%s: unexpected EOF\n", in_name);
			exit(1);
		}
		fputs(buf, out_file);
	}
	fputc('\n', out_file);
	
	while (ch = fgetc(in_file), isspace(ch));
	if (ch == EOF)
	{
		if (ferror(in_file))
		{
			perror(in_name);
			exit(1);
		}
		return 0;
	}
	ungetc(ch, in_file);
	return 1;
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		fprintf(stderr, "usage: %s INFILE OUTFILE\n", argv[0]);
		return 1;
	}
	
	out_name = argv[2];
	in_name  = argv[1];
	
	in_file = fopen(in_name, "r");
	if (!in_file)
	{
		perror(in_name);
		return 1;
	}
	
	out_file = fopen(out_name, "w");
	if (!out_file)
	{
		perror(out_name);
		return 1;
	}
	
	get_height();
	fprintf(out_file, "height\t%i\n\n", height);
	
	rewind(in_file);
	while (conv_chr());
	
	if (fclose(out_file))
	{
		perror(out_name);
		return 1;
	}
	fclose(in_file);
	return 0;
}
