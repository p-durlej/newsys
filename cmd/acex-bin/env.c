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

#include <findexec.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

extern char **environ;

static int i_flag;

static int cmpenv(const char *e0, const char *e1)
{
	int i = 0;
	
	while (e0[i] == e1[i])
	{
		if (e0[i] == '=' || !e0[i])
			return 0;
		i++;
	}
	if (e0[i] < e1[i])
		return -1;
	return 1;
}

static int qcmpenv(const void *a0, const void *a1)
{
	return cmpenv(*(char **)a0, *(char **)a1);
}

static void dedup2(char **t0, char **t1, char **end)
{
	char **p0, **p1;
	int d;
	
	p0 = t0;
	p1 = t1;
	while (p0 < t0 && p1 < end)
	{
		d = cmpenv(*p0, *p1);
		if (!d)
		{
			*p1++ = NULL;
			continue;
		}
		if (d < 0)
			p0++;
		else
			p1++;
	}
}

static int dedup(char **tab, char **end)
{
	char **p0, **p1;
	
	for (p0 = tab; p0 + 1 < end; p0++)
	{
		if (p0[0] == NULL || p0[1] == NULL)
			continue;
		if (cmpenv(p0[0], p0[1]))
			continue;
		p0[1] = NULL;
	}
	
	for (p0 = p1 = tab; p1 < end; p1++)
		if (*p1 != NULL)
			*p0++ = *p1;
	*p0 = NULL;
	
	return p0 - tab;
}

int main(int argc, char **argv)
{
	const char *exec;
	char **nenv;
	int nenvc;
	int envs, enve;
	int envc;
	int i, n;
	int c;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		printf("\nUsage: %s [-i] [NAME=VAL...] CMD [ARG...]\n"
		       "Run command with modified environment.\n\n"
		       "Print the current environment.\n\n"
		       "  -i ignore inherited environment\n\n", argv[0]);
		return 0;
	}
	
	if (argc <= 1)
	{
		for (i = 0; environ[i] != NULL; i++)
			puts(environ[i]);
		return 0;
	}
	
	while (c = getopt(argc, argv, "i"), c > 0)
		switch (c)
		{
		case 'i':
			i_flag = 1;
			break;
		default:
			return 127;
		}
	
	envc = 0;
	while (environ[envc] != NULL)
		envc++;
	
	envs = enve = optind;
	while (enve < argc && strchr(argv[enve], '=') != NULL)
		enve++;
	
	if (enve >= argc)
		errx(127, "missing required parameter");
	
	if (i_flag)
	{
		nenvc = enve - envs;
		nenv  = calloc(sizeof *nenv, nenvc + 1);
		if (nenv == NULL)
			err(errno, "calloc");
		for (i = 0; i < enve - envs; i++)
			nenv[i] = argv[i + envs];
		qsort(nenv, nenvc, sizeof *nenv, qcmpenv);
	}
	else
	{
		nenvc = envc + enve - envs;
		nenv  = calloc(sizeof *nenv, nenvc + 1);
		if (nenv == NULL)
			err(errno, "calloc");
		for (n = envs, i = 0; n < enve; i++, n++)
			nenv[i] = argv[n];
		for (n = 0; n < envc; i++, n++)
			nenv[i] = environ[n];
		
		qsort(nenv, enve - envs, sizeof *nenv, qcmpenv);
		qsort(nenv + enve - envs, envc, sizeof *nenv, qcmpenv);
		dedup2(nenv, nenv + enve - envs, nenv + nenvc);
	}
	nenvc = dedup(nenv, nenv + nenvc);
	qsort(nenv, nenvc, sizeof *nenv, qcmpenv);
	
	exec = _findexec(argv[enve], NULL);
	if (exec == NULL)
		err(errno, "%s", argv[enve]);
	
	execve(exec, argv + enve, nenv); /* XXX execvpe */
	err(errno, "%s", argv[enve]);
}
