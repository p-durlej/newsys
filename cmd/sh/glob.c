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

#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <err.h>

#define DEBUG	0

#if DEBUG
#define dbgprintf(...)	fprintf(stderr, __VA_ARGS__)
#else
#define dbgprintf(...)
#endif

static char	pathbuf[PATH_MAX];

static char **	n_argv;
static int	n_argc;

static void add_arg(const char *arg);
static int  pncat(char *buf, const char *dir, const char *ent, size_t buf_sz);
static int  match(const char *filename, const char *pat, size_t plen);
static int  do_glob_1(const char *pat, size_t plen);
static int  do_glob_sub(const char *arg);
static int  do_glob(const char *arg);

static void add_arg(const char *arg)
{
	n_argv = realloc(n_argv, sizeof *n_argv * (n_argc + 2));
	if (n_argv == NULL)
		err(1, "");
	n_argv[n_argc++] = (char *)arg;
	n_argv[n_argc  ] = NULL;
}

static int pncat(char *buf, const char *dir, const char *ent, size_t buf_sz)
{
	size_t dir_len;
	size_t ent_len;
	size_t tot_len;
	
	dir_len = strlen(dir);
	ent_len = strlen(ent);
	tot_len = dir_len + ent_len;
	if (dir_len && dir[dir_len - 1] != '/');
		tot_len++;
	if (tot_len >= buf_sz || tot_len < dir_len)
	{
		errno = ENAMETOOLONG;
		return -1;
	}
	if (buf != dir)
		memcpy(buf, dir, dir_len + 1);
	if (dir_len && dir[dir_len - 1] != '/')
		strcat(buf, "/");
	strcat(buf, ent);
	return 0;
}

static int match(const char *filename, const char *pat, size_t plen)
{
	const char *p;
	
	dbgprintf("match(\"%s\", \"%s\", %u);\n", filename, pat, plen);
	if (!strcmp(filename, ".") || !strcmp(filename, ".."))
		return 0;
	
	while (plen)
	{
		switch (*pat)
		{
		case '*':
			while (plen && *pat == '*')
			{
				plen--;
				pat++;
			}
			if (!plen)
				return 1;
			for (p = filename; *p; p++)
				if (match(p, pat, plen))
					return 1;
			continue;
		case '?':
			if (!*filename)
				return 0;
			break;
		default:
			if (*pat != *filename)
				return 0;
		}
		filename++;
		pat++;
		plen--;
	}
	if (*filename)
		return 0;
	return 1;
}

static int do_glob_1(const char *pat, size_t plen)
{
	size_t prefix_len;
	struct dirent *de;
	const char *odn;
	const char *p;
	int m = 0;
	DIR *d;
	
	prefix_len = strlen(pathbuf);
	
	dbgprintf("do_glob_1(\"%s\", %u): pathbuf = \"%s\"\n", pat, plen, pathbuf);
	while (*pat == '/')
	{
		plen--;
		pat++;
	}
	
	odn = pathbuf;
	if (!*odn)
		odn = ".";
	
	d = opendir(odn);
	if (d == NULL)
		return 0; /* XXX error */
	while (errno = 0,  de = readdir(d),  de != NULL)
		if (match(de->d_name, pat, plen))
		{
			dbgprintf("matched: \"%s\", \"%s\"\n", de->d_name, pat);
			if (pncat(pathbuf, pathbuf, de->d_name, sizeof pathbuf)) /* XXX pathname too long */
				continue;
			p = pat + plen;
			if (do_glob_sub(p))
				m = 1;
			pathbuf[prefix_len] = 0;
		}
	closedir(d);
	return m;
}

static int do_glob_sub(const char *arg)
{
	const char *dir_s, *dir_e, *pat_s, *pat_e;
	size_t prefix_len;
	size_t dir_len;
	size_t pat_len;
	char *p;
	int m;
	
	dbgprintf("do_glob_sub(\"%s\"): pathbuf = \"%s\"\n", arg, pathbuf);
	
	prefix_len = strlen(pathbuf);
	for (dir_s = arg; *dir_s == '/'; dir_s++);
	dir_e = dir_s + strcspn(dir_s, "*?[");
	if (!*dir_e)
	{
		if (prefix_len + strlen(arg) >= sizeof pathbuf)
			return 0;  /* XXX pathname too long */
		strcat(pathbuf, arg);
		if (!access(pathbuf, 0))
		{
			p = strdup(pathbuf);
			if (p == NULL)
				err(1, "");
			add_arg(p);
			return 1;
		}
		pathbuf[prefix_len] = 0;
		return 0;
	}
	while (dir_e > dir_s && *dir_e != '/')
		dir_e--;
	dir_len = dir_e - dir_s;
	
	for (pat_s = dir_e; *pat_s == '/'; pat_s++);
	for (pat_e = pat_s; *pat_e != '/' && *pat_e; pat_e++);
	pat_len = pat_e - pat_s;
	
	dbgprintf("dir_s - arg = %u\n", dir_s - arg);
	dbgprintf("dir_e - dir_s  = %u\n", dir_len);
	dbgprintf("pat_e - pat_s  = %u\n", pat_e - pat_s);
	if (prefix_len + pat_len >= sizeof pathbuf) /* XXX overflow */
		return 0; /* XXX error */
	memcpy(pathbuf + prefix_len, dir_s, dir_len);
	pathbuf[prefix_len + dir_len] = 0;
	
	m = do_glob_1(pat_s, pat_e - pat_s);
	pathbuf[prefix_len] = 0;
	return m;
}

static int do_glob(const char *arg)
{
	dbgprintf("do_glob(\"%s\");\n", arg);
	*pathbuf = 0;
	if (*arg == '/')
		strcpy(pathbuf, "/");
	if (do_glob_sub(arg))
		return 1;
	add_arg(arg);
	return 0;
}

int main(int argc, char **argv)
{
	char *p;
	int i;
	
	if (argc < 2)
		errx(1, "wrong nr of args");
	add_arg(argv[1]);
	
	for (i = 2; i < argc; i++)
	{
		p = argv[i] + strcspn(argv[i], "*?[");
		if (*p)
			do_glob(argv[i]);
		else
			add_arg(argv[i]);
	}
	execvp(argv[1], n_argv);
	err(1, "%s", argv[1]);
}
