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

#ifndef _TIME_H
#define _TIME_H

#include <sys/types.h>
#include <sys/time.h>
#include <libdef.h>

typedef long clock_t;

#ifndef _KERN_

#define CLOCKS_PER_SEC	100

struct tm
{
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_year;
	int tm_mon;
	int tm_mday;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

extern char *tzname[2];
extern time_t timezone;

void	    tzset(void);

time_t	    time(time_t *t);
int	    stime(const time_t *t);

double	    difftime(time_t t1, time_t t0);

char *	    asctime(const struct tm *time);
char *	    ctime(const time_t *time);
struct tm * gmtime(const time_t *time);
struct tm * localtime(const time_t *time);
time_t	    mktime(const struct tm *time);

size_t	    strftime(char *s, size_t max, const char *fmt, const struct tm *tm);
void	    cltime(struct tm *time);

clock_t	    clock(void);
#endif

#endif
