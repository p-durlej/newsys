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

#ifndef _LIST_H
#define _LIST_H

#include <stddef.h>

struct list
{
	struct list_item *first;
	struct list_item *last;
	ptrdiff_t elem_off;
};

struct list_item
{
	struct list_item *next;
	struct list_item *prev;
};

#define list_init(li, type, ptrs)	_list_init((li), (ptrdiff_t)&((type *)0)->ptrs)
#define list_first(li)			_list_get((li), (li)->first)
#define list_last(li)			_list_get((li), (li)->last)
#define list_app(li, elem)		list_ia((li), list_last(li), (elem)); /* XXX */
#define list_pre(li, elem)		list_ib((li), list_first(li), (elem));
#define list_is_empty(li)		((li)->first == NULL)
void _list_init(volatile struct list *li, ptrdiff_t elem_off); /* XXX volatile */
void *_list_get(volatile struct list *li, void *elem);
void  list_ib(volatile struct list *li, void *elem1, void *elem2);
void  list_ia(volatile struct list *li, void *elem1, void *elem2);
void  list_rm(volatile struct list *li, void *elem);
void *list_next(volatile struct list *li, void *elem);
void *list_prev(volatile struct list *li, void *elem);

void  list_check(volatile struct list *li);

#endif
