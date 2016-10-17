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

#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <err.h>

int main(int argc, char **argv)
{
	char buf[80];
	unsigned v;
	char *p;
	int i;
	
	if (argc != 2)
		errx(1, "wrong nr of args");
	
	printf("char %s[] =\n{", argv[1]);
	i = 0;
	
	while (fgets(buf, sizeof buf, stdin))
	{
		p = strchr(buf, '\n');
		if (p)
			*p = 0;
		
		if (!*buf)
			continue;
		
		v = 0;
		for (p = buf; *p; p++)
		{
			v <<= 1;
			if (*p == '*')
				v |= 1;
			else if (*p != '.')
				goto nextln;
		}
		
		if (!i++)
			fputs("\n\t", stdout);
		i %= 8;
		
		printf("0x%02x, ", v);
nextln:
		;
	}
	
	printf("\n};\n");
	return 0;
}
