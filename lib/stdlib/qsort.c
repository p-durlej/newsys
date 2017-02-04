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

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

static void swap(char *p0, char *p1, size_t size)
{
	char t;
	
	while (size--)
	{
		t   = *p0;
		*p0 = *p1;
		*p1 = t;
		
		p0++;
		p1++;
	}
}

static void sub_sort(char *start, char *end, size_t size, int (*cmp)(const void *a, const void *b))
{
	char *p0 = start;
	char *p1 = end;
	
	if (start > end - size)
		return;
	
	while (p0 < p1)
	{
		while (cmp(p0, start) <= 0 && p0 < end)
			p0 += size;
		while (cmp(p1, start) >= 0 && p1 > start)
			p1 -= size;
		if (p0 < p1)
			swap(p0, p1, size);
	}
	swap(start, p1, size);
	
	sub_sort(start, p1, size, cmp);
	sub_sort(p1 + size, end, size, cmp);
}

void qsort(void *data, size_t cnt, size_t size, int (*cmp)(const void *a, const void *b))
{
	if (cnt == 0)
		return;
	sub_sort(data, (char *)data + (cnt - 1) * size, size, cmp);
}
