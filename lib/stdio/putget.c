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

#include <priv/stdio.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int fflush(FILE *f)
{
	const char *p;
	size_t resid;
	ssize_t rcnt;
	int err = 0;
	int i;
	
	if (f == NULL)
	{
		for (i = 0; i < sizeof files / sizeof *files; i++)
			if ((files[i].mode & __LIBC_FMODE_W) && fflush(&files[i]))
				err = EOF;
		return err;
	}
	
	if (!(f->mode & __LIBC_FMODE_W))
	{
		f->err = 1;
		_set_errno(EBADF);
		return EOF;
	}
	if (!f->buf)
	{
		f->buf_write = 0;
		return 0;
	}
	if (f->buf_write && f->buf_pos)
	{
		resid = f->buf_pos;
		p     = f->buf;
		
		f->buf_pos = 0;
		while (resid)
		{
			rcnt = write(f->fd, f->buf, resid);
			if (rcnt < 0)
			{
				f->err = 1;
				return EOF;
			}
			resid -= rcnt;
		}
	}
	return 0;
}

int __libc_putc_nbf(int c, FILE *f)
{
	if (write(f->fd, &c, 1) == 1)
		return c;
	f->err = 1;
	return EOF;
}

int __libc_putc_fbf(int c, FILE *f)
{
	if (!f->buf_write)
	{
		f->buf_write  = 1;
		f->buf_insize = 0;
		f->buf_pos    = 0;
	}
	if (!f->buf)
	{
		f->buf = malloc(BUFSIZ);
		if (!f->buf)
		{
			f->err = 1;
			return EOF;
		}
		f->buf_size   = BUFSIZ;
		f->buf_malloc = 1;
	}
	if (f->buf_pos == f->buf_size && fflush(f))
		return EOF;
	f->buf[f->buf_pos] = c;
	f->buf_pos++;
	if (f->buf_mode == _IOLBF && c == '\n')
		return fflush(f);
	return c;
}

int __libc_putc_str(int c, FILE *f)
{
	if (f->strout.pos + 1 < f->strout.cap)
		f->strout.str[f->strout.pos] = c;
	f->strout.pos++;
	return c;
}

int fputc(int c, FILE *f)
{
	if (!(f->mode & __LIBC_FMODE_W))
	{
		_set_errno(EBADF);
		f->err = 1;
		return EOF;
	}
	
	return f->fputc(c & 0xff, f);
}

int (putc)(int c, FILE *f)
{
	return fputc(c, f);
}

int __libc_getc_nbf(FILE *f)
{
	unsigned char c;
	int cnt;
	
	if (f->tty)
		fflush(NULL);
	
	cnt = read(f->fd, &c, 1);
	if (cnt == 1)
		return c;
	if (cnt < 0)
		f->err = 1;
	return EOF;
}

int __libc_getc_fbf(FILE *f)
{
	if (f->buf_write)
		fflush(f);
	if (!f->buf)
	{
		f->buf = malloc(BUFSIZ);
		if (!f->buf)
		{
			f->err = 1;
			return EOF;
		}
		f->buf_size   = BUFSIZ;
		f->buf_malloc = 1;
	}
	if (f->buf_pos == f->buf_insize)
	{
		if (f->tty)
			fflush(NULL);
		
		f->buf_pos    = 0;
		f->buf_insize = read(f->fd, f->buf, f->buf_size);
		if (f->buf_insize < 0)
		{
			f->buf_insize = 0;
			f->err = 1;
			return EOF;
		}
		if (!f->buf_insize)
		{
			f->eof = 1;
			return EOF;
		}
	}
	return f->buf[f->buf_pos++] & 0xff;
}

int fgetc(FILE *f)
{
	if (!(f->mode & __LIBC_FMODE_R))
	{
		_set_errno(EBADF);
		f->err = 1;
		return EOF;
	}
	
	if (f->ungotc != EOF)
	{
		int c = f->ungotc;
		
		f->ungotc = EOF;
		return c;
	}
	
	return f->fgetc(f);
}

int (getc)(FILE *f)
{
	return fgetc(f);
}

int fputs(const char *s, FILE *f)
{
	while (*s)
	{
		if (fputc(*s, f) == EOF)
			return EOF;
		s++;
	}
	return 0;
}

int puts(const char *s)
{
	if (fputs(s, _get_stdout()))
		return EOF;
	if (fputc('\n', _get_stdout()) == EOF)
		return EOF;
	return 0;
}

char *gets(char *s)
{
	FILE *stdin = _get_stdin();
	char *p = s;
	
	for (;;)
	{
		int c = fgetc(stdin);
		
		*p = c;
		
		switch (c)
		{
		case EOF:
			*p = 0;
			if (p == s)
				return NULL;
			return s;
		case '\n':
			*p = 0;
			return s;
		default:
			p++;
		}
	}
}

char *fgets(char *s, int size, FILE *f)
{
	int i;
	
	for (i = 0; i < size - 1; i++)
	{
		int c;
		
		while (c = fgetc(f), !c);
		
		s[i] = c;
		switch (c)
		{
		case EOF:
			s[i] = 0;
			if (!*s)
				return NULL;
			return s;
		case '\n':
			s[i + 1] = 0;
			return s;
		}
	}
	return NULL;
}

int ungetc(int c, FILE *f)
{
	if (f->ungotc == EOF)
	{
		f->ungotc = c & 0xff;
		return c;
	}
	f->err = 1;
	return EOF;
}

int putchar(int c)
{
	return fputc(c, _get_stdout());
}

int getchar(void)
{
	return fgetc(_get_stdin());
}
