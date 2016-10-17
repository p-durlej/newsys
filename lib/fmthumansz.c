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

#include <fmthuman.h>

const char *fmthumanoffs(off_t sz, int frac)
{
	static const char buf[64];
	const char *suf;
	uintmax_t v;
	
	if (sz >= 1024 * 1048576)
	{
		v   = (uintmax_t)sz * 10 / 1073741824;
		suf = "G";
	}
	else if (sz >= 1024 * 1024)
	{
		v   = (uintmax_t)sz * 10 / 1048576;
		suf = "M";
	}
	else if (sz >= 1024)
	{
		v   = (uintmax_t)sz * 10 / 1024;
		suf = "K";
	}
	else
	{
		sprintf(buf, "%ju", (uintmax_t)sz);
		return buf;
	}
	
	if (frac)
		sprintf(buf, "%ju.%ju%s", v / 10, v % 10, suf);
	else
		sprintf(buf, "%ju%s", v / 10, suf);
	return buf;
}

const char *fmthumanoff(off_t sz, int frac)
{
	static const char buf[64];
	const char *suf;
	uintmax_t v;
	
	if (sz >= 1024 * 1048576)
	{
		v   = (uintmax_t)sz * 10 / 1073741824;
		suf = " GiB";
	}
	else if (sz >= 1024 * 1024)
	{
		v   = (uintmax_t)sz * 10 / 1048576;
		suf = " MiB";
	}
	else if (sz >= 1024)
	{
		v   = (uintmax_t)sz * 10 / 1024;
		suf = " KiB";
	}
	else
	{
		sprintf(buf, "%ju", (uintmax_t)sz);
		return buf;
	}
	
	if (frac)
		sprintf(buf, "%ju.%ju%s", v / 10, v % 10, suf);
	else
		sprintf(buf, "%ju%s", v / 10, suf);
	return buf;
}

const char *fmthumansz(size_t sz, int frac)
{
	return fmthumanoff(sz, frac);
}
