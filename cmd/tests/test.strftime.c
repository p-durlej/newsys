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

#include <string.h>
#include <time.h>
#include <err.h>

#include <stdlib.h>

static int xit;

static struct test
{
	const char *fmt, *exp;
} tests[] =
{
	{ "^",	"^",			},
	{ "%%",	"%",			},
	{ "%A",	"Wednesday",		},
	{ "%a",	"Wed",			},
	{ "%B",	"May",			},
	{ "%b",	"May",			},
	{ "%h",	"May",			},
	{ "%c",	"1995-05-20 12:34:56",	},
	{ "%C",	"19",			},
	{ "%d",	"20",			},
	{ "%D",	"05/20/95",		},
	{ "%e",	"20",			},
	{ "%F",	"1995-05-20",		},
	{ "%G",	"1995",			},
	{ "%g",	"95",			},
	{ "%H",	"12",			},
	{ "%I",	"12",			},
	{ "%j",	"001",			},
	{ "%k",	"12",			},
	{ "%l",	"12",			},
	{ "%M",	"34",			},
	{ "%m",	"05",			},
	{ "%n",	"\n",			},
	{ "%P",	"pm",			},
	{ "%p",	"PM",			},
	{ "%R",	"12:34",		},
	{ "%r",	"12:34:56 PM",		},
	{ "%S",	"56",			},
	{ "%s",	"800973296",		},
	{ "%t",	"\t",			},
	{ "%T",	"12:34:56",		},
	{ "%U",	"??",			},
	{ "%u",	"3",			},
	{ "%V",	"??",			},
	{ "%W",	"??",			},
	{ "%w",	"3",			},
	{ "%X",	"12:34:56",		},
	{ "%x",	"1995-05-20",		},
	{ "%Y",	"1995",			},
	{ "%y",	"95",			},
	{ "%Z",	"GMT",			},
	{ "%z",	"+00",			},
	{ "$",	"$",			},
	{ NULL, NULL			}
};

static void do_test(const char *fmt, const char *exp, int i)
{
	char buf[4096];
	struct tm tm;
	
	memset(&tm, 0, sizeof tm);
	
	tm.tm_wday = 3;
	tm.tm_year = 95;
	tm.tm_mon  = 4;
	tm.tm_mday = 20;
	
	tm.tm_hour = 12;
	tm.tm_min  = 34;
	tm.tm_sec  = 56;
	
	strftime(buf, sizeof buf, fmt, &tm);
	
	if (strcmp(buf, exp))
	{
		warnx("fmt = \"%s\"", fmt);
		warnx("buf = \"%s\"", buf);
		warnx("exp = \"%s\"", exp);
		warnx("i   = %i", i);
		xit = 1;
		
		exit(1);
	}
}

int main()
{
	int i;
	
	for (i = 0; tests[i].fmt; i++)
		do_test(tests[i].fmt, tests[i].exp, i);
	
	return xit;
}
