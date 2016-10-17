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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

typedef long long INT;

#define T_INT	1
#define T_STR	2

#define P_MOD	0

#define P_DIV	1

#define P_MUL	2

#define P_SUB	3
#define P_ADD	3

#define P_CMP	4

#define P_AND	5

#define P_OR	6

#define P_ALL	7

struct value
{
	int type;
	union
	{
		INT integer;
		char *string;
	};
};

static char **token;

static void getsingleval(struct value *buf);
static void getvalue(struct value *buf, int p);

static void getsingleval(struct value *buf)
{
	char *end;
	INT iv;
	
	if (!*token)
		errx(255, "unexpected end of expression");
	
	if (!strcmp(*token, "("))
	{
		token++;
		getvalue(buf, P_ALL);
		if (!*token || strcmp(*token, ")"))
			errx(255, "'(' with no corresponding ')'\n");
		
		token++;
		return;
	}
	
	errno = 0;
	iv = strtoul(*token, &end, 0);
	if (errno)
	{
		perror(*token);
		exit(errno);
	}
	if (end == *token + strlen(*token))
	{
		buf->type    = T_INT;
		buf->integer = iv;
		token++;
		return;
	}
	buf->type   = T_STR;
	buf->string = strdup(*token);
	if (!buf->string)
	{
		perror(*token);
		exit(255);
	}
	token++;
}

static void getvalue(struct value *buf, int p)
{
#define INTOP(char, op, pri)							\
	if (p < pri)								\
		break;								\
	if (!strcmp(*token, char))						\
	{									\
		token++;							\
		getvalue(&v2, pri);						\
		if (v1.type!=T_INT || v2.type!=T_INT)				\
		{								\
			fprintf(stderr, "expr: %s"				\
				": numeric arguments required\n", char);	\
			exit(255);						\
		}								\
		v1.integer = v1.integer op v2.integer;				\
		continue;							\
	}
	
	struct value v1;
	struct value v2;
	
	getsingleval(&v1);
	
	while (*token)
	{
		INTOP("%",  %,  P_MOD);
		INTOP("/",  /,  P_DIV);
		INTOP("*",  *,  P_MUL);
		
		INTOP("+",  +,  P_ADD);
		INTOP("-",  -,  P_ADD);
		
		INTOP("<",  <,  P_CMP);
		INTOP("<=", <=, P_CMP);
		INTOP(">",  >,  P_CMP);
		INTOP(">=", >=, P_CMP);
		INTOP("==", ==, P_CMP);
		INTOP("!=", !=, P_CMP);
		
		if (p < P_AND)
			break;
		if (!strcmp(*token, "&"))
		{
			token++;
			getvalue(&v2, P_CMP);
			
			if (v2.type == T_INT && !v2.integer)
			{
				v1.type	   = T_INT;
				v1.integer = 0;
			}
			if (v2.type == T_STR && !*v2.string)
			{
				v1.type	   = T_INT;
				v1.integer = 0;
			}
			if (v1.type == T_STR && !*v1.string)
			{
				v1.type	   = T_INT;
				v1.integer = 0;
			}
			continue;
		}
		if (p < P_OR)
			break;
		if (!strcmp(*token, "|"))
		{
			token++;
			getvalue(&v2, P_CMP);
			
			if (v1.type == T_INT && !v1.integer)
				v1 = v2;
			if (v1.type == T_STR && !*v1.string)
				v1 = v2;
			continue;
		}
		break;
	}
	*buf = v1;
#undef INTOP
}

int main(int argc, char **argv)
{
	struct value v;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		printf("\nUsage: expr EXPR\n"
			"Evaluate EXPR and print the result.\n\n"
			" Supported operators are:\n"
			"  %%                        (priority 0)\n"
			"  /                        (priority 1)\n"
			"  *                        (priority 2)\n"
			"  +, -                     (priority 3)\n"
			"  <, <=, >, >=, ==, !=     (priority 4)\n"
			"  &                        (priority 5)\n"
			"  |                        (priority 6)\n\n"
			" The lower number the higher priority.\n\n"
			);
		return 0;
	}
	
	token = argv + 1;
	getvalue(&v, P_ALL);
	
	switch (v.type)
	{
	case T_INT:
		printf("%lli\n", (long long)v.integer);
		return 0;
	case T_STR:
		printf("\"%s\"\n", v.string);
			return 0;
	}
	abort();
}
