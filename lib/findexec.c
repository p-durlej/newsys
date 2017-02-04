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

#include <sys/stat.h>
#include <findexec.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <paths.h>

static char exe[PATH_MAX];

const char *_findexec(const char *cmd, const char *path)
{
	struct stat st;
	char *s;
	int cl;
	int l;
	
	if (strchr(cmd, '/'))
	{
		if (!access(cmd, X_OK))
		{
			strcpy(exe, cmd);
			goto found;
		}
		return NULL;
	}
	
	if (path == NULL)
	{
		path = getenv("PATH");
		if (path == NULL)
			path = _PATH_DEFPATH;
	}
	cl = strlen(cmd);
	
	for (;;)
	{
		s = strchr(path, ':');
		if (s)
			l = s - path;
		else
			l = strlen(path);
		
		if (l + cl + 1 < PATH_MAX)
		{
			memcpy(exe, path, l);
			if (l)
			{
				exe[l] = '/';
				l++;
			}
			strcpy(exe + l, cmd);
			if (!access(exe, X_OK))
				goto found;
		}
		if (!s)
			break;
		path = s + 1;
	}
	
	_set_errno(ENOENT);
	return NULL;
found:
	if (stat(exe, &st))
		return NULL;
	switch (st.st_mode & S_IFMT)
	{
	case S_IFREG:
		break;
	case S_IFDIR:
		_set_errno(EISDIR);
		return NULL;
	default:
		_set_errno(EACCES);
		return NULL;
	}
	return exe;
}
