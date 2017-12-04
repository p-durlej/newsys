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
#include <err.h>

#define CC	"/usr/bin/cc"

static char *output;
static int cflag;
static int kflag;
static int mflag;

int main(int argc, char **argv)
{
	char **cargs;
	char **spp;
	char **pp;
	pid_t pid;
	int st;
	int c;
	
	while (c = getopt(argc, argv, "kmco:"), c > 0)
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
		default:
			return 1;
		}
	
	argv += optind;
	argc -= optind;
	
	if (!argc)
		errx(1, "wrong nr of args");
	
	if (output == NULL)
	{
		char *p;
		
		p = strchr(argv[0], '.');
		if (p == NULL || !p[1])
			errx(1, "no output name");
		
		output = strdup(argv[0]);
		if (output == NULL)
			err(1, NULL);
		
		p = strchr(output, '.');
		if (cflag)
		{
			p[1] = 'o';
			p[2] = 0;
		}
		else
			*p = 0;
	}
	
	pp = cargs = calloc(argc + 8, sizeof *cargs);
	if (cargs == NULL)
		err(1, NULL);
	
	*pp++ = CC;
	
	if (kflag)
		*pp++ = "-D_KERN_";
	if (cflag)
		*pp++ = "-c";
	
	*pp++ = "-o";
	*pp++ = output;
	
	for (spp = argv; *spp != NULL; spp++)
		*pp++ = *spp;
	*pp = NULL;
	
	signal(SIGCHLD, SIG_DFL);
	
	pid = _newtaskv(CC, cargs);
	if (pid < 0)
		err(1, "%s", CC);
	
	while (wait(&st) != pid)
		;
	
	if (WIFEXITED(st))
		return WEXITSTATUS(st);
	return WTERMSIG(st) + 128;
}
