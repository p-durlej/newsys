/* Copyright (C) Piotr Durlej */

#define _GNU_SOURCE

#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <err.h>

#include "umake.h"

struct rule *rules;
struct var *vars;

static const char *mfpathname;
static int mfnextline;
static int mfline;
static int mffail;
static int mfeof;
static FILE *mffile;

static char inbuf[4096];
static char *tokenp;

void setvar(const char *name, const char *value)
{
	struct var *var;
	
	if (vflag)
		warnx("%s = %s", name, value);
	
	for (var = vars; var != NULL; var = var->next)
		if (!strcmp(var->name, name))
		{
			var->val = strdup(value);
			if (var->val == NULL)
				err(1, NULL);
			return;
		}
	
	var = calloc(1, sizeof *var);
	if (var == NULL)
		err(1, NULL);
	
	var->name = strdup(name);
	var->val  = strdup(value);
	var->next = vars;
	vars	  = var;
	
	if (var->name == NULL || var->val == NULL)
		err(1, NULL);
}

static int input(void)
{
	char *p;
	int c;
	
	mfline = mfnextline;
	
	p = inbuf;
	while (c = fgetc(mffile), c != EOF)
	{
		if (p >= inbuf + sizeof inbuf - 1)
		{
			warnx("%s: %i: line too long", mfpathname, mfline);
			mffail = 1;
			return -1;
		}
		
		switch (c)
		{
		case '\\':
			c = fgetc(mffile);
			if (c == '\n')
			{
				mfnextline++;
				c = ' ';
			}
			if (c != EOF)
				*p++ = c;
			break;
		case '\n':
			mfnextline++;
			
			if (*inbuf == '#' || p == inbuf)
			{
				p = inbuf;
				continue;
			}
			tokenp = inbuf;
			*p = 0;
			return 0;
		default:
			*p++ = c;
		}
	}
	
	if (feof(mffile))
		mfeof = 1;
	
	return -1;
}

static const char *getvar(const char *name)
{
	struct var *var;
	const char *p;
	size_t nlen;
	
	p = strchr(name, ')');
	nlen = p - name;
	
	for (var = vars; var != NULL; var = var->next)
	{
		if (strncmp(var->name, name, nlen))
			continue;
		if (var->name[nlen])
			continue;
		
		return var->val;
	}
	
	return NULL;
}

char *substvars(const char *s)
{
	char buf[4096]; // XXX
	char *p, *p1, *p2;
	const char *v;
	char *sp, *dp;
	ssize_t o;
	size_t l;
	
	strcpy(buf, s); // XXX
	p = buf;
	while (p1 = strstr(p, "$("), p1 != NULL)
	{
		p2 = strchr(p1, ')');
		
		if (p2 == NULL)
		{
			warnx("%s: %i: unterminated $(...)", mfpathname, mfline);
			mffail = 1;
			goto fini;
		}
		
		v = getvar(p1 + 2);
		if (v == NULL)
		{
			p = p2 + 1;
			continue;
		}
		l = strlen(v);
		
		dp = p1 + l;
		sp = p2 + 1;
		
		memmove(dp, sp, strlen(sp) + 1);
		memcpy(p1, v, l);
	}
fini:
	p = strdup(buf);
	if (p == NULL)
		err(1, NULL);
	return p;
}

static int isstokc(int c)
{
	return c == '=' || c == ':' || c == '+';
}

static int ismtokc(int c)
{
	if (isstokc(c))
		return 0;
	if (!c)
		return 0;
	return !isspace(c);
}

static char *token(void)
{
	char *t;
	char *p;
	
	while (isspace(*tokenp))
		tokenp++;
	if (!*tokenp)
		return NULL;
	
	if (isstokc(*tokenp))
	{
		t = malloc(2);
		
		t[0] = *tokenp;
		t[1] = 0;
		
		tokenp++;
		return t;
	}
	
	for (p = tokenp; ismtokc(*p); p++)
		;
	
	t = malloc(p - tokenp + 1);
	if (t == NULL)
		err(1, NULL);
	
	memcpy(t, tokenp, p - tokenp);
	t[p - tokenp] = 0;
	
	tokenp = p;
	return t;
}

