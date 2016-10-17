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

#include <priv/libc.h>
#include <list.h>

#define LIST_DEBUG	0

#if LIST_DEBUG
#define list_cck(li)	do { list_check(li); } while (0)
#else
#define list_cck(li)	do { } while (0)
#endif

void _list_init(volatile struct list *li, ptrdiff_t elem_off)
{
	li->first    = NULL;
	li->last     = NULL;
	li->elem_off = elem_off;
}

void *_list_get(volatile struct list *li, void *elem)
{
	if (!elem)
		return NULL;
	return (char *)elem - li->elem_off;
}

void list_ib(volatile struct list *li, void *elem1, void *elem2)
{
	struct list_item *e1 = (void *)((char *)elem1 + li->elem_off);
	struct list_item *e2 = (void *)((char *)elem2 + li->elem_off);
	
	list_cck(li);
	
	if (!elem1)
	{
		if (li->first)
			__libc_panic("list_prep: !elem1 && li->first");
		li->first = e2;
		li->last  = e2;
		e2->prev  = NULL;
		e2->next  = NULL;
		
		list_cck(li);
		return;
	}
	
	e2->next = e1;
	e2->prev = e1->prev;
	
	if (e1->prev)
		e1->prev->next = e2;
	else
		li->first = e2;
	e1->prev = e2;
	
	list_cck(li);
}

void list_ia(volatile struct list *li, void *elem1, void *elem2)
{
	struct list_item *e1 = (void *)((char *)elem1 + li->elem_off);
	struct list_item *e2 = (void *)((char *)elem2 + li->elem_off);
	
	list_cck(li);
	
	if (!elem1)
	{
		if (li->first)
			__libc_panic("list_app: !elem1 && li->first");
		li->first = e2;
		li->last  = e2;
		e2->prev  = NULL;
		e2->next  = NULL;
		
		list_cck(li);
		return;
	}
	
	e2->next = e1->next;
	e2->prev = e1;
	
	if (e1->next)
		e1->next->prev = e2;
	else
		li->last = e2;
	e1->next = e2;
	
	list_cck(li);
}

void list_rm(volatile struct list *li, void *elem)
{
	struct list_item *e = (void *)((char *)elem + li->elem_off);
	
	list_cck(li);
	
	if (e == li->first)
		li->first = e->next;
	if (e == li->last)
		li->last = e->prev;
	
	if (e->next)
		e->next->prev = e->prev;
	if (e->prev)
		e->prev->next = e->next;
	
	list_cck(li);
}

void *list_next(volatile struct list *li, void *elem)
{
	struct list_item *e;
	
	list_cck(li);
	
	if (!elem)
		__libc_panic("list_next: !elem");
	
	e = (struct list_item *) ((char *)elem + li->elem_off);
	if (!e->next)
		return NULL;
	return (char *)e->next - li->elem_off;
}

void *list_prev(volatile struct list *li, void *elem)
{
	struct list_item *e;
	
	list_cck(li);
	
	if (!elem)
		__libc_panic("list_prev: !elem");
	
	e = (struct list_item *) ((char *)elem + li->elem_off);
	if (!e->prev)
		return NULL;
	return (char *)e->prev - li->elem_off;
}

void list_check(volatile struct list *li)
{
	struct list_item *p, *i, *f;
	
	i = li->first;
	f = li->first;
	p = NULL;
	while (i)
	{
		if (i->prev != p)
			__libc_panic("list_check: bad prev");
		
		p = i;
		i = i->next;
		if (f != NULL)
		{
			f = f->next;
			if (f != NULL)
			{
				f = f->next;
				if (i == f)
					__libc_panic("list_check: cycle");
			}
		}
	}
}
