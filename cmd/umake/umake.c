/* Copyright (C) Piotr Durlej */

#define _GNU_SOURCE

#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <err.h>

#include "umake.h"

#define TIMESPEC	0

static char *mfnames[] =
{
	"umakefile",
	"Makefile",
	"makefile",
};

static const char *defmk = "/usr/mk/default.mk";
static const char *defpath = "/usr/mk";
static char *mfname;
static int sflag;
static int nflag;
static int rflag;
static int fail;
static int depth;

const char **incpaths;
int incpathcnt;
int vflag;

int makebyname(const char *name);

static char *mkinput(const char *target)
{
	static char buf[4096]; // XXX
	
	struct rule *r;
	const char *tx;
	char **sp;
	char *p;
	
	*buf = 0;
	
	for (r = rules; r; r = r->next)
		if (*r->output != '.' && !strcmp(r->output, target))
		{
			for (sp = r->input; *sp != NULL; sp++)
			{
				strcat(buf, *sp);
				strcat(buf, " ");
			}
			return buf;
		}
	
	tx = strrchr(target, '.');
	if (tx != NULL)
		for (r = rules; r; r = r->next)
		{
			if (*r->output != '.')
				continue;
			
			p = strchr(r->output + 1, '.');
			if (p == NULL)
				continue;
			
			if (strcmp(tx, p))
				continue;
			
			for (sp = r->input; *sp != NULL; sp++)
			{
				strcat(buf, *sp);
				strcat(buf, " ");
			}
			
			strcat(buf, target);
			*strrchr(buf, '.') = 0;
			
			strcat(buf, r->output);
			*strrchr(buf, '.') = 0;
			
			return buf;
		}
	
	for (r = rules; r; r = r->next)
	{
		if (*r->output != '.')
			continue;
		
		p = strchr(r->output + 1, '.');
		if (p != NULL)
			continue;
		
		for (sp = r->input; *sp != NULL; sp++)
		{
			strcat(buf, *sp);
			strcat(buf, " ");
		}
		
		strcat(buf, target);
		strcat(buf, r->output);
		
		return buf;
	}
	return buf;
}

int docmd(const char *cmd, const char *target, struct rule *rule)
{
	struct rule *r;
	char buf[4096];
	char *sbuf;
	const char *sp;
	int noecho = 0;
	int iexit = 0;
	int f = 0;
	char *dp;
	
	sbuf = substvars(cmd);
	
	for (dp = buf, sp = sbuf; *sp; sp++)
		switch (*sp)
		{
		case '$':
			switch (*++sp)
			{
			case '@':
				strcpy(dp, target);
				dp += strlen(target);
				break;
			case '<':
				strcpy(dp, mkinput(target));
				dp = strchr(dp, 0);
				break;
			default:
				*dp++ = '$';
				*dp++ = *sp;
			}
			break;
		default:
			*dp++ = *sp;
		}
	
	sp = buf;
	while (*sp == '-' || *sp == '@')
		switch (*sp++)
		{
		case '-':
			iexit = 1;
			break;
		case '@':
			noecho = 1;
			break;
		}
	*dp++ = 0;
	
	if (!noecho && !sflag)
	{
		fputs(sp, stderr);
		fputc('\n', stderr);
	}

	if (nflag)
		return 0;

	f = !!system(sp);
	if (iexit)
		return 0;
	return f;
}

static void trace(struct rule *r, const char *src, const char *target)
{
	char **input;

	if (!vflag)
		return;
	
	if (src == NULL)
		src = "-";
	
	warnx("making %s: %s (%s)", target, src, r->output);
	if (r->input)
		for (input = r->input; *input; input++)
			warnx("making %s: input %s", target, *input);
}

#if TIMESPEC
static int older(const struct timespec *ts1, const struct timespec *ts2)
{
	if (ts1->tv_sec < ts2->tv_sec)
		return 1;
	if (ts1->tv_sec > ts2->tv_sec)
		return 0;
	return ts1->tv_nsec < ts2->tv_nsec;
}
#else
static int older(time_t t1, time_t t2)
{
	return t1 < t2;
}
#endif

int make(struct rule *r, const char *src, const char *target)
{
#if TIMESPEC
	struct timespec stv = { 0, 0 };
#else
	time_t stv;
#endif
	struct stat st;
	char **input;
	char **cmd;
	int f = 0;
	
	if (r->done)
		return 0;
	
	if (++depth > 10)
	{
		warnx("recursion limit exceeded");
		return 1;
	}
	
	if (target == NULL)
		target = r->output;
	
	if (src == NULL && r->input)
		src = r->input[0];
	
	trace(r, src, target);
	
	for (input = r->input; *input; input++)
	{
		f |= makebyname(*input);
#if TIMESPEC
		if (!stat(*input, &st) && older(&stv, &st.st_mtim))
			stv = st.st_mtim;
#else
		if (!stat(*input, &st) && older(stv, st.st_mtime))
			stv = st.st_mtime;
#endif
	}
#if TIMESPEC
	if (!stat(src, &st) && older(&stv, &st.st_mtim))
		stv = st.st_mtim;
#else
	if (!stat(src, &st) && older(stv, st.st_mtime))
		stv = st.st_mtime;
#endif
	
	if (f)
	{
		depth--;
		return 1;
	}
	
#if TIMESPEC
	if (src != NULL && !stat(target, &st) && !older(&st.st_mtim, &stv))
	{
		depth--;
		return 0;
	}
#else
	if (src != NULL && !stat(target, &st) && !older(st.st_mtime, stv))
	{
		depth--;
		return 0;
	}
#endif
	
	if (r->cmds)
		for (cmd = r->cmds; *cmd; cmd++)
			docmd(*cmd, target, r);
	
	if (*r->output != '.')
		r->done = 1;
	
	depth--;
	return 0;
}

