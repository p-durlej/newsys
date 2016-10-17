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

#include <unistd.h>
#include <stdio.h>
#include <os386.h>
#include <err.h>

int main(int argc, char **argv)
{
	int sflag = 0;
	int xflag = 0;
	char *ttyn;
	int c;
	
	while (c = getopt(argc, argv, "sx"), c > 0)
		switch (c)
		{
		case 's':
			sflag = 1;
			break;
		case 'x':
			xflag = 1;
			break;
		default:
			return 1;
		}
	
	ttyn = ttyname(0);
	if (ttyn == NULL)
	{
		if (!sflag)
			puts("not a tty");
		return 1;
	}
	if (!sflag)
	{
		if (xflag)
		{
			static struct pty_size tsz;
			
			if (ioctl(0, PTY_GSIZE, &tsz))
				err(1, "%s", ttyn);
			printf("%s: %i x %i\n", ttyn, tsz.w, tsz.h);
		}
		else
			puts(ttyn);
	}
	return 0;
}
