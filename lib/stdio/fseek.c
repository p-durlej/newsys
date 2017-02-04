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

#include <unistd.h>
#include <errno.h>
#include <stdio.h>

int fseek(FILE *f, long off, int whence)
{
	off_t o = ftell(f);
	
	if (o < 0)
		return -1;
	
	if (f->buf_write)
		fflush(f);
	
	f->buf_insize	= 0;
	f->buf_pos	= 0;
	
	switch (whence)
	{
	case SEEK_SET:
		o = lseek(f->fd, off, SEEK_SET);
		break;
	case SEEK_CUR:
		o = lseek(f->fd, o + off, SEEK_SET);
		break;
	case SEEK_END:
		o = lseek(f->fd, off, SEEK_END);
		break;
	default:
		_set_errno(EINVAL);
		f->err = 1;
		return -1;
	}
	
	if (o < 0)
	{
		f->err = 1;
		return -1;
	}
	return 0;
}

long ftell(FILE *f)
{
	off_t o;
	
	o = lseek(f->fd, 0L, SEEK_CUR);
	if (o < 0)
	{
		f->err = 1;
		return -1;
	}
	
	if (!f->buf_write)
		o += f->buf_pos - f->buf_insize;
	
	if (f->ungotc != EOF)
		return o - 1;
	return o;
}

void rewind(FILE *f)
{
	fseek(f, 0L, SEEK_SET);
}
