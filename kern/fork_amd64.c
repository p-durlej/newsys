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

#include <kern/console.h>
#include <kern/errno.h>
#include <kern/page.h>
#include <kern/task.h>

#define DSHIFT	(1 << 28)

static void skip(vpage *ip, int shift)
{
	*ip &= -1LU << shift;
	*ip |= (1LU << shift) - 1;
}

int copy_pages(struct task *t)
{
	static uint64_t *dtabs[4] =
	{
		pg2vap(PAGE_TAB  | DSHIFT),
		pg2vap(PAGE_DIR  | DSHIFT),
		pg2vap(PAGE_PDP  | DSHIFT),
		pg2vap(PAGE_PML4 | DSHIFT)
	};
	
	char *npa = pg2vap(DSHIFT);
	
	ppage pg;
	vpage i;
	int err;
	
	err = pg_newdir(t->pg_dir);
	if (err)
		return err;
	
	pg_tabs[3][2] = t->pg_dir[0];
	pg_tabs[3][3] = t->pg_dir[1];
	pg_utlb();
	
	for (i = 1; i < 512; i++)
	{
		if (!pg_tabs[2][i])
			continue;
		
		if (dtabs[2][i])
			continue;
		
		if (pg_tabs[2][i] & PGF_PRESENT)
		{
			err = pg_alloc(&pg);
			if (err)
				goto err;
			
			dtabs[2][i] = (pg << 12) | PGF_HIGHLEVEL;
			pg_utlb();
			
			memset(&dtabs[1][i << 9], 0, 4096);
			continue;
		}
		
		if (dtabs[2][i])
			panic("copy_pages: PDPE not present");
		dtabs[2][i] = 0;
	}
	
	for (i = 512; i < (1 << 18); i++)
	{
		if (!dtabs[2][i >> 9])
		{
			skip(&i, 9);
			continue;
		}
		
		if (dtabs[1][i])
			continue;
		
		if (pg_tabs[1][i] & PGF_PRESENT)
		{
			err = pg_alloc(&pg);
			if (err)
				goto err;
			
			dtabs[1][i] = (pg << 12) | PGF_HIGHLEVEL;
			pg_utlb();
			
			memset(&dtabs[0][i << 9], 0, 4096);
			continue;
		}
		
		if (pg_tabs[1][i])
			panic("copy_pages: PDE not present");
		dtabs[1][i] = 0;
	}
	
	for (i = (1 << 18); i < (1 << 27); i++)
	{
		if (!dtabs[2][i >> 18])
		{
			skip(&i, 18);
			continue;
		}
		
		if (!dtabs[1][i >> 9])
		{
			skip(&i, 9);
			continue;
		}
		
		if (dtabs[0][i])
			continue;
		
		if (pg_tabs[0][i] & PGF_PRESENT)
		{
			err = pg_alloc(&pg);
			if (err)
				goto err;
			
			dtabs[0][i] = (pg << 12) | PGF_HIGHLEVEL;
			pg_utlb();
			
			memcpy(npa + i * 4096, pg2vap(i), 4096);
			continue;
		}
		
		dtabs[0][i] = pg_tabs[0][i];
	}
err:
	pg_tabs[3][2] = 0;
	pg_tabs[3][3] = 0;
	pg_utlb();
	
	return err;
}
