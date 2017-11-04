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

#define REGEXP_DEBUG	0

#if REGEXP_DEBUG
#include "regexp.h"
#else
#include <regexp.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

#define SYMBOL(sym)	sym

#if !REGEXP_DEBUG
#define stdout		_get_stdout()
#define errno		_get_errno()
#endif

#if REGEXP_DEBUG
#define DPRINTF		warnx
#else
#define DPRINTF(x, ...)
#endif

struct piece
{
	int		min_rep, max_rep;
	int		type;
	struct piece *	next;
	struct piece *	prev;
	struct piece *	subx;
	struct piece *	nextbr;
	struct piece *	endbr;
	int		sbxi;
	const char *	start;
	const char *	end;
};

struct regexp
{
	char *startp[NSUBEXP];
	char *endp[NSUBEXP];
	
	struct piece	root;
};

struct rcptrs
{
	struct piece *	piece;
	struct piece *	lastp;
	const char *	str;
	int		nest;
	int		sbxi;
};

#define SI_BACKTRACK	0
#define SI_RESTORE	1

struct rx
{
	struct piece *	piece;
	const char *	spos;
	const char *	str;
	struct regexp *	rx;
};

#define PT_GUARD	1
#define PT_STRING	2
#define PT_WILDCARD	3
#define PT_SUBX		4
#define PT_SOL		5
#define PT_EOL		6
#define PT_BRANCH	7

#define MAX_REP		INT_MAX

static struct piece *	newpiece(struct rcptrs *rp, struct piece *list, int type);
static struct piece *	SYMBOL(regcomp_str)(struct rcptrs *rp, struct piece *list);
static struct piece *	SYMBOL(regcomp_split)(struct rcptrs *rp, struct piece *pc);
static int		SYMBOL(regcomp_1)(struct rcptrs *rp, struct piece *list);

static int		SYMBOL(regexec_p1)(struct rx *rx);
static int		SYMBOL(regexec_p)(struct rx *rx);
static int		SYMBOL(regexec_br)(struct rx *rx);
static int		SYMBOL(regexec_m)(struct rx *rx);
static int		SYMBOL(regexec_s)(struct regexp *rxp, const char *str);

static struct piece *newpiece(struct rcptrs *rp, struct piece *list, int type)
{
	struct piece *pc = rp->piece++;
	
	memset(pc, 0, sizeof *pc);
	pc->min_rep	= pc->max_rep = 1;
	pc->type	= type;
	
	if (list != NULL)
	{
		pc->next	= list;
		pc->prev	= list->prev;
		pc->next->prev	= pc;
		pc->prev->next	= pc;
	}
	else
		pc->next = pc->prev = pc;
	
	rp->lastp   = pc;
	return rp->lastp;
}

static struct piece *SYMBOL(regcomp_str)(struct rcptrs *rp, struct piece *list)
{
	struct piece *pc;
	
	pc = newpiece(rp, list, PT_STRING);
	pc->start = rp->str - 1;
	pc->end   = rp->str + strcspn(rp->str, ".+*?()^$|");
	
	rp->str = pc->end;
	return pc;
}

static struct piece *SYMBOL(regcomp_split)(struct rcptrs *rp, struct piece *pc)
{
	struct piece *n;
	
	switch (pc->type)
	{
	case PT_STRING:
		if (pc->start + 1 >= pc->end)
			return pc;
		
		n = newpiece(rp, pc->next, PT_STRING);
		n->start = pc->end - 1;
		n->end   = pc->end;
		pc->end  = n->start;
		
		return n;
	default:
		;
	}
	return pc;
}

int SYMBOL(regcomp_1)(struct rcptrs *rp, struct piece *list)
{
	struct piece *sob = rp->piece;
	struct piece *lt = NULL;
	struct piece *pc, *pc1;
	int c;
	int n;
	
	if (rp->nest >= 4)
		return -1;
	
	while (c = *rp->str++, c)
	{
		switch (c)
		{
		case '(':
			pc  = newpiece(rp, list, PT_SUBX);
			pc1 = newpiece(rp, NULL, PT_GUARD);
			
			pc->sbxi = rp->sbxi++;
			pc->subx = pc1;
			
			n = rp->nest++;
			if (SYMBOL(regcomp_1)(rp, pc1))
				goto bad;
			if (n != rp->nest)
				goto bad;
			break;
		case ')':
			if (!rp->nest--)
				goto bad;
			goto fini;
		case '*':
			if (lt == NULL)
				goto bad;
			
			pc1 = SYMBOL(regcomp_split)(rp, lt);
			pc1->min_rep = 0;
			pc1->max_rep = MAX_REP;
			break;
		case '+':
			if (lt == NULL)
				goto bad;
			
			pc1 = SYMBOL(regcomp_split)(rp, lt);
			pc1->max_rep = MAX_REP;
			break;
		case '?':
			if (lt == NULL)
				goto bad;
			
			pc1 = SYMBOL(regcomp_split)(rp, lt);
			pc1->min_rep--;
			break;
		case '.':
			if (rp->lastp != NULL && rp->lastp->type == PT_WILDCARD)
			{
				if (pc->min_rep != MAX_REP)
					rp->lastp->min_rep++;
				if (pc->max_rep != MAX_REP)
					rp->lastp->max_rep++;
				break;
			}
			
			pc = newpiece(rp, list, PT_WILDCARD);
			break;
		case '^':
			pc = newpiece(rp, list, PT_SOL);
			break;
		case '$':
			pc = newpiece(rp, list, PT_EOL);
			break;
		case '|':
			pc = newpiece(rp, list, PT_BRANCH);
			sob->nextbr = pc;
			break;
		default:
			pc = SYMBOL(regcomp_str)(rp, list);
		}
		lt = pc;
	}
fini:
	return 0;
bad:
	DPRINTF("bad");
	return -1;
}

