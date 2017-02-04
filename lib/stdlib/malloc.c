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

#include <kern/machine/machine.h>
#include <kern/page.h>

#include <priv/libc_globals.h>
#include <priv/libc.h>
#include <priv/sys.h>

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <os386.h>

#define NR_SMALL_PAGES	256
#define MMAGIC		0x0ace3860

#define PAGE_SIZE	4096 /* XXX */

#define MAP_PAGES	(PAGE_HEAP_END - PAGE_HEAP + 4095) / 4096 /* XXX */

struct small_head
{
	unsigned magic;
	size_t	 size;
	unsigned free;
	unsigned last;
};

static const unsigned first_page = PAGE_HEAP + MAP_PAGES + 1 + NR_SMALL_PAGES;
static const unsigned small_page = PAGE_HEAP + MAP_PAGES + 1;
static const unsigned map_page   = PAGE_HEAP;
static char *const    page_map   = pg2vap(PAGE_HEAP);

static struct small_head *small_heap;

static int trace = 0;

#define align_size(size)	(((size) + 15) & ~15)
#define is_small(p)		(((uintptr_t)(p) >> 12) < first_page)

void __libc_malloc_init(void)
{
	if (_pg_alloc(map_page, map_page + 4096))
	{
		_sysmesg("__libc_malloc_init: extremely low memory\n");
		_exit(255);
	}
	if (_pg_alloc(small_page, small_page + NR_SMALL_PAGES))
	{
		_sysmesg("__libc_malloc_init: extremely low memory\n");
		_exit(255);
	}
	small_heap = pg2vap(small_page);
	small_heap->magic = MMAGIC;
	small_heap->size  = pg2size(NR_SMALL_PAGES);
	small_heap->free  = 1;
	small_heap->last  = 1;
}

static void *small_malloc(size_t size)
{
	struct small_head *p = small_heap;
	struct small_head *n;
	char msg[256];
	
	size += sizeof(struct small_head);
	for (;;)
	{
		if (p->magic != MMAGIC)
			__libc_panic("heap corruption detected (small_malloc:0)");
		while (!p->last)
		{
			n = (void *)((char *)p + p->size);
			if (p->magic != MMAGIC || n->magic != MMAGIC)
				__libc_panic("heap corruption detected (small_malloc:1)");
			if (p->free && n->free)
			{
				p->size += n->size;
				p->last  = n->last;
			}
			else
				break;
		}
		if (p->free)
		{
			if (p->size == size)
			{
				if (trace)
				{
					sprintf(msg, "%s: small_malloc: %p, %i\n", __libc_argv[0], p + 1, size);
					_sysmesg(msg);
				}
				p->free = 0;
				return p + 1;
			}
			if (p->size >= size + sizeof(struct small_head))
			{
				n = (struct small_head *) ((char *)p + size);
				n->magic = MMAGIC;
				n->last	 = p->last;
				n->free	 = 1;
				n->size	 = p->size - size;
				p->size	 = size;
				p->last	 = 0;
				p->free	 = 0;
				if (trace)
				{
					sprintf(msg, "%s: small_malloc: %p, %i\n", __libc_argv[0], p + 1, size);
					_sysmesg(msg);
				}
				return p + 1;
			}
		}
		if (p->last)
			return NULL;
		p = (void *)((char *)p + p->size);
	}
}

static void small_free(void *ptr)
{
	struct small_head *h;
	
	h = (struct small_head *)ptr - 1;
	if (h->magic != MMAGIC)
	{
		if (trace)
		{
			char msg[256];
			
			sprintf(msg, "%s: small_free: %p\n", __libc_argv[0], ptr);
			_sysmesg(msg);
		}
		__libc_panic("heap corruption detected (small_free)");
	}
	h->free = 1;
}

void *malloc(size_t size)
{
	unsigned start = first_page;
	unsigned count = 0;
	unsigned i;
	void *p;
	
	size = align_size(size);
	if (size < 4092)
	{
		p = small_malloc(size);
		if (p)
			return p;
	}
	
	for (i = first_page; i < PAGE_HEAP_END; i++)
		if (!page_map[i - first_page])
		{
			count++;
			if ((size_t)count << 12 >= size + sizeof(size_t))
			{
				size_t *p = pg2vap(start);
				
				for (i = start; i < start + count; i++)
					page_map[i - first_page] = 1;
				
				if (_pg_alloc(start, start + count))
					return NULL;
				
				*p = size + sizeof *p;
				return p + 1;
			}
		}
		else
		{
			start = i + 1;
			count = 0;
		}
	
	_set_errno(ENOMEM);
	return NULL;
}

void free(void *ptr)
{
	unsigned start;
	unsigned end;
	size_t * p;
	unsigned i;
	
	if (!ptr)
		return;
	
	if (is_small(ptr))
	{
		small_free(ptr);
		return;
	}
	
	p	= (size_t *)ptr - 1;
	start	= (uintptr_t)p >> 12;
	end	= start + ((*p + 4095) >> 12);
	
	for (i = start; i < end; i++)
		page_map[i - first_page] = 0;
	
	_pg_free(start, end);
}

void *realloc(void *ptr, size_t size)
{
	struct small_head *she;
	size_t old_size;
	void *nptr;
	
	if (!ptr)
		return malloc(size);
	
	if (is_small(ptr))
	{
		she = ptr;
		she--;
		old_size = she->size - sizeof *she;
	}
	else
		old_size = *((size_t *)ptr - 1) - sizeof(size_t);
	
	size = align_size(size);
	if (old_size == size)
		return ptr;
	
	nptr = malloc(size);
	if (!nptr)
		return NULL;
	
	if (old_size > size)
		memcpy(nptr, ptr, size);
	else
		memcpy(nptr, ptr, old_size);
	
	free(ptr);
	return nptr;
}

void *calloc(size_t n, size_t m)
{
	size_t size = n * m;
	void *ptr;
	
	if (n && m && size / n != m)
		return NULL;
	
	ptr = malloc(size);
	if (!ptr)
		return NULL;
	
	memset(ptr, 0, size);
	return ptr;
}
