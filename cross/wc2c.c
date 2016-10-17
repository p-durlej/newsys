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

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	char buf[256];
	int r, g, b, a;
	uint32_t c;
	char *p, *p1;
	int i;
	
	while (fgets(buf, sizeof buf, stdin))
	{
		if (*buf == '#')
			continue;
		
		p = strchr(buf, '\n');
		if (p != NULL)
			*p = 0;
		
		p = strchr(buf, '=');
		if (p == NULL)
			continue;
		*p = 0;
		
		c = strtoul(p + 1, NULL, 0);
		
		printf("\t[WC_");
		for (p1 = buf; p1 < p; p1++)
			putchar(toupper(*p1));
		
		r = (c >> 16) & 255;
		g = (c >>  8) & 255;
		b =  c	      & 255;
		a = (c >> 24) & 255;
		
		printf("]\t= { %3i, %3i, %3i, %3i },\n", r, g, b, a);
	}
	return 0;
}