struct regexp *SYMBOL(regcomp_int)(const char *rxs)
{
	struct rcptrs rp;
	struct regexp *rx;
	size_t sbxc;
	size_t pcc;
	size_t sz;
	
	sbxc = pcc = strlen(rxs); /* XXX */
	
	sz  = sizeof *rx;
	sz += pcc * sizeof(struct piece); /* XXX overflow */
	sz += pcc + 1;
	rx = calloc(1, sz);
	if (rx == NULL)
		return NULL;
	
	memset(&rp, 0, sizeof rp);
	rp.piece = (void *)(rx + 1);
	rp.str	 = (char *)(rx + 1) + pcc * sizeof(struct piece);
	strcpy((void *)rp.str, rxs);
	
	rx->root.min_rep = rx->root.max_rep = 1;
	rx->root.next	 = &rx->root;
	rx->root.prev	 = &rx->root;
	rx->root.type	 = PT_GUARD;
	
	if (SYMBOL(regcomp_1)(&rp, &rx->root))
		goto bad;
	
	return rx;
bad:
	free(rx);
	return NULL;
}

regexp *regcomp(const char *rxs)
{
	return (regexp *)regcomp_int(rxs);
}

static int SYMBOL(regexec_p1)(struct rx *rx)
{
	struct piece *pc = rx->piece;
	const char *osp;
	int sr;
	
	switch (pc->type)
	{
	case PT_STRING:
		if (strncmp(rx->spos, pc->start, pc->end - pc->start))
			return 0;
		rx->spos += pc->end - pc->start;
		return 1;
	case PT_WILDCARD:
		if (!*rx->spos)
			return 0;
		rx->spos++;
		return 1;
	case PT_SOL:
		if (rx->spos != rx->str)
			return 0;
		return 1;
	case PT_EOL:
		if (*rx->spos)
			return 0;
		return 1;
	case PT_SUBX:
		DPRINTF("regex_p1: entering subexpression");
		rx->piece = pc->subx->next;
		osp = rx->spos;
		sr  = SYMBOL(regexec_m)(rx);
		rx->piece = pc;
		if (pc->sbxi < NSUBEXP)
		{
			rx->rx->startp[pc->sbxi] = (char *)osp;
			rx->rx->endp[pc->sbxi]	 = (char *)rx->spos;
		}
		DPRINTF("regex_p1: leaving subexpression");
		return sr;
	default:
		errx(1, "regex_p1: %p: bad opc %i", pc, pc->type);
	}
}

static int SYMBOL(regexec_p)(struct rx *rx)
{
	struct piece *pc = rx->piece;
	const char *osp  = rx->spos;
	int i, cnt = pc->max_rep;
	
	DPRINTF("i_regexec_p: pc = %p, spos = \"%s\", pc->type = %i", pc, rx->spos, pc->type);
	
	if (pc->type == PT_GUARD || pc->type == PT_BRANCH)
		return 1;
	
	if (pc->min_rep == 1 && pc->max_rep == 1)
	{
		if (SYMBOL(regexec_p1)(rx))
		{
			rx->piece = pc->next;
			return 1;
		}
		return 0;
	}
	
	while (cnt >= pc->min_rep)
	{
		rx->piece = pc;
		rx->spos  = osp;
		
		DPRINTF("i_regexec_p: cnt = %i, min_rep = %i", cnt, pc->min_rep);
		for (i = 0; i < cnt; i++)
			if (!SYMBOL(regexec_p1)(rx))
			{
				cnt = i;
				break;
			}
		DPRINTF("i_regexec_p:  cnt = %i, min_rep = %i", cnt, pc->min_rep);
		
		if (i < pc->min_rep)
			break;
		
		rx->piece = rx->piece->next;
		if (SYMBOL(regexec_p)(rx))
		{
			DPRINTF("i_regexec_p: match succeeded");
			return 1;
		}
		rx->piece = rx->piece->prev;
		cnt--;
	}
	rx->piece = pc;
	rx->spos  = osp;
	DPRINTF("i_regexec_p: match failed");
	return 0;
}

