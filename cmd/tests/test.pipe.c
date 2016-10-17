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

#include <sys/wait.h>
#include <newtask.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <os386.h>
#include <err.h>

#define VERBOSE	0

static unsigned buf[4000];

static ssize_t x_read(int fd, void *buf, size_t len)
{
	size_t resid = len;
	char *p = buf;
	ssize_t cnt;
	
	while (resid)
	{
		cnt = read(fd, p, resid);
		if (cnt < 0)
			return -1;
		
		if (!cnt)
			return len - resid;
		
		resid -= cnt;
		p += cnt;
	}
	
	return len;
}

static void wait_slave(void)
{
	int status;
	
	while (wait(&status) < 1);
	printf("slave exited with status 0x%04x\n", status);
}

int main(int argc, char **argv)
{
	int saved_out;
	int fd[2];
	int cnt;
	int x = 0;
	int i;
	
	for (i = 0; i < sizeof buf / sizeof *buf; i++)
		buf[i] = i;
	
	if (argc > 1)
	{
		errno = EINVAL;
		if (write(1, buf, sizeof buf) != sizeof buf)
			err(1, "writing to pipe");
#if VERBOSE
		warnx("write to pipe ok");
#endif
		exit(0);
	}
	
#if VERBOSE
	warnx("PIPE_BUF = %i", PIPE_BUF);
#endif
	
	if (pipe(fd))
	{
		perror("pipe");
		return 1;
	}
	
	saved_out = dup(1);
	dup2(fd[1], 1);
	
	if (_newtasklp("test.pipe", "test.pipe", "slave", (void *)NULL) < 0)
		err(1, "_newtaskl");
	
	dup2(saved_out, 1);
	close(fd[1]);
	
#if VERBOSE
	printf("reading pipe...\n");
#endif
	cnt = x_read(fd[0], buf, sizeof buf);
	if (cnt < 0)
	{
		warn("reading pipe");
		wait_slave();
		return 1;
	}
	
	for (i = 0; i < sizeof buf / sizeof *buf; i++)
		if (buf[i] != i)
		{
			warnx("mismatch, buf[%i] = %u", i, buf[i]);
			x = 1;
		}
	
	wait_slave();
	return x;
}
