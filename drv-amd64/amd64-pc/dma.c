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

#include <kern/lib.h>
#include <kern/hw.h>
#include <stdint.h>

void dma_setup(int nr, struct dma_setup *ds)
{
	static const int page_reg[8] = { 0x87, 0x83, 0x81, 0x82, 0x00, 0x8b, 0x89, 0x8a };
	
	unsigned count = ds->count - 1;
	
	dma_dis(nr);
	
	if (nr < 4)
	{
		outb(0x0c, 0);
		outb(nr * 2, (uintptr_t)ds->addr);
		outb(nr * 2, (uintptr_t)ds->addr >> 8);
		outb(nr * 2 + 1, count);
		outb(nr * 2 + 1, count >> 8);
		outb(0x0b, nr | ds->mode | ds->dir);
	}
	else
	{
		nr -= 4;
		
		outb(0xc8, 0);
		outb(0xc0 + nr * 4, (uintptr_t)ds->addr);
		outb(0xc0 + nr * 4, (uintptr_t)ds->addr >> 8);
		outb(0xc0 + nr * 4 + 2, count);
		outb(0xc0 + nr * 4 + 2, count  >> 8);
		outb(0xd6, nr | ds->mode | ds->dir);
	}
	outb(page_reg[nr], (uintptr_t)ds->addr >> 16);
}

void dma_ena(int nr)
{
	if (nr < 4)
		outb(0x0a, nr);
	else
		outb(0xd4, nr - 4);
}

void dma_dis(int nr)
{
	if (nr < 4)
		outb(0x0a, nr | 4);
	else
		outb(0xd4, (nr - 4) | 4);
}
