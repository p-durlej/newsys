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

#include <sys/types.h>
#include <systat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>

static void alloc_mem(long mem)
{
	struct systat st;
	size_t sz;
	void *v;
	
	for (;;)
	{
		if (_systat(&st))
			err(1, "_systat");
		if (st.core_avail / 1024 <= mem)
			break;
		if (st.core_avail / 1024 >= mem + 2048)
			sz = 1048576;
		else
			sz = 2048;
		v = malloc(sz);
		if (v == NULL)
			err(1, "malloc");
		memset(v, 0, sz);
	}
	warnx("alloc_mem: %ji bytes free", (intmax_t)st.core_avail);
}

int main(int argc, char **argv)
{
	long mem = -1;
	int c;
	
	while (c = getopt(argc, argv, "m:"), c >= 0)
		switch (c)
		{
		case 'm':
			mem = strtol(optarg, NULL, 0);
			break;
		default:
			return 1;
		}
	
	if (mem >= 0)
		alloc_mem(mem);
	
	warnx("idle");
	for (;;)
		pause();
}
