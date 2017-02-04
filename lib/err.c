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

#include <priv/libc_globals.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

void _cwarn(const char *fmt, ...)
{
	char buf[256]; /* XXX */
	const char *a0;
	const char *p;
	char *dp;
	va_list ap;
	
	a0 = __libc_argv[0];
	p  = strrchr(a0, '/');
	if (p == NULL)
		p = a0;
	else
		p++;
	
	strcpy(buf, p);
	strcat(buf, ": ");
	
	dp = buf + strlen(buf);
	
	if (fmt != NULL)
	{
		va_start(ap, fmt);
		vsprintf(dp, fmt, ap);
		va_end(ap);
		
		strcat(dp, ": ");
	}
	strcat(dp, strerror(_get_errno()));
	strcat(dp, "\n");
	
	_sysmesg(buf);
}

void _cerr(int status, const char *fmt, ...)
{
	char buf[256]; /* XXX */
	const char *a0;
	const char *p;
	char *dp;
	va_list ap;
	
	a0 = __libc_argv[0];
	p  = strrchr(a0, '/');
	if (p == NULL)
		p = a0;
	else
		p++;
	
	strcpy(buf, p);
	strcat(buf, ": ");
	
	dp = buf + strlen(buf);
	
	if (fmt != NULL)
	{
		va_start(ap, fmt);
		vsprintf(dp, fmt, ap);
		va_end(ap);
		
		strcat(dp, ": ");
	}
	strcat(dp, strerror(_get_errno()));
	strcat(dp, "\n");
	
	_sysmesg(buf);
	exit(status);
}

void errc(int status, int err, const char *fmt, ...)
{
	const char *a0;
	const char *p;
	va_list ap;
	
	a0 = __libc_argv[0];
	p  = strrchr(a0, '/');
	if (p == NULL)
		p = a0;
	else
		p++;
	fprintf(_get_stderr(), "%s: ", p);
	
	if (fmt != NULL)
	{
		va_start(ap, fmt);
		vfprintf(_get_stderr(), fmt, ap);
		va_end(ap);
		fputs(": ", _get_stderr());
	}
	
	fprintf(_get_stderr(), "%s\n", strerror(err));
	exit(status);
}

void errx(int status, const char *fmt, ...)
{
	const char *a0;
	const char *p;
	va_list ap;
	
	a0  = __libc_argv[0];
	p   = strrchr(a0, '/');
	if (p == NULL)
		p = a0;
	else
		p++;
	fprintf(_get_stderr(), "%s: ", p);
	
	if (fmt != NULL)
	{
		va_start(ap, fmt);
		vfprintf(_get_stderr(), fmt, ap);
		va_end(ap);
	}
	fputc('\n', _get_stderr());
	
	exit(status);
}

void err(int status, const char *fmt, ...)
{
	const char *a0;
	const char *p;
	FILE *stderr;
	va_list ap;
	int err;
	
	stderr = _get_stderr();
	err    = _get_errno();
	
	a0 = __libc_argv[0];
	p  = strrchr(a0, '/');
	if (p == NULL)
		p = a0;
	else
		p++;
	fprintf(stderr, "%s: ", p);
	
	if (fmt != NULL)
	{
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		fputs(": ", stderr);
	}
	
	fprintf(stderr, "%s\n", strerror(err));
	exit(status);
}

void warnc(int err, const char *fmt, ...)
{
	const char *a0;
	const char *p;
	FILE *stderr;
	va_list ap;
	
	stderr = _get_stderr();
	
	a0 = __libc_argv[0];
	p  = strrchr(a0, '/');
	if (p == NULL)
		p = a0;
	else
		p++;
	fprintf(stderr, "%s: ", p);
	
	if (fmt != NULL)
	{
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		fputs(": ", stderr);
	}
	
	fprintf(stderr, "%s\n", strerror(err));
}

void warnx(const char *fmt, ...)
{
	const char *a0;
	const char *p;
	FILE *stderr;
	va_list ap;
	int err;
	
	stderr = _get_stderr();
	
	a0 = __libc_argv[0];
	p  = strrchr(a0, '/');
	if (p == NULL)
		p = a0;
	else
		p++;
	fprintf(stderr, "%s: ", p);
	
	if (fmt != NULL)
	{
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
	fputc('\n', stderr);
}

void warn(const char *fmt, ...)
{
	const char *a0;
	const char *p;
	FILE *stderr;
	va_list ap;
	int err;
	
	stderr = _get_stderr();
	err    = _get_errno();
	
	a0 = __libc_argv[0];
	p  = strrchr(a0, '/');
	if (p == NULL)
		p = a0;
	else
		p++;
	fprintf(stderr, "%s: ", p);
	
	if (fmt != NULL)
	{
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		fputs(": ", stderr);
	}
	
	fprintf(stderr, "%s\n", strerror(err));
}
