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

#include <unistd.h>
#include <string.h>
#include <os386.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

static int f_flag;

int main(int argc, char **argv)
{
	char buf[DMESG_BUF];
	int start;
	int end;
	int i;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		printf("\nUsage: %s [-f]\n"
		       "System messages.\n\n"
		       "  -f enable follow mode\n\n");
		return 0;
	}
	
	while (i = getopt(argc, argv, "f"), i > 0)
		switch (i)
		{
		case 'f':
			f_flag = 1;
			break;
		default:
			return 127;
		}
	
	end = _dmesg(buf, sizeof buf);
	if (end < 0)
		err(errno, "_dmesg");
	
	for (i = end; i < sizeof buf; i++)
		if (buf[i])
			fputc(buf[i], stdout);
	for (i = 0; i < end; i++)
		if (buf[i])
			fputc(buf[i], stdout);
	
	if (f_flag)
		for (;;)
		{
			sleep(1);
			
			start = end;
			end = _dmesg(buf, sizeof buf);
			if (end < 0)
				err(errno, "_dmesg");
			
			for (i = start; i != end; i++, i %= sizeof buf)
				if (buf[i])
					fputc(buf[i], stdout);
			fflush(stdout);
		}
	return 0;
}
