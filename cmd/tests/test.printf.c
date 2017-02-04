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

#include <string.h>
#include <stdio.h>
#include <err.h>

#define VERBOSE	0

#define TEST(expect, ...)				\
	do {						\
		char buf[256];				\
							\
		snprintf(buf, sizeof buf, __VA_ARGS__);	\
		buf[sizeof buf - 1] = 0;		\
		if (strcmp(expect, buf))		\
			fail(__LINE__, expect, buf);	\
		else					\
			ok(__LINE__, expect);		\
	} while (0)

static int xcode;

static void fail(int ln, const char *expect, const char *result)
{
	warnx("line %i: expected \"%s\", got \"%s\"", ln, expect, result);
	xcode = 255;
}

static void ok(int ln, const char *expect)
{
#if VERBOSE
	warnx("line %i: ok \"%s\"", ln, expect);
#endif
}

int main()
{
	TEST("0.000000",	"%f",		0.);
	TEST("+0.000000",	"%+f",		0.);
	TEST(" 0.000000",	"% f",		0.);
	TEST("-0.000000",	"%+f",		-0.);
	TEST("-0.000000",	"% f",		-0.);
	
	TEST("0.234567",	"%f",		.234567);
	TEST("1.234567",	"%f",		1.234567);
	TEST("1.234568",	"%f",		1.2345678);
	TEST("1.234567",	"%f",		1.2345671);
	TEST("1234.567000",	"%f",		1234.567000);
	
	TEST("1235",		"%.0f",		1234.567000);
	TEST("1234.6",		"%.1f",		1234.567000);
	TEST("1234.57",		"%.2f",		1234.567000);
	TEST("1234.567",	"%.3f",		1234.567000);
	
	TEST("+0.234567",	"%+f",		.234567);
	TEST("+1.234567",	"%+f",		1.234567);
	
	TEST(" 0.234567",	"% f",		.234567);
	TEST(" 1.234567",	"% f",		1.234567);
	
	TEST("    0.2346",	"%10.4f",	.234567);
	TEST("    1.2346",	"%10.4f",	1.234567);
	
	TEST("   +0.2346",	"%+10.4f",	.234567);
	TEST("   +1.2346",	"%+10.4f",	1.234567);
	
	TEST("+0000.2346",	"%+010.4f",	.234567);
	TEST("+0001.2346",	"%+010.4f",	1.234567);
	
	TEST("+0.2346   ",	"%+-10.4f",	.234567);
	TEST("+1.2346   ",	"%+-10.4f",	1.234567);
	return xcode;
}
