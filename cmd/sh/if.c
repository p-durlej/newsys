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

#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <err.h>

#define ERR_FALSE	1
#define ERR_GENERIC	2
#define ERR_NOEXEC	125
#define ERR_EXEC	126
#define ERR_NOENT	127

static char **arg;

static int eval(int prio);

static int do_d(const char *a0, const char *a1)
{
	struct stat st;
	
	if (stat(a0, &st) || !S_ISDIR(st.st_mode))
		return 0;
	return 1;
}

static int do_e(const char *a0, const char *a1)
{
	if (access(a0, 0))
		return 0;
	return 1;
}

static int do_f(const char *a0, const char *a1)
{
	struct stat st;
	
	if (stat(a0, &st) || !S_ISREG(st.st_mode))
		return 0;
	return 1;
}

static int do_h(const char *a0, const char *a1)
{
	struct stat st;
	
	if (stat(a0, &st) || !S_ISLNK(st.st_mode))
		return 0;
	return 1;
}

static int do_r(const char *a0, const char *a1)
{
	if (access(a0, R_OK))
		return 1;
	return 0;
}

static int do_s(const char *a0, const char *a1)
{
	struct stat st;
	
	if (stat(a0, &st) || !st.st_size)
		return 0;
	return 1;
}

static int do_t(const char *a0, const char *a1)
{
	return isatty(atoi(a0));
}

static int do_w(const char *a0, const char *a1)
{
	if (access(a0, W_OK))
		return 1;
	return 0;
}

static int do_x(const char *a0, const char *a1)
{
	if (access(a0, X_OK))
		return 1;
	return 0;
}

static int do_ef(const char *a0, const char *a1)
{
	struct stat st0, st1;
	
	if (stat(a0, &st0) || stat(a1, &st1))
		return 0;
	if (st0.st_ino != st1.st_ino)
		return 0;
	if (st0.st_dev != st1.st_dev)
		return 0;
	return 1;
}

static int do_nt(const char *a0, const char *a1)
{
	struct stat st0, st1;
	
	if (stat(a0, &st0) || stat(a1, &st1))
		return 0;
	return st0.st_mtime > st1.st_mtime;
}

static int do_ot(const char *a0, const char *a1)
{
	struct stat st0, st1;
	
	if (stat(a0, &st0) || stat(a1, &st1))
		return 0;
	return st0.st_mtime < st1.st_mtime;
}

static int do_seq(const char *a0, const char *a1)
{
	return !strcmp(a0, a1);
}

static int do_sne(const char *a0, const char *a1)
{
	return !!strcmp(a0, a1);
}

static void __attribute__((noreturn)) err_exec(const char *pathname)
{
	switch (errno)
	{
	case ENOEXEC:
		err(ERR_NOEXEC, "%s", pathname);
	case ENOENT:
		err(ERR_NOENT, "%s", pathname);
	default:
		;
	}
	err(ERR_EXEC, "%s", pathname);
}

static struct prim
{
	int	(*proc)(const char *a0, const char *a1);
	int	argc;
	char *	str;
} prims[] =
{
	{ do_d,		1, "-d",	},
	{ do_e,		1, "-e",	},
	{ do_f,		1, "-f",	},
	{ do_h,		1, "-h",	},
	{ do_r,		1, "-r",	},
	{ do_s,		1, "-s",	},
	{ do_t,		1, "-t",	},
	{ do_w,		1, "-w",	},
	{ do_x,		1, "-x",	},
	{ do_ef,	2, "-ef",	},
	{ do_nt,	2, "-nt",	},
	{ do_ot,	2, "-ot",	},
	{ do_seq,	2, "=",		},
	{ do_sne,	2, "!=",	},
};

static int comm(void)
{
	int nest = 1;
	pid_t pid;
	int st;
	char **s;
	
	for (s = ++arg; nest; arg++)
	{
		if (*arg == NULL)
			errx(1, "Missing \"}\".");
		if (!strcmp(*arg, "{"))
		{
			nest++;
			continue;
		}
		if (!strcmp(*arg, "}"))
		{
			nest--;
			continue;
		}
	}
	if (s + 1 >= arg)
		errx(ERR_FALSE, "Missing command.");
	
	pid = fork();
	if (pid < 0)
		err(ERR_EXEC, "fork");
	if (!pid)
	{
		arg[-1] = NULL;
		execvp(*s, s);
		err_exec(*s);
	}
	while (wait(&st) != pid);
	if (!st)
		return 1;
	return 0;
}

static int evalp(void)
{
	int neg = 0;
	int i;
	
	while (arg[0] != NULL && !strcmp(arg[0], "!"))
	{
		neg = !neg;
		arg++;
	}
	
	if (arg[0] == NULL || arg[1] == NULL)
		goto eox;
	
	if (!strcmp(arg[0], "{"))
		return comm() ^ neg;
	
	if (!strcmp(arg[0], "("))
	{
		arg++;
		i = eval(0);
		if (arg[0] == NULL)
			goto eox;
		if (strcmp(arg[0], ")"))
			errx(1, "Missing \")\".");
		arg++;
		return i;
	}
	
	for (i = 0; i < sizeof prims / sizeof *prims; i++)
		if (prims[i].argc == 1 && !strcmp(prims[i].str, arg[0]))
		{
			arg += 2;
			return prims[i].proc(arg[-1], NULL) ^ neg;
		}
	if (arg[2] != NULL)
		for (i = 0; i < sizeof prims / sizeof *prims; i++)
			if (prims[i].argc == 2 && !strcmp(prims[i].str, arg[1]))
			{
				arg += 3;
				return prims[i].proc(arg[-3], arg[-1]) ^ neg;
			}
	errx(1, "\"%s\" unexpected.", arg[0]);
eox:
	errx(1, "Unexpected end of expression.", arg[0]);
}

static int eval(int prio)
{
	int r;
	
	r = evalp();
	while (*arg != NULL)
	{
		if (!strcmp(*arg, "-a"))
		{
			arg++;
			r &= eval(1);
			continue;
		}
		if (!prio && !strcmp(*arg, "-o"))
		{
			arg++;
			r |= eval(1);
			continue;
		}
		break;
	}
	return r;
}

int main(int argc, char **argv)
{
	arg = argv + 1;
	if (eval(0))
	{
		if (arg[0] == NULL)
			errx(1, "Missing command.");
		execvp(arg[0], arg);
		err_exec(arg[0]);
	}
	return 1;
}
