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

#include <stdlib.h>
#include <limits.h>
#include <errno.h>

long long strtoll(const char *nptr, char **endptr, int base)
{
	unsigned long long uv;
	const char *ptr = nptr;
	char *ep;
	int neg = 0;
	
	while (isspace(*ptr))
		ptr++;
	
	if (*ptr == '-')
	{
		neg = 1;
		ptr++;
	}
	else if (*ptr == '+')
		ptr++;
	
	uv = strtoull(ptr, &ep, base);
	if (neg)
	{
		if (uv > (unsigned long long)-LLONG_MIN)
		{
			_set_errno(ERANGE);
			return LLONG_MIN;
		}
		if (endptr)
			*endptr = ep;
		return -uv;
	}
	if (uv > LLONG_MAX)
	{
		_set_errno(ERANGE);
		return LLONG_MAX;
	}
	if (endptr)
		*endptr = ep;
	return uv;
}
