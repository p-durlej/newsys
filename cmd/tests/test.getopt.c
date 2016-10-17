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

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct expect
{
	char	opt;
	char *	arg;
};

static struct test
{
	char **		argv;
	char *		opts;
	struct expect *	expected;
	int		optind;
} tests[] =
{
	{
		(char *[]){ "test.getopt", "-xyz", NULL },
		"xyz",
		(struct expect []) {
			{ 'x', NULL },
			{ 'y', NULL },
			{ 'z', NULL },
			{ -1,  NULL },
		},
		2
	},
	
	{
		(char *[]){ "test.getopt", "-xyaz", NULL },
		"xyz",
		(struct expect []) {
			{ 'x', NULL },
			{ 'y', NULL },
			{ '?', NULL },
		},
		1
	},
	
	{
		(char *[]){ "test.getopt", "-x", "-y", "-z", NULL },
		"xyz",
		(struct expect []) {
			{ 'x', NULL },
			{ 'y', NULL },
			{ 'z', NULL },
			{ -1,  NULL },
		},
		4
	},
	
	{
		(char *[]){ "test.getopt", "-x", "-y", "test", "-z", NULL },
		"xy:z",
		(struct expect []) {
			{ 'x', NULL },
			{ 'y', "test" },
			{ 'z', NULL },
			{ -1,  NULL },
		},
		5,
	},
	
	{
		(char *[]){ "test.getopt", "-x", "-ytest", "-z", NULL },
		"xy:z",
		(struct expect []) {
			{ 'x', NULL },
			{ 'y', "test" },
			{ 'z', NULL },
			{ -1,  NULL },
		},
		4
	},
		
	{
		(char *[]){ "test.getopt", "-xytest", "--", "-z", NULL },
		"xy:z",
		(struct expect []) {
			{ 'x', NULL },
			{ 'y', "test" },
			{ -1,  NULL },
		},
		3
	},
	
};

static void fail_opt(struct test *t, struct expect *e, int r)
{
	fprintf(stderr, "test.getopt: test %i, exp %i: bad option (got %i)\n", (int)(t - tests), (int)(e - t->expected), r);
	exit(255);
}

static void fail_arg(struct test *t, struct expect *e)
{
	fprintf(stderr, "test.getopt: test %i, exp %i: bad optarg\n", (int)(t - tests), (int)(e - t->expected));
	exit(255);
}

static void fail_ind(struct test *t, struct expect *e)
{
	fprintf(stderr, "test.getopt: test %i: bad optind (got %i)\n", (int)(t - tests), optind);
	exit(255);
}

int main()
{
	struct expect *e;
	struct test *t;
	char **ae;
	int ac;
	int i;
	int r;
	
	for (i = 0; i < sizeof tests / sizeof *tests; i++)
	{
		t = &tests[i];
		e = t->expected;
		
		for (ae = t->argv; *ae; ae++)
			;
		ac = ae - t->argv;
		
		if (i)
		{
			optreset = 1;
			optind	 = 1;
		}
		do
		{
			r = getopt(ac, t->argv, t->opts);
			if (r != e->opt)
				fail_opt(t, e, r);
			if (e->arg && strcmp(optarg, e->arg))
				fail_arg(t, e);
			e++;
		}
		while (r != '?' && r != ':' && r != -1);
		
		if (optind != t->optind)
			fail_ind(t, e);
	}
	return 0;
}
