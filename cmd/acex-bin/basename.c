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
#include <stdio.h>
#include <err.h>

int main(int argc, char **argv)
{
	char *base;
	int sl;
	int l;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		printf("\nUsage: basename PATHNAME [SUFFIX]\n\n"
			"Print PATHNAME with directory components stripped,\n"
			"removing trailing SUFFIX, if any.\n\n");
		return 0;
	}
	
	if (argc < 2)
		errx(255, "too few arguments");
	
	if (argc > 3)
		errx(255, "too many arguments");
	
	if (!strcmp(argv[1], "/"))
	{
		puts("/");
		return 0;
	}
	
	base = strrchr(argv[1], '/');
	if (base)
		base++;
	else
		base = argv[1];
	
	if (argc == 3)
	{
		l  = strlen(base);
		sl = strlen(argv[2]);
		if (l > sl && !strcmp(argv[2], base + l - sl))
			base[l - sl] = 0;
	}
	
	puts(base);
	return 0;
}
