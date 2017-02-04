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
#include <kern/lib.h>
#include <stdint.h>

int phys_map(void **p, unsigned base, unsigned size, int cache)
{
	vpage npages = size / PAGE_SIZE + 1;
	vpage vpage;
	int err;
	
	err = pg_adget(&vpage, npages);
	if (err)
		return err;
	
	err = pg_atphys(vpage, npages, base / PAGE_SIZE, cache);
	if (err)
	{
		pg_adput(vpage, npages);
		return err;
	}
	
	*p = (void *)((intptr_t)vpage * PAGE_SIZE + base % PAGE_SIZE);
	return 0;
}

int phys_unmap(void *p, unsigned size)
{
	vpage npages = size / PAGE_SIZE + 1;
	vpage vpage = (intptr_t)p / PAGE_SIZE;
	
	pg_dtphys(vpage, npages);
	pg_adput(vpage, npages);
	return 0;
}
