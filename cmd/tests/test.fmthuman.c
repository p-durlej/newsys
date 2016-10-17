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

#include <fmthuman.h>
#include <string.h>
#include <err.h>

static struct test
{
	uintmax_t	value;
	const char	*expect;
} tests[] =
{
	{ 3207393280UL,	"2.9 GiB"	},
	{ 1000000000UL,	"953.6 MiB"	},
	{    1000000UL,	"976.5 KiB"	},
	{       1000  ,	"1000"		},
	{	   1  ,	"1"		},
};

int main()
{
	const char *p;
	int fail = 0;
	int i;
	
	for (i = 0; i < sizeof tests / sizeof *tests; i++)
	{
		struct test *t = &tests[i];
		
		p = fmthumansz(t->value, 1);
		if (strcmp(p, t->expect))
		{
			warnx("value %ju, got \"%s\", expected \"%s\"", t->value, p, t->expect);
			fail = 1;
		}
	}
	return fail;
}
