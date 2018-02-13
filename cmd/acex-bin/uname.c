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

#include <sys/utsname.h>
#include <gitver.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

static int sysname;
static int nodename;
static int release;
static int version;
static int machine;
static int gitver;

static void procopt(int argc, char **argv)
{
	int opt;
	
	while (opt = getopt(argc, argv, "asnrvmg"), opt > 0)
		switch (opt)
		{
		case 'a':
			sysname	 = 1;
			nodename = 1;
			release	 = 1;
			version	 = 1;
			machine	 = 1;
			break;
		case 's':
			sysname = 1;
			break;
		case 'n':
			nodename = 1;
			break;
		case 'r':
			release = 1;
			break;
		case 'v':
			version = 1;
			break;
		case 'm':
			machine = 1;
			break;
		case 'g':
			gitver = 1;
			break;
		default:
			exit(255);
		}
}

static void usage(void)
{
	printf("\nUsage: uname [-1adilqCD] [FILE...]\n"
		"Print system information.\n\n"
		"  -a equivalent to -snrvm flags\n"
		"  -s print the operating system name\n"
		"  -n print the hostname\n"
		"  -r print the operating system version\n"
		"  -v print the operating system build date\n"
		"  -m print the processor type\n"
		"  -g print the source code version\n\n"
		);
	exit(0);
}

int main(int argc, char **argv)
{
	struct utsname u;
	char *s = "";
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
		usage();
	
	if (uname(&u))
		err(errno, "utsname");
	if (argc < 2)
		sysname = 1;
	procopt(argc, argv);
	
	if (sysname)
	{
		printf("%s", u.sysname);
		s = " ";
	}
	if (nodename)
	{
		printf("%s%s", s, u.nodename);
		s = " ";
	}
	if (release)
	{
		printf("%s%s", s, u.release);
		s = " ";
	}
	if (version)
	{
		printf("%s%s", s, u.version);
		s = " ";
	}
	if (machine)
	{
		printf("%s%s", s, u.machine);
		s = " ";
	}
	if (gitver)
	{
		printf("%s%s", s, GITVER);
		s = " ";
	}
	putc('\n', stdout);
	return 0;
}