static int SYMBOL(regexec_br)(struct rx *rx)
{
	DPRINTF("i_regexp: entering branch, pc = %p, spos = \"%s\"", rx->piece, rx->spos);
	while (rx->piece->type != PT_GUARD)
	{
		if (rx->piece->type == PT_BRANCH)
			break;
		
		DPRINTF("i_regexec_br: pc = %p, spos = \"%s\"",
			rx->piece, rx->spos);
		
		if (!SYMBOL(regexec_p)(rx))
		{
			DPRINTF("i_regexec_br: branch failed");
			return 0;
		}
		// rx->piece = rx->piece->next;
	}
	DPRINTF("i_regexec_br: branch succeeded, spos = \"%s\"", rx->spos);
	return 1;
}

static int SYMBOL(regexec_m)(struct rx *rx)
{
	struct piece *pc = rx->piece;
	const char *osp  = rx->spos;
	
	for (;;)
	{
		if (SYMBOL(regexec_br)(rx))
			return 1;
		if (pc->nextbr == NULL)
			break;
		pc = pc->nextbr->next;
		
		rx->piece = pc;
		rx->spos  = osp;
	}
	
	rx->piece = pc;
	rx->spos  = osp;
	return 0;
}

static int SYMBOL(regexec_s)(struct regexp *rxp, const char *str)
{
	struct rx rx;
	
	DPRINTF("i_regexec_s");
	
	memset(&rx, 0, sizeof rx);
	rx.piece = rxp->root.next;
	rx.str	 = str;
	rx.spos	 = str;
	rx.rx	 = rxp;
	
	return SYMBOL(regexec_m)(&rx);
}

int SYMBOL(regexec_int)(struct regexp *rx, const char *str)
{
	if (rx->root.next->type == PT_SOL)
		return SYMBOL(regexec_s)(rx, str);
	
	while (*str)
	{
		if (SYMBOL(regexec_s)(rx, str))
			return 1;
		str++;
	}
	return SYMBOL(regexec_s)(rx, "");
}

int regexec(regexp *rx, const char *str)
{
	return regexec_int((struct regexp *)rx, str);
}

static void SYMBOL(regdump_1)(struct piece *p, int nest)
{
	const char *cp;
	int i;
	
	do
	{
		printf("%p ", p);
		for (i = 0; i < nest; i++)
			putchar(' ');
		
		switch (p->type)
		{
		case PT_WILDCARD:
			fputs("PT_WILDCARD ", stdout);
			break;
		case PT_STRING:
			printf("PT_STRING   \"");
			for (cp = p->start; cp < p->end; cp++)
				putchar(*cp);
			printf("\" @ %p", p->start);
			break;
		case PT_SUBX:
			fputs("PT_SUBX     ", stdout);
			break;
		case PT_SOL:
			fputs("PT_SOL      ", stdout);
			break;
		case PT_EOL:
			fputs("PT_EOL      ", stdout);
			break;
		case PT_GUARD:
			fputs("PT_GUARD    ", stdout);
			break;
		case PT_BRANCH:
			fputs("PT_BRANCH   ", stdout);
			break;
		default:
			fputs("** BAD **   ", stdout);
		}
		if (p->min_rep != 1 || p->max_rep != 1)
			printf("{ %i, %i }", p->min_rep, p->max_rep);
		if (p->nextbr != NULL)
			printf("nextbr %p ", p->nextbr);
		putchar('\n');
		
		if (p->type == PT_SUBX)
			SYMBOL(regdump_1)(p->subx, nest + 1);
		
		p = p->next;
	} while (p->type != PT_GUARD);
}

void SYMBOL(regdump_int)(struct regexp *rx)
{
	SYMBOL(regdump_1)(&rx->root, 0);
}

void regdump(regexp *rx)
{
	regdump_int((struct regexp *)rx);
}

#if REGEXP_DEBUG
struct rxtest
{
	char *	pattern;
	char *	string;
	int	result;
} rxt[] =
{
	{ "^test",	"test",		1 },
	{ "^test",	"xtest",	0 },
	{ "a+",		"ab",		1 },
	{ "a+",		"aa",		1 },
	{ "a+$",	"ab",		0 },
	{ ".*",		"abc",		1 },
	{ "^(ab|cd)+x",	"ababcdab",	0 },
	{ "^(ab|cd)+x",	"ababcdabx",	1 },
	{ "a*",		"",		1 },
	{ "a*^",	"",		1 },
};

int main(int argc, char **argv)
{
	regexp *rx;
	int i;
	
	setlinebuf(stdout);
	
	for (i = 0; i < sizeof rxt / sizeof *rxt; i++)
	{
		rx = regcomp(rxt[i].pattern);
		if (rx == NULL)
			errx(1, "regcomp failed for \"%s\"", rxt[i].pattern);
		regdump(rx);
		
		if (!!regexec(rx, rxt[i].string) != rxt[i].result)
			errx(1, "mismatch for pattern \"%s\", string \"%s\"", rxt[i].pattern, rxt[i].string);
	}
	return 0;
}
#endif
