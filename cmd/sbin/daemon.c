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

#include <newtask.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <os386.h>
#include <err.h>

static const char *io = NULL;
static int	   c_flag;

static void usage(FILE *f, const char *a0)
{
	fprintf(f, "\nUsage: daemon [-cfC] CMD [ARG...]\n"
		   "       bg [-cC] CMD [ARG...]\n"
		   "Run a background task.\n\n"
		   "  -c change into root directory\n"
		   "  -f redirect stdio to /dev/null (default when running as bg)\n"
		   "  -C redirect stdio to /dev/console\n\n", a0);
}

int main(int argc, char **argv)
{
	char *p;
	int id, od;
	int sed = 2;
	int se;
	int c;
	
	if (argc == 2 && !strcmp(argv[0], "--help"))
	{
		usage(stdout, argv[0]);
		return 0;
	}
	
	while (c = getopt(argc, argv, "cfC"), c > 0)
		switch (c)
		{
		case 'c':
			c_flag = 1;
			break;
		case 'f':
			io = "/dev/null";
			break;
		case 'C':
			io = "/dev/console";
			break;
		default:
			return 127;
		}
	
	if (optind >= argc)
	{
		usage(stderr, argv[0]);
		return 127;
	}
	
	p = strrchr(argv[0], '/');
	if (p != NULL)
		p++;
	else
		p = argv[0];
	
	if (!strcmp(p, "bg"))
		io = "/dev/null";
	
	if (c_flag && chdir("/"))
		err(errno, "/: chdir");
	
	if (_ctty(""))
		err(errno, "ctty");
	
	if (io != NULL)
	{
		id = open(io, O_RDONLY);
		if (id < 0)
			err(errno, "%s: open");
		
		od = open(io, O_WRONLY);
		if (od < 0)
			err(errno, "%s: open");
		
		sed = dup(2);
		if (sed < 0)
			err(errno, "stderr: dup");
		fcntl(sed, F_SETFD, 1);
		
		dup2(id, 0);
		dup2(od, 1);
		dup2(od, 2);
	}
	
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	
	if (_newtaskvp(argv[optind], argv + optind) < 0)
	{
		se = errno;
		if (sed != 2)
			dup2(sed, 2);
		errno = se;
		err(errno, "%s", argv[0]);
	}
	return 0;
}
