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

#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

int _readdir(int fd, struct dirent *de, int index);

DIR *opendir(const char *path)
{
	struct stat st;
	DIR *dir;
	
	dir = malloc(sizeof *dir);
	if (!dir)
		return NULL;
	
	dir->fd = open(path, O_RDONLY);
	if (dir->fd < 0)
	{
		free(dir);
		return NULL;
	}
	fcntl(dir->fd, F_SETFD, 1);
	
	if (fstat(dir->fd, &st))
	{
		closedir(dir);
		return NULL;
	}
	
	if (!S_ISDIR(st.st_mode))
	{
		closedir(dir);
		_set_errno(ENOTDIR);
		return NULL;
	}
	
	dir->index = 0;
	return dir;
}

int closedir(DIR *dir)
{
	if (!dir)
	{
		_set_errno(EBADF);
		return -1;
	}
	close(dir->fd);
	free(dir);
	return 0;
}

struct dirent *readdir(DIR *dir)
{
	int saved_errno = _get_errno();
	
	do
	{
		if (_readdir(dir->fd, &dir->de, dir->index))
		{
			if (_get_errno() == ENOENT)
				_set_errno(saved_errno);
			return NULL;
		}
		dir->index++;
	} while(!dir->de.d_ino);
	
	return &dir->de;
}

void rewinddir(DIR *dir)
{
	dir->index = 0;
}

void seekdir(DIR *dir, off_t off)
{
	dir->index = off;
}

off_t telldir(DIR *dir)
{
	return dir->index;
}
