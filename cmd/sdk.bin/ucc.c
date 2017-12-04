/* Copyright (c) 2017, Piotr Durlej
 * All rights reserved.
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <newtask.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <err.h>

#define MGEN	"/usr/bin/mgen"
#define CC	"/usr/bin/cc"

static char *output;
static int cflag;
static int kflag;
static int mflag;
static int vflag;
static int xflag;

static int spawn(char *pathname, char **argv)
{
	pid_t pid;
	int st;
	int i;
	
	signal(SIGCHLD, SIG_DFL);
	
	if (vflag)
	{
		for (i = 0; argv[i]; i++)
			fprintf(stderr, "%s ", argv[i]);
		fputc('\n', stderr);
	}
	
	pid = _newtaskv(pathname, argv);
	if (pid < 0)
		err(1, "%s", CC);
	
	while (wait(&st) != pid)
		;
	
	if (WIFEXITED(st))
		return WEXITSTATUS(st);
	return WTERMSIG(st) + 128;
}

static int ccld(char *output, char **input, int cflag)
{
	char **cargs;
	char **spp;
	char **pp;
	int cnt;
	
	for (cnt = 0; input[cnt]; cnt++)
		;
	
	pp = cargs = calloc(cnt + 16, sizeof *cargs);
	if (cargs == NULL)
		err(1, NULL);
	
	*pp++ = CC;
	
	if (kflag)
		*pp++ = "-D_KERN_";
	if (cflag)
		*pp++ = "-c";
	
	*pp++ = "-o";
	*pp++ = output;
	
	for (spp = input; *spp != NULL; spp++)
		*pp++ = *spp;
	*pp = NULL;
	
	return spawn(CC, cargs);
}

static int mkmod(char *output, char **input)
{
	char *args[] =
	{
		MGEN,
		output,
		"",
		NULL
	};
	char *partial;
	int st;
	
	if (asprintf(&partial, "%s.o", output) < 0)
		err(1, NULL);
	args[2] = partial;
	
	st = ccld(partial, input, 1);
	if (st)
		return st;
	
	return spawn(MGEN, args);
}

static int docc(char **input)
{
	char *out = output;
	int st;
	
	if (out == NULL)
	{
		char *p;
		
		p = strrchr(input[0], '.');
		if (p == NULL || !p[1])
			errx(1, "no output name");
		
		out = strdup(input[0]);
		if (out == NULL)
			err(1, NULL);
		
		p = strrchr(out, '.');
		if (cflag)
		{
			p[1] = 'o';
			p[2] = 0;
		}
		else
			*p = 0;
	}
	
	if (mflag && !cflag)
		st = mkmod(out, input);
	else
		st = ccld(out, input, cflag);
	
	return st;
}

int main(int argc, char **argv)
{
	char **pp;
	int st;
	int c;
	
	while (c = getopt(argc, argv, "kmco:vx"), c > 0)
		switch (c)
		{
		case 'k':
			kflag = 1;
			break;
		case 'm':
			kflag = 1;
			mflag = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		case 'o':
			output = optarg;
			break;
		case 'x':
			xflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		default:
			return 1;
		}
	
	argv += optind;
	argc -= optind;
	
	if (!argc)
		errx(1, "wrong nr of args");
	
	if (xflag)
	{
		for (pp = argv; *pp; pp++)
		{
			char *args[] = { *pp, NULL };
			
			st = docc(args);
			if (st)
				return st;
		}
		return 0;
	}
	
	return docc(argv);
}
