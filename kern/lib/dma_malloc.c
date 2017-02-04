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

#include <kern/page.h>
#include <kern/lib.h>
#include <stdint.h>

void *dma_malloc(unsigned size)
{
	vpage pg_cnt = (size - 1) / PAGE_SIZE + 1;
	vpage pg;
	
	if (pg_adget(&pg, pg_cnt + 1))
		return NULL;
	if (pg_atdma(pg, pg_cnt))
	{
		pg_adput(pg, pg_cnt + 1);
		return NULL;
	}
	return (void *)((intptr_t)pg * PAGE_SIZE);
}

void dma_free(void *ptr, unsigned size)
{
	vpage pg_cnt = (size - 1) / PAGE_SIZE + 1;
	vpage pg = (intptr_t)ptr / PAGE_SIZE;
	
	pg_dtdma(pg, pg_cnt);
	pg_adput(pg, pg_cnt + 1);
}
