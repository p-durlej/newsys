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

#ifdef _KERN_
#include <kern/umem.h>
#include <kern/lib.h>
#else
#include <string.h>
#endif

#include <priv/libc.h>
#include <ringbuf.h>

void rb_check(struct ringbuf *rb)
{
	if (rb->util > rb->size)
		__libc_panic("rb_check: rb->util > rb->size");
}

void rb_init(struct ringbuf *rb, void *buf, size_t sz)
{
	memset(rb, 0, sizeof *rb);
	rb->buf	 = buf;
	rb->size = sz;
}

size_t rb_write(struct ringbuf *rb, const void *data, size_t sz)
{
	size_t tail = (rb->head + rb->util) % rb->size;
	size_t split;
	
	rb_check(rb);
	
	if (sz > rb->size - rb->util)
		sz = rb->size - rb->util;
	
	if (tail + sz > rb->size)
	{
		split = rb->size - tail;
		
		memcpy(rb->buf + tail, data, split);
		memcpy(rb->buf, (char *)data + split, sz - split);
	}
	else
		memcpy(rb->buf + tail, data, sz);
	
	rb->util += sz;
	return sz;
}

size_t rb_read(struct ringbuf *rb, void *buf, size_t sz)
{
	size_t split;
	
	rb_check(rb);
	
	if (sz > rb->util)
		sz = rb->util;
	
	if (rb->head + sz > rb->size)
	{
		split = rb->size - rb->head;
		
		memcpy(buf, rb->buf + rb->head, split);
		memcpy((char *)buf + split, rb->buf, sz - split);
	}
	else
		memcpy(buf, rb->buf + rb->head, sz);
	
	rb->head += sz;
	rb->head %= rb->size;
	rb->util -= sz;
	return sz;
}

size_t rb_peek(struct ringbuf *rb, void *buf, size_t sz, size_t off)
{
	size_t split;
	
	rb_check(rb);
	
	if (off >= rb->util)
		return 0;
	
	if (sz > rb->util - off)
		sz = rb->util - off;
	
	off += rb->head;
	off %= rb->size;
	
	if (off + sz > rb->size)
	{
		split = rb->size - off;
		
		memcpy(buf, rb->buf + off, split);
		memcpy((char *)buf + split, rb->buf, sz - split);
	}
	else
		memcpy(buf, rb->buf + off, sz);
	
	return sz;
}
