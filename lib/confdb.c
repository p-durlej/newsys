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
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <confdb.h>
#include <stdio.h>
#include <paths.h>

static void c_makedir(const char *pathname)
{
	char path[PATH_MAX];
	char *p;
	
	strcpy(path, pathname);
	
	p = strrchr(path, '/');
	if (p == NULL)
		return;
	*p = 0;
	
	mkdir(path, 0700);
}

static char *c_makepath(const char *name)
{
	const char *defroot = _PATH_H_DEFAULT;
	const char *envar   = "HOME";
	const char *subdir  = "/.conf/";
	const char *seltr   = name;
	const char *root;
	char *p;
	
	if (!strncmp(name, "/user/", 6))
		seltr = name + 6;
	else if (!strncmp(name, "/desk/", 6))
	{
		defroot = NULL;
		envar   = "DESKTOP_DIR";
		subdir  = "/";
		seltr   = name + 6;
	}
	
	root = getenv(envar);
	if (root == NULL)
	{
		if (defroot == NULL)
		{
			_set_errno(ENOENT);
			return NULL;
		}
		root = defroot;
	}
	
	if (asprintf(&p, "%s%s%s", root, subdir, seltr) < 0)
		return NULL;
	return p;
}

int c_load(const char *name, void *buf, int size)
{
	char *p;
	int cnt;
	int fd;
	
	p = c_makepath(name);
	if (!p)
		return -1;
	
	fd = open(p, O_RDONLY);
	if (fd < 0)
	{
		free(p);
		return -1;
	}
	
	cnt = read(fd, buf, size);
	if (cnt != size)
	{
		memset(buf, 0, size);
		if (cnt > 0)
			_set_errno(EINVAL);
		close(fd);
		free(p);
		return -1;
	}
	
	close(fd);
	free(p);
	return 0;
}

int c_save(const char *name, const void *buf, int size)
{
	int fd = -1;
	char *p;
	
	p = c_makepath(name);
	if (!p)
		return -1;
	
	fd = open(p, O_CREAT | O_WRONLY | O_TRUNC, 0600);
	if (fd < 0)
	{
		if (_get_errno() == ENOENT)
		{
			c_makedir(p);
			fd = open(p, O_CREAT | O_WRONLY | O_TRUNC, 0600);
			if (fd < 0)
				goto fail;
		}
		else
			goto fail;
	}
	
	if (write(fd, buf, size) != size)
	{
		unlink(p);
		goto fail;
	}
	
	close(fd);
	free(p);
	return 0;
fail:
	if (fd >= 0)
		close(fd);
	free(p);
	return -1;
}

int c_remove(const char *name)
{
	char *p;
	int r;
	
	p = c_makepath(name);
	if (!p)
		return -1;
	
	r = unlink(p);
	free(p);
	return r;
}

int c_access(const char *name)
{
	char *p;
	int r;
	
	p = c_makepath(name);
	if (!p)
		return -1;
	
	r = access(p, R_OK);
	free(p);
	return r;
}
