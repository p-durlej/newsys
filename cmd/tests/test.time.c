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
#include <stdio.h>
#include <time.h>

static struct test
{
	int year, mon, mday, h, m, s;
	time_t time;
	int wday;
} tests[] =
{
	{ 1910,  1,  1,  0,  0,  0, -1893456000, 6 },
	{ 1920,  1,  1,  0,  0,  0, -1577923200, 4 },
	{ 1930,  1,  1,  0,  0,  0, -1262304000, 3 },
	{ 1940,  1,  1,  0,  0,  0,  -946771200, 1 },
	{ 1950,  1,  1,  0,  0,  0,  -631152000, 0 },
	{ 1960,  1,  1,  0,  0,  0,  -315619200, 5 },
	{ 1968,  1,  1,  0,  0,  0,   -63158400, 1 },
	{ 1968,  2, 28,  0,  0,  0,   -58147200, 3 },
	{ 1968,  3,  1,  0,  0,  0,   -57974400, 5 },
	{ 1968,  3,  2,  0,  0,  0,   -57888000, 6 },
	{ 1968, 12, 31,  0,  0,  0,   -31622400, 2 },
	{ 1968, 12, 31, 23, 59, 59,   -31536001, 2 },
	{ 1969,  1,  1,  0,  0,  0,   -31536000, 3 },
	{ 1969, 12, 31, 23, 59, 59,          -1, 3 },
	{ 1970,  1,  2,  0,  0,  0,       86400, 5 },
	{ 1972,  2, 28,  0,  0,  0,    68083200, 1 },
	{ 1972,  3,  1,  0,  0,  0,    68256000, 3 },
	{ 2000,  2, 28,  0,  0,  0,   951696000, 1 },
	{ 2000,  2, 29,  0,  0,  0,   951782400, 2 },
	{ 2000,  3,  1,  0,  0,  0,   951868800, 3 },
	{ 2004,  2, 28,  0,  0,  0,  1077926400, 6 },
	{ 2004,  2, 29,  0,  0,  0,  1078012800, 0 },
	{ 2004,  3,  1,  0,  0,  0,  1078099200, 1 },
	{ 2010,  2, 28,  0,  0,  0,  1267315200, 0 },
	// { 2010,  2, 29,  0,  0,  0, 1267401600 },
	{ 2010,  3,  1,  0,  0,  0,  1267401600, 1 },
	{ 3000,  2, 28,  0,  0,  0, 32508691200LL, 5 },
	// { 3000,  2, 29,  0,  0,  0, 32508777600LL },
	{ 3000,  3,  1,  0,  0,  0, 32508777600LL, 6 },
};

static int err;

static void test(struct test *t)
{
	struct tm *rtm;
	struct tm tm;
	time_t rt;
	
	tm.tm_year = t->year - 1900;
	tm.tm_mon  = t->mon  - 1;
	tm.tm_mday = t->mday;
	tm.tm_hour = t->h;
	tm.tm_min  = t->m;
	tm.tm_sec  = t->s;
	rt = mktime(&tm);
	if (rt != t->time)
	{
		fprintf(stderr, "test.time: %04i-%02i-%02i %02i:%02i:%02i: "
			       "got %lli, expected %lli\n",
			       t->year, t->mon, t->mday, t->h, t->m, t->s,
			       (long long)rt, (long long)t->time);
		err = 1;
	}
	
	rtm = gmtime(&t->time);
	if (tm.tm_year != rtm->tm_year ||
	    tm.tm_mon  != rtm->tm_mon  ||
	    tm.tm_mday != rtm->tm_mday ||
	    tm.tm_hour != rtm->tm_hour ||
	    tm.tm_min  != rtm->tm_min  ||
	    tm.tm_sec  != rtm->tm_sec  ||
	    t->wday    != rtm->tm_wday)
	{
		fprintf(stderr, "test.time: %04i-%02i-%02i,%i %02i:%02i:%02i: "
				"got %04i-%02i-%02i,%i %02i:%02i:%02i\n",
				t->year,
				t->mon,
				t->mday,
				t->wday,
				t->h,
				t->m,
				t->s,
				rtm->tm_year + 1900,
				rtm->tm_mon  + 1,
				rtm->tm_mday,
				rtm->tm_wday,
				rtm->tm_hour,
				rtm->tm_min,
				rtm->tm_sec);
		err = 1;
	}
}

int main()
{
	int i;
	
	clearenv();
	setenv("TZ", "", 1);
	for (i = 0; i < sizeof tests / sizeof *tests; i++)
		test(&tests[i]);
	return err;
}
