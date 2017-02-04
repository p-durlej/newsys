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

#include <priv/stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

FILE files[OPEN_MAX];

FILE *__libc_file_get(void)
{
	FILE *f;
	int i;
	
	for (i = 0; i < OPEN_MAX; i++)
		if (files[i].fd < 0)
		{
			f = &files[i];
			
			f->tty		= 0;
			f->mode		= 0;
			f->eof		= 0;
			f->err		= 0;
			f->buf		= NULL;
			f->buf_mode	= _IOFBF;
			f->buf_size	= 0;
			f->buf_pos	= 0;
			f->buf_insize	= 0;
			f->buf_write	= 0;
			f->buf_malloc	= 0;
			f->ungotc	= EOF;
			f->fputc	= __libc_putc_fbf;
			f->fgetc	= __libc_getc_fbf;
			
			return f;
		}
	
	_set_errno(ENOMEM);
	return NULL;
}

void __libc_file_put(FILE *f)
{
	close(f->fd);
	f->fd = -1;
}

FILE *fopen(const char *path, const char *mstr)
{
	int flags;
	int mode;
	FILE *f;
	int fd;
	
	switch (*mstr)
	{
	case 'r':
		mode  = O_RDONLY;
		flags = 0;
		break;
	case 'w':
		mode  = O_WRONLY;
		flags = O_CREAT | O_TRUNC;
		break;
	case 'a':
		mode  = O_WRONLY;
		flags = O_CREAT | O_APPEND;
		break;
	default:
		_set_errno(EINVAL);
		return NULL;
	}
	
	if (mstr[1] == '+')
		mode = O_RDWR;
	if (mstr[1] && mstr[2] == '+')
		mode = O_RDWR;
	
	fd = open(path, mode | flags, 0666);
	if (fd < 0)
		return NULL;
	
	f = fdopen(fd, mstr);
	if (!f)
		close(fd);
	return f;
}

FILE *fdopen(int fd, const char *mstr)
{
	FILE *f = __libc_file_get();
	
	if (!f)
		return NULL;
	
	switch (*mstr)
	{
	case 'r':
		f->mode = __LIBC_FMODE_R;
		break;
	case 'w':
		f->mode = __LIBC_FMODE_W;
		break;
	case 'a':
		f->mode = __LIBC_FMODE_W | __LIBC_FMODE_A;
		break;
	default:
		_set_errno(EINVAL);
		return NULL;
	}
	
	if (mstr[1] == '+')
		f->mode |= __LIBC_FMODE_R | __LIBC_FMODE_W;
	if (mstr[1] && mstr[2] == '+')
		f->mode |= __LIBC_FMODE_R | __LIBC_FMODE_W;
	
	if (isatty(fd))
		f->tty = 1;
	if (f->mode & __LIBC_FMODE_A)
		if (lseek(fd, 0L, SEEK_END)<0)
			return NULL;
	f->fd = fd;
	return f;
}

int fclose(FILE *f)
{
	if (f->mode & __LIBC_FMODE_W)
		fflush(f);
	if (f->buf_malloc)
		free(f->buf);
	__libc_file_put(f);
	return 0;
}
