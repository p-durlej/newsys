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

#include <mkcanon.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

int _mkcanon(const char *cwd, char *buf)
{
	char lbuf[PATH_MAX];
	char *d_p = lbuf;
	char *s_p = buf;
	char *psep;
	int l;
	
	if (strlen(cwd) >= PATH_MAX)
	{
		_set_errno(ENAMETOOLONG);
		return -1;
	}
	
	if (*s_p != '/')
	{
		strcpy(lbuf, cwd);
		if (!strcmp(lbuf, "/"))
			*lbuf = 0;
		else
			d_p = strchr(lbuf, 0);
	}
	
	for (;;)
	{
		while (*s_p == '/')
			s_p++;
		
		if (!*s_p)
			break;
		
		psep = strchr(s_p, '/');
		if (psep)
			l = psep - s_p;
		else
			l = strlen(s_p);
		
		if (!l)
			goto next;
		
		if (l == 1 && s_p[0] == '.')
			goto next;
		
		if (l == 2 && s_p[0] == '.' && s_p[1] == '.')
		{
			*d_p = 0;
			
			d_p = strrchr(lbuf, '/');
			if (!d_p)
				d_p = lbuf;
			
			goto next;
		}
		
		if (d_p - lbuf + l >= PATH_MAX)
		{
			_set_errno(ENAMETOOLONG);
			return -1;
		}
		
		*(d_p++) = '/';
		memcpy(d_p, s_p, l);
		d_p += l;
next:
		s_p += l;
	}
	
	*d_p = 0;
	strcpy(buf, lbuf);
	return 0;
}
