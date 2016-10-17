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

#include <kern/page.h>
#include <kern/task.h>
#include <kern/lib.h>

int copy_pages(struct task *t)
{
#define TMP_TAB ((unsigned *)(PAGE_TMP << 12))
	unsigned pg;
	unsigned i;
	const void *s;
	void *d;
	int err;
	
	err = pg_newdir(t->pg_dir);
	if (err)
		return err;
	
	for (i = PAGE_USER; i < PAGE_USER_END; i++)
	{
		if (!pg_dir[i >> 10])
		{
			i += 1023;
			continue;
		}
		if (!t->pg_dir[i >> 10])
		{
			err = pg_alloc(&pg);
			if (err)
				goto fail;
			t->pg_dir[i >> 10] = (pg << 12) | PGF_PRESENT | PGF_WRITABLE | PGF_USER;
			pg_tab[PAGE_TMP] = (pg << 12) | PGF_PRESENT | PGF_WRITABLE;
			pg_utlb();
			memset(TMP_TAB, 0, PAGE_SIZE);
		}
		else
		{
			pg_tab[PAGE_TMP] = t->pg_dir[i >> 10];
			pg_utlb();
		}
		
		if (pg_tab[i] & PGF_AUTO)
		{
			TMP_TAB[i & 0x3ff] = PGF_AUTO;
			continue;
		}
		
		if (!(pg_tab[i] & PGF_PRESENT))
			continue;
		
		err = pg_alloc(&pg);
		if (err)
			goto fail;
		
		TMP_TAB[i & 0x3ff] = (pg << 12) | PGF_PRESENT | PGF_WRITABLE | PGF_USER;
		
		d = (void *)((intptr_t)PAGE_TMP << 12);
		s = (const void *)((intptr_t)i << 12);
		
		pg_tab[PAGE_TMP] = (pg << 12) | PGF_PRESENT | PGF_WRITABLE;
		pg_utlb();
		memcpy(d, s, PAGE_SIZE);
	}
	
	pg_tab[PAGE_TMP] = 0;
	pg_utlb();
	return 0;
	
fail:
	pg_tab[PAGE_TMP] = 0;
	pg_utlb();
	return err;
#undef TMP_TAB
}
