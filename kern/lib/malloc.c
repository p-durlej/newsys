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
#include <kern/console.h>
#include <kern/config.h>
#include <kern/printk.h>
#include <kern/page.h>
#include <kern/lib.h>
#include <stdint.h>
#include <list.h>

static struct list kmalloc_list;

struct mhead
{
	struct list_item litem;
	char	 name[20];
	unsigned npages;
};

int kmalloc(void *buf, unsigned size, const char *name)
{
	struct mhead *mh;
	vpage npages;
	vpage page;
	int err;
	int len;
	
#if KMALLOC_DEBUG
	printk("kmalloc(%p, %i, \"%s\");\n", buf, size, name);
#endif
	
	npages = (size + sizeof(struct mhead) + PAGE_SIZE - 1) / PAGE_SIZE;
	
	err = pg_adget(&page, npages);
	if (err)
	{
#if KMALLOC_DEBUG
		perror("kmalloc: pg_adget", err);
#endif
		return err;
	}
	
	err = pg_atmem(page, npages, 0);
	if (err)
	{
#if KMALLOC_DEBUG
		perror("kmalloc: pg_atmem", err);
#endif
		pg_adput(page, npages);
		return err;
	}
	
	len = strlen(name);
	if (len >= sizeof mh->name - 1)
		len = sizeof mh->name - 1;
	
	mh = (struct mhead *)((intptr_t)page * PAGE_SIZE);
	mh->npages = npages;
	memcpy(mh->name, name, len);
	mh->name[len] = 0;
	
	list_app(&kmalloc_list, mh);
	
	*(void **)buf = mh + 1;
#if KMALLOC_DEBUG
	printk("kmalloc: fini, %p\n", *(void **)buf);
#endif
	return 0;
}

void *malloc(unsigned size)
{
	void *ptr = NULL;
	
	kmalloc(&ptr, size, "malloc");
	return ptr;
}

void free(void *ptr)
{
	struct mhead *mh;
	vpage npages;
	vpage page;
	
	if (!ptr)
		return;
	
	mh = ptr;
	mh--;
	
#if KMALLOC_DEBUG
	printk("free(%p);", ptr);
	printk("free: mh->name = \"%s\"\n", mh->name);
#endif
	
	list_rm(&kmalloc_list, mh);
	
	page   = (intptr_t)mh / PAGE_SIZE;
	npages = mh->npages;
	pg_dtmem(page, npages);
	pg_adput(page, npages);
}

int krealloc(void *buf, void *ptr, unsigned size, const char *name)
{
	unsigned old_size;
	struct mhead *mh;
	void *n;
	int err;
	
#if KMALLOC_DEBUG
	printk("krealloc(%p, %p, %i, \"%s\");\n", buf, ptr, size, name);
#endif
	
	if (!ptr)
		return kmalloc(buf, size, name);
	
	mh = ptr;
	mh--;
	
	old_size = mh->npages * PAGE_SIZE - sizeof *mh;
	
	err = kmalloc(&n, size, name);
	if (err)
	{
#if KMALLOC_DEBUG
		perror("krealloc: kmalloc", err);
#endif
		return err;
	}
	
	if (old_size < size)
		memcpy(n, ptr, old_size);
	else
		memcpy(n, ptr, size);
	free(ptr);
	
	*(void **)buf = n;
#if KMALLOC_DEBUG
	printk("krealloc: fini\n");
#endif
	return 0;
}

void *realloc(void *ptr, unsigned size)
{
	void *n = NULL;
	
	krealloc(&n, ptr, size, "malloc");
	return n;
}

void malloc_boot(void)
{
	list_init(&kmalloc_list, struct mhead, litem);
}

void malloc_dump(void)
{
	struct mhead *mh;
	unsigned tot = 0;
	
	for (mh = list_first(&kmalloc_list); mh != NULL; mh = list_next(&kmalloc_list, mh))
	{
		printk("malloc_dump: mh->name = \"%s\", mh->npages = %u\n", mh->name, mh->npages);
		tot += mh->npages;
	}
	printk("malloc_dump: tot = %u\n", tot);
	printk("malloc_dump: %u pages allocated\n", pg_total - pg_nfree - pg_nfree_dma);
}
