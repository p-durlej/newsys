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

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <err.h>

struct font_chr
{
	unsigned	nr;
	unsigned	pos;
	unsigned	width;
	unsigned	height;
};

static int line_nr = 1;

static FILE *outfile;
static FILE *infile;

static int		tspc, bspc;
static int		fwidth;

static int		height, totw;

static uint8_t *	bmp;
static int		pos;

static void fetch_line(char *buf, size_t sz, int w)
{
	char *p;
	
	for (;;)
	{
		if (fgets(buf, sz, infile) == NULL)
		{
			if (ferror(infile))
				err(1, "input");
			errx(1, "unexpected EOF");
		}
		
		p = strchr(buf, '\n');
		if (p == NULL)
			errx(1, "line %i too long", line_nr);
		*p = 0;
		
		if (!*buf)
			continue;
		
		if (w >= 0 && strlen(buf) != w)
			errx(1, "%i: wrong line length", line_nr);
		break;
	}
}

static int get_val(char *key)
{
	char buf[128];
	int key_len;
	
	fetch_line(buf, sizeof buf, -1);
	
	key_len = strlen(key);
	if (strncmp(buf, key, key_len) || !isspace(buf[key_len]))
		errx(1, "%i: \"%s NUMBER\" expected, found \"%s\"\n",
			line_nr, key, buf);
	
	return atoi(buf + key_len + 1);
}

static void pset(int x, int y)
{
	bmp[x / 8 + y * totw / 8] |= 1 << (x % 8);
}

static int conv_chr(unsigned nr)
{
	static int fw;
	struct font_chr hdr;
	int width = -1;
	unsigned code;
	unsigned di = 0;
	char buf[256];
	char *nl;
	int ch;
	int len;
	int x, y;
	
	code = get_val("glyph");
	
	for (y = 0; y < height; y++)
	{
		fetch_line(buf, sizeof buf, width);
		if (width < 0)
			width = strlen(buf);
		
		for (x = 0; x < width; x++)
			if (buf[x] != '.')
				pset(pos + x, y + tspc);
	}
	
	if (fwidth)
	{
		if (fw && fw != width)
			errx(1, "%i: character width is %i, expected %i", line_nr, width, fw);
		fw = width;
	}
	
	hdr.nr	   = code;
	hdr.pos	   = pos;
	hdr.width  = width;
	hdr.height = height + tspc + bspc;
	
	if (fwrite(&hdr, sizeof hdr, 1, outfile) != 1)
		err(1, "output");
	pos += width;
	
	while (ch = fgetc(infile), isspace(ch) && ch != EOF);
	if (ch == EOF)
		return 0;
	ungetc(ch, infile);
	return 1;
}

static void end_chr(void)
{
	static const struct font_chr hdr = { .nr = (unsigned)-1 };
	
	if (fwrite(&hdr, sizeof hdr, 1, outfile) != 1)
		err(1, "output");
}

static void count_glyphs(void)
{
	char buf[256];
	int ch;
	char *p;
	int w;
	int i;
	
	for (;;)
	{
		get_val("glyph");
		
		fetch_line(buf, sizeof buf, -1);
		w = strlen(buf);
		totw += w;
		
		for (i = 1; i < height; i++)
			fetch_line(buf, sizeof buf, w);
		
		while (ch = fgetc(infile), isspace(ch) && ch != EOF);
		if (ch == EOF)
			return;
		ungetc(ch, infile);
	}
}

int main(int argc, char **argv)
{
	unsigned i = 0;
	off_t o;
	
	if (argc < 3)
	{
		fprintf(stderr, "usage: %s INFILE OUTFILE\n", argv[0]);
		return 255;
	}
	
	for (i = 1; i < argc - 2; i++)
	{
		if (!strcmp(argv[i], "-fixed"))
		{
			fwidth = 1;
			continue;
		}
		if (!strncmp(argv[i], "-ts=", 4))
		{
			tspc = atoi(argv[i] + 4);
			continue;
		}
		if (!strncmp(argv[i], "-bs=", 4))
		{
			bspc = atoi(argv[i] + 4);
			continue;
		}
		errx(1, "bad option \"%s\"\n", argv[i]);
	}
	
	infile = fopen(argv[i], "r");
	if (!infile)
		err(1, "%s", argv[i]);
	
	height = get_val("height");
	o = ftello(infile);
	count_glyphs();
	
	totw +=  31;
	totw &= ~31;
	
	bmp = calloc(height + tspc + bspc, totw / 8);
	if (bmp == NULL)
		err(1, "calloc");
	
	outfile = fopen(argv[i + 1], "w");
	if (!outfile)
		err(1, "%s", argv[i + 1]);
	
	fseeko(infile, o, SEEK_SET);
	while (conv_chr(i++));
	end_chr();
	
	if (fwrite(bmp, totw / 8, height + tspc + bspc, outfile) != height + tspc + bspc)
	{
		if (ferror(outfile))
			err(1, "output");
		errx(1, "output: Short write");
	}
	
	return 0;
}
