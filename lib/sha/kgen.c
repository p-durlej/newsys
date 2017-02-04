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

#include <stdint.h>
#include <stdio.h>
#include <math.h>

#define NPRM	1000

static int isnprm[NPRM];

static void gprm(void)
{
	int i, n;
	
	for (i = 2; i < NPRM; i++)
		if (!isnprm[i])
			for (n = 2 * i; n < NPRM; n += i)
				isnprm[n] = 1;
	isnprm[0] = 1;
	isnprm[1] = 1;
}

static int nprm(int p)
{
	for (p++; p < NPRM; p++)
		if (!isnprm[p])
			return p;
	return 0;
}

static void pprm(void)
{
	int i;
	
	for (i = 2; i; i = nprm(i))
		printf("%i\n", i);
}

static void pprmcrf(int start, int cnt)
{
	uint32_t v;
	int i, p;
	
	for (i = 0, p = start; i < cnt; i++, p = nprm(p))
	{
		v = powl(p, 1 / 3.L) * 0x10000L * 0x10000L;
		printf("\t%#010lx,\n", (unsigned long)v);
	}
}

int main()
{
	int i;
	
	gprm();
	// pprm();
	pprmcrf(2, 64);
}
