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

#include <errno.h>
#include <ctype.h>

unsigned long strtoul(const char *nptr, char **endptr, int base)
{
	const unsigned char *ptr = nptr;
	unsigned long v = 0;
	int neg = 0;
	
	while (isspace(*ptr))
		ptr++;
	
	if (!base)
	{
		if (*ptr == '0')
		{
			ptr++;
			
			if (*ptr == 'x' || *ptr == 'X')
			{
				ptr++;
				base = 16;
			}
			else
				base = 8;
		}
		else
			base = 10;
	}
	else
		if (base == 16 && ptr[0] == '0' && (ptr[1] == 'x' || ptr[1] == 'X'))
			ptr += 2;
	
	while (*ptr)
	{
		unsigned long prev = v;
		int digit =* ptr - '0';
		
		if (isdigit(*ptr))
			digit =* ptr - '0';
		else
		if (isupper(*ptr))
		{
			digit =* ptr - 'A';
			digit += 10;
		}
		else
		if (islower(*ptr))
		{
			digit =* ptr - 'a';
			digit += 10;
		}
		else
			break;
		
		if (digit < 0 || digit >= base)
			break;
		
		v *= base;
		if (v / base != prev)
		{
			_set_errno(ERANGE);
			break;
		}
		v += digit;
		ptr++;
	}
	
	if (endptr)
		*endptr = ptr;
	return v;
}