static void procrule(void)
{
	static char *noinput[] = { NULL };
	
	struct rule *r;
	char *d;
	int cnt = 0;
	
	r = calloc(1, sizeof *r);
	r->next = rules;
	rules = r;
	
	r->output = substvars(token());
	free(token());
	
	d = substvars(tokenp);
	tokenp = d;
	
	while (d = token(), d != NULL)
	{
		r->input = realloc(r->input, (cnt + 2) * sizeof r->input);
		r->input[cnt++] = d;
	}
	if (cnt)
		r->input[cnt] = NULL;
	else
		r->input = noinput;
}

static void procset(void)
{
	char *name;
	char *val;
	
	name = token();
	free(token());
	
	val = tokenp;
	while (isspace(*val))
		val++;
	
	setvar(name, val);
	free(name);
}

static void proccat(void)
{
	struct var *v;
	char *name;
	char *val;
	char *eq;
	
	name = token();
	free(token());
	eq = token();
	val = token();
	
	if (strcmp(eq, "=") || val == NULL)
	{
		warnx("%s: %i: syntax error", mfpathname, mfline);
		mffail = 1;
		return;
	}
	
	for (v = vars; v != NULL; v = v->next)
		if (!strcmp(v->name, name))
		{
			if (asprintf(&v->val, "%s %s", v->val, val) < 0)
				err(1, NULL);
			return;
		}
	
	setvar(name, val);
}

static void proccmd(void)
{
	struct rule *r = rules;
	char *p;
	int cnt = 0;
	
	if (r->cmds)
		for (cnt = 0; r->cmds[cnt]; cnt++)
			;
	
	for (p = inbuf; isspace(*p); p++)
		;
	if (!*p)
		return;
	
	r->cmds = realloc(r->cmds, (cnt + 2) * sizeof *r->cmds);
	if (r->cmds == NULL)
		err(1, NULL);
	
	r->cmds[cnt    ] = strdup(p);
	r->cmds[cnt + 1] = NULL;
	
	if (r->cmds[cnt] == NULL)
		err(1, NULL);
}

static void procincl(void)
{
	const char *smfpathname;
	int smfnextline;
	int smfline;
	int smffail;
	FILE *smffile;
	char *pathname = NULL;
	char buf[PATH_MAX];
	char *name;
	int i;
	
	smfpathname	= mfpathname;
	smfnextline	= mfnextline;
	smfline		= mfline;
	smffail		= mffail;
	smffile		= mffile;
	
	free(token());
	name = token();
	
	if (!access(name, 0))
		pathname = name;
	else
		for (i = 0; i < incpathcnt; i++)
		{
			sprintf(buf, "%s/%s", incpaths[i], name);
			if (!access(buf, 0))
			{
				pathname = buf;
				break;
			}
		}
	
	if (pathname == NULL)
	{
		warnx("%s: %i: %s: Not found", mfpathname, mfline, name);
		mffail = 1;
		return;
	}
	
	load(pathname);
	
	mfpathname	= smfpathname;
	mfnextline	= smfnextline;
	mfline		= smfline;
	// mffail		= smffail;
	mffile		= smffile;
	mfeof		= 0;
	
	free(name);
}

static void procline(void)
{
	char *p;
	
	if (vflag)
		warnx("< %s", inbuf);
	
	// substvars();
	
	if (*inbuf == '\t' || *inbuf == ' ')
	{
		proccmd();
		return;
	}
	
	p = inbuf;
	while (ismtokc(*p))
		p++;
	
	if (p - inbuf == 7 && !memcmp(inbuf, "include", 7))
	{
		procincl();
		return;
	}
	
	while (isspace(*p))
		p++;
	
	switch (*p)
	{
	case ':':
		procrule();
		return;
	case '=':
		procset();
		return;
	case '+':
		proccat();
		return;
	}
	
	warnx("%s: %i: syntax error", mfpathname, mfline);
	mffail = 1;
}

int load(const char *pathname)
{
	FILE *f;
	
	f = fopen(pathname, "r");
	if (f == NULL)
	{
		warn("%s", pathname);
		return -1;
	}
	
	mfpathname = pathname;
	mfline = 1;
	mfeof  = 0;
	mffile = f;
	
	while (!input())
		procline();
	
	fclose(f);
	
	if (mfeof && !mffail)
		return 0;
	return -1;
}