int makebyname(const char *name)
{
	struct rule *r;
	const char *tx;
	const char *p;
	size_t blen;
	size_t xlen;
	char *src;
	
	for (r = rules; r; r = r->next)
		if (*r->output != '.' && !strcmp(r->output, name))
		{
			if (make(r, NULL, NULL))
				return -1;
			if (r->cmds)
				return 0;
		}
	
	tx = strrchr(name, '.');
	if (tx != NULL)
		for (r = rules; r; r = r->next)
		{
			if (*r->output != '.')
				continue;
			
			p = strchr(r->output + 1, '.');
			if (p == NULL)
				continue;
			
			if (strcmp(tx, p))
				continue;
			
			xlen = p - r->output;
			blen = tx - name;
			
			src = malloc(blen + p - r->output + 1);
			if (src == NULL)
				err(1, NULL);
			
			memcpy(src, name, tx - name);
			memcpy(src + blen, r->output, xlen);
			src[blen + xlen] = 0;
			
			if (make(r, src, name))
				return -1;
			if (r->cmds)
				return 0;
		}
	
	for (r = rules; r; r = r->next)
	{
		if (*r->output != '.')
			continue;
		
		p = strchr(r->output + 1, '.');
		if (p != NULL)
			continue;
		
		xlen = p - r->output;
		blen = tx - name;
		
		if (asprintf(&src, "%s%s", name, r->output) < 0)
			err(1, NULL);
		
		if (make(r, src, name))
			return -1;
		if (r->cmds)
			return 0;
	}
	
	if (!access(name, 0))
		return 0;
	
	warnx("%s: No rule to make target", name);
	errno = EINVAL;
	fail = 1;
	return -1;
}

static void linkrule(struct rule *r)
{
	const char *output = r->output;
	const char *oext;
	struct rule *r1;
	
	oext = strrchr(output, '.');
	for (r1 = r->next; r1 != NULL; r1 = r1->next)
	{
		if (oext == NULL || strcmp(r1->output, oext))
			continue;
		if (strcmp(r1->output, output))
			continue;
		
		r->chain = r1;
		break;
	}
}

static void linkrules(void)
{
	struct rule *r;
	
	for (r = rules; r != NULL; r = r->next)
		linkrule(r);
}

static void addpath(const char *path)
{
	incpaths = realloc(incpaths, ++incpathcnt * sizeof *incpaths);
	if (incpaths == NULL)
		err(1, NULL);
	incpaths[incpathcnt - 1] = path;
}

static void rdump1(struct rule *r)
{
	char **pp;
	
	printf("%s:", r->output);
	if (r->input)
		for (pp = r->input; *pp; pp++)
			printf(" %s", *pp);
	fputc('\n', stdout);
	
	if (r->cmds)
		for (pp = r->cmds; *pp; pp++)
			printf("\t%s\n", *pp);
}

static void rdump(void)
{
	struct rule **rr;
	struct rule *r;
	int cnt;
	int i;
	
	for (cnt = 0, r = rules; r != NULL; r = r->next)
		cnt++;
	
	rr = calloc(cnt, sizeof *rr);
	for (i = cnt, r = rules; r; r = r->next)
		rr[--i] = r;
	
	for (i = 0; i < cnt; i++)
		rdump1(rr[i]);
}

int main(int argc, char **argv)
{
	struct utsname un;
	struct rule *r;
	char *p;
	int i;
	int c;
	
	for (i = 0; i < sizeof mfnames; i++)
		if (!access(mfnames[i], 0))
		{
			mfname = mfnames[i];
			break;
		}
	
	while (c = getopt(argc, argv, "nrsvd:f:i:I:"), c > 0)
		switch (c)
		{
		case 'i':
			defpath = optarg;
			break;
		case 'I':
			addpath(optarg);
			break;
		case 'd':
			defmk = optarg;
			break;
		case 'f':
			mfname = optarg;
			break;
		case 's':
			sflag = 1;
			break;
		case 'n':
			nflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		case 'r':
			rflag = 1;
			break;
		default:
			return 1;
		}
	
	if (*defpath)
		addpath(defpath);
	
	argv += optind;
	argc -= optind;
	
	uname(&un);
	setvar("MACH", un.machine);
	p = strchr(un.machine, '-');
	if (p)
		*p = 0;
	setvar("ARCH", un.machine);
	
	if (load(defmk))
		return 1;
	if (load(mfname))
		return 1;
	linkrules();
	
	if (rflag)
	{
		rdump();
		return 0;
	}
	
	if (!argc)
	{
		struct rule *def = NULL;
		
		for (r = rules; r->next; r = r->next)
			if (*r->output != '.')
				def = r;
		
		if (def == NULL)
			errx(1, "no default target");
		make(def, NULL, NULL);
	}
	
	for (i = 0; i < argc; i++)
		makebyname(argv[i]);
	
	return fail;
}
