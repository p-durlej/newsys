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

#include <unistd.h>
#include <string.h>
#include <stdio.h>

static int getopt_ai;

int _getopt(int argc, char *const argv[], const char *opts,
	    char **oa, int *oi, int *oo, int *oe, int *or)
{
	char *ap;
	char *op;
	char c;
	
	if (getopt_ai < 1 || *or)
	{
		getopt_ai = 1;
		*or	  = 0;
	}
	if (*oi < 1)
	{
		getopt_ai = 1;
		*oi	  = 1;
	}
	
next_arg:
	if (*oi >= argc)
		return -1;
	
	ap = argv[*oi];
	if (*ap != '-')
		return -1;
	
	ap += getopt_ai;
	if (!*ap)
	{
		getopt_ai = 1;
		(*oi)++;
		goto next_arg;
	}
	
	c  = *ap++;
	if (c == '-')
	{
		(*oi)++;
		return -1;
	}
	
	op = strchr(opts, c);
	if (!op)
	{
		if (*oe)
			fprintf(_get_stderr(), "%s: -%c: bad option\n", argv[0], c);
		*oo = c;
		return '?';
	}
	
	if (op[1] == ':')
	{
		if (*ap)
		{
			*oa = ap;
			(*oi)++;
		}
		else
		{
			if (*oi >= argc - 1)
			{
				if (*oe)
					fprintf(_get_stderr(), "%s: -%c: missing argument\n", argv[0], c);
				*oo = c;
				if (*opts == ':')
					return ':';
				return '?';
			}
			*oa  = argv[*oi + 1];
			*oi += 2;
		}
		getopt_ai = 0;
	}
	getopt_ai++;
	
	return c;
}
