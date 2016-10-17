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

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

void setbuf(FILE *f, char *buf)
{
	setbuffer(f, buf, BUFSIZ);
}

void setbuffer(FILE *f, char *buf, size_t size)
{
	if (buf)
		setvbuf(f, buf, _IOFBF, size);
	else
		setvbuf(f, NULL, _IONBF, size);
}

void setlinebuf(FILE *f)
{
	setvbuf(f, NULL, _IOLBF, 0);
}

int setvbuf(FILE *f, char *buf, int mode, size_t size)
{
	if (mode != _IONBF && mode != _IOFBF && mode != _IOLBF)
	{
		_set_errno(EINVAL);
		f->err = 1;
		return -1;
	}
	
	if (f->buf_write)
		fflush(f);
	
	if (f->buf_malloc)
		free(f->buf);
	
	if (mode == _IONBF)
	{
		f->fputc = __libc_putc_nbf;
		f->fgetc = __libc_getc_nbf;
	}
	else
	{
		f->fputc = __libc_putc_fbf;
		f->fgetc = __libc_getc_fbf;
	}
	
	f->buf_pos	= 0;
	f->buf_insize	= 0;
	f->buf_malloc	= 0;
	f->buf		= buf;
	f->buf_mode	= mode;
	f->buf_size	= size;
	return 0;
}
