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

#include <wingui.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

static void show_modes(void)
{
	struct win_modeinfo mi;
	int cmode;
	int i;
	
	if (win_getmode(&cmode))
		err(1, "win_getmode");
	
	printf("  NR XRES YRES   COLORS CTE\n");
	for (i = 0; !win_modeinfo(i, &mi); i++)
	{
		if (i == cmode)
			putchar('*');
		else
			putchar(' ');
		
		printf(" %2i %4i %4i %8i %3i\n",
			i,
			mi.width,
			mi.height,
			mi.ncolors,
			mi.user_cte_count);
	}
}

static void usage(const char *pn)
{
	fprintf(stderr, "Usage: %s XRESxYRES[xNCOLORS]\n", pn);
	fprintf(stderr, "       %s MODE_NUMBER\n", pn);
	fprintf(stderr, "       %s\n", pn);
	exit(0);
}

static void setmodenr(int nr)
{
	if (win_setmode(nr, 0))
		err(1, "win_setmode");
}

static void setmodexyc(int w, int h, int c)
{
	struct win_modeinfo mi;
	int i;
	
	if (c <= 1)
	{
		if (win_modeinfo(-1, &mi))
			err(1, "win_modeinfo");
		c = mi.ncolors;
	}
	
	for (i = 0; !win_modeinfo(i, &mi); i++)
		if (mi.width == w && mi.height == h && mi.ncolors == c)
		{
			setmodenr(i);
			return;
		}
	errx(1, "%i,%i,%i: Mode not found", w, h, c);
}

static void setmode(const char *mstr)
{
	char *s1, *s2, *s3;
	int v1, v2, v3;
	char *s;
	
	s = strdup(mstr);
	
	s1 = strtok(s,	  "x,");
	s2 = strtok(NULL, "x,");
	s3 = strtok(NULL, "x,");
	
	v1 = s1 ? atoi(s1) : -1;
	v2 = s2 ? atoi(s2) : -1;
	v3 = s3 ? atoi(s3) : -1;
	
	if (v2 < 0)
	{
		setmodenr(v1);
		return;
	}
	
	setmodexyc(v1, v2, v3);
}

int main(int argc, char **argv)
{
	if (argc == 2 && !strcmp(argv[1], "--help"))
		usage(argv[0]);
	
	if (win_attach())
		err(255, NULL);
	
	if (argc < 2)
	{
		show_modes();
		return 0;
	}
	
	setmode(argv[1]);
	return 0;
}
