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

#include <wingui_buf.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

static char *wbu_name(void)
{
	char *p;
	
	p = getenv("HOME");
	if (!p)
	{
		_set_errno(EINVAL);
		return NULL;
	}
	
	if (asprintf(&p, "%s/.clipbrd", p) < 0)
		return NULL;
	return p;
}

int wbu_open(int write)
{
	char *path;
	int fd;
	int se;
	
	path = wbu_name();
	if (!path)
		return -1;
	
	if (!write)
		return open(path, O_RDONLY);
	
	se = _get_errno();
restart:
	if (unlink(path) && _get_errno() != ENOENT)
		return -1;
	fd = open(path, O_CREAT | O_EXCL | O_WRONLY, 0600);
	if (fd < 0)
	{
		if (_get_errno() == EEXIST)
			goto restart;
		return -1;
	}
	_set_errno(se);
	return fd;
}

int wbu_store(char *text, size_t len)
{
	size_t cnt;
	int fd;
	int se;
	
	se = _get_errno();
	fd = wbu_open(1);
	if (fd < 0)
		return -1;
	
	cnt = write(fd, text, len);
	if (cnt < 0)
	{
		close(fd);
		return -1;
	}
	if (cnt != len)
	{
		_set_errno(EINVAL);
		close(fd);
		return -1;
	}
	if (close(fd))
		return -1;
	_set_errno(se);
	return 0;
}

char *wbu_load(void)
{
	struct stat st;
	char *buf;
	size_t cnt;
	int fd;
	
	fd = wbu_open(0);
	if (fd < 0)
	{
		if (_get_errno() == ENOENT)
			return strdup("");
		return NULL;
	}
	fstat(fd, &st);
	
	if (st.st_size + 1 <= st.st_size)
	{
		_set_errno(EFBIG);
		close(fd);
		return NULL;
	}
	
	buf = malloc(st.st_size + 1);
	if (!buf)
	{
		close(fd);
		return NULL;
	}
	
	cnt = read(fd, buf, st.st_size);
	if (cnt < 0)
	{
		free(buf);
		close(fd);
		return NULL;
	}
	if (cnt != st.st_size)
	{
		_set_errno(EINVAL);
		free(buf);
		close(fd);
		return NULL;
	}
	buf[st.st_size] = 0;
	
	close(fd);
	return buf;
}

int wbu_clear(void)
{
	char *path;
	int se;
	
	se = _get_errno();
	path = wbu_name();
	if (!path)
		return -1;
	if (unlink(path) && _get_errno() != ENOENT)
		return -1;
	_set_errno(se);
	return 0;
}
