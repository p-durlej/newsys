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
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <os386.h>

#include "elo.h"

void fail(const char *msg, int err) __attribute__((noreturn));

void elo_fail(const char *msg)
{
	fail(msg, 0);
}

void elo_mesg(const char *msg)
{
	_sysmesg(msg);
}

int elo_read(int fd, void *buf, size_t sz)
{
	ssize_t cnt;
	
	cnt = read(fd, buf, sz);
	if (cnt < 0)
		return -1;
	if (cnt != sz)
	{
		_set_errno(EINVAL);
		return -1;
	}
	return 0;
}

int elo_seek(int fd, off_t off)
{
	if (lseek(fd, off, SEEK_SET) < 0)
		return -1;
	return 0;
}

int elo_pgalloc(unsigned start, unsigned end)
{
	return _pg_alloc(start, end);
}

void *elo_malloc(size_t sz)
{
	return malloc(sz);
}

void elo_free(void *p)
{
	free(p);
}

void elo_csync(void *p, size_t sz)
{
	_csync(p, sz);
}
