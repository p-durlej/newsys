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

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

void show_input(char *buf, int cnt)
{
	int i;
	
	printf("%i bytes read:\n", cnt);
	for (i = 0; i < cnt; i++)
		switch(buf[i])
		{
		case '\b':
			printf("[BS] ");
			break;
		case '\n':
			printf("[NL] ");
			break;
		default:
			printf("[%c] ", buf[i]);
		}
	printf("\n");
}

void set_flags(int flags)
{
	struct termios ti;
	
	tcgetattr(0, &ti);
	ti.c_lflag = flags;
	tcsetattr(0, TCSANOW, &ti);
}

int main()
{
	struct termios ts;
	char buf[256];
	int cnt;
	
	tcgetattr(0, &ts);
	
	set_flags(0);
	
	printf("Echo and editing should be disabled. Press keys, terminate\n");
	printf("with 'q'.\n\n");
	
	do
	{
		cnt = read(0, buf, sizeof buf);
		if (cnt < 0)
			perror("stdin");
		else
			show_input(buf, cnt);
	} while(!memchr(buf, 'q', cnt));
	
	set_flags(ECHO);
	
	printf("Now echo should be enabled. Press keys, terminate\n");
	printf("with 'q'.\n\n");
	
	do
	{
		cnt = read(0, buf, sizeof buf);
		if (cnt < 0)
			perror("stdin");
		else
			show_input(buf, cnt);
	} while(!memchr(buf, 'q', cnt));
	
	set_flags(ICANON);
	
	printf("Canonical input without echo should be enabled. Enter a\n");
	printf("string.\n\n");
	
	cnt = read(0, buf, sizeof buf);
	if (cnt < 0)
		perror("stdin");
	else
		show_input(buf, cnt);
	
	tcsetattr(0, TCSANOW, &ts);
	return 0;
}
