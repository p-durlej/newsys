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

#include <kern/umem.h>
#include <kern/lib.h>
#include <ringbuf.h>

int rb_uwrite(struct ringbuf *rb, const void *data, size_t *szp)
{
	size_t tail = (rb->head + rb->util) % rb->size;
	size_t split;
	size_t sz = *szp;
	int err;
	
	if (sz > rb->size - rb->util)
		sz = rb->size - rb->util;
	
	if (tail + sz > rb->size)
	{
		split = rb->size - tail;
		
		err = fucpy(rb->buf + tail, data, split);
		if (err)
			return err;
		
		err = fucpy(rb->buf, (char *)data + split, sz - split);
		if (err)
			return err;
	}
	else
		memcpy(rb->buf + tail, data, sz);
	
	rb->util += sz;
	*szp = sz;
	return 0;
}

int rb_uread(struct ringbuf *rb, void *buf, size_t *szp)
{
	size_t sz = *szp;
	size_t split;
	int err;
	
	if (sz > rb->util)
		sz = rb->util;
	
	if (rb->head + sz > rb->size)
	{
		split = rb->size - rb->head;
		
		err = tucpy(buf, rb->buf + rb->head, split);
		if (err)
			return err;
		
		err = tucpy((char *)buf + split, rb->buf, sz - split);
		if (err)
			return err;
	}
	else
	{
		err = tucpy(buf, rb->buf + rb->head, sz);
		if (err)
			return err;
	}
	
	rb->head += sz;
	rb->head %= rb->size;
	rb->util -= sz;
	*szp = sz;
	return 0;
}
