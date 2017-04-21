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

#include <sysload/i386-pc.h>
#include <sysload/console.h>
#include <sysload/kparam.h>
#include <sysload/main.h>
#include <sysload/mem.h>

#include <stdint.h>
#include <string.h>

#define E820_MEM	1
#define E820_RSVD	2
#define E820_ACPI	3
#define E820_NVS	4

struct kmapent *mem_map;
int		mem_cnt;
uint64_t	mem_size;

static size_t mem_lbrk = 0x100000;
static size_t mem_hbrk;

struct e820_run
{
	uint64_t base;
	uint64_t size;
	uint32_t type;
};

void mem_preinit(void)
{
	uint16_t mem_size_88;
	struct e820_run ard;
	struct kmapent *p;
	
	bcp.eflags &= ~1;
	bcp.eax	    = 0x8800;
	bcp.intr    = 0x15;
	bioscall();
	if (bcp.eflags & 1)
		fail("mem_preinit: BIOS call failed", NULL);
	mem_size_88 = bcp.eax;
	mem_hbrk    = 0x100000 + mem_size_88 * 1024;
	
	p = (void *)(uintptr_t)conv_mem_hbrk;
	
	bcp.intr = 0x15;
	bcp.ebx	 = 0;
	do
	{
		bcp.eax	 = 0xe820;
		bcp.ecx	 = sizeof *p;
		bcp.edx	 = 'SMAP';
		bcp.es	 = (intptr_t)&ard >> 4;
		bcp.edi	 = (intptr_t)&ard & 15;
		bioscall();
		if (bcp.eax != 'SMAP' || (bcp.eflags & 1))
			break;
		memset(--p, 0, sizeof *p);
		p->type = ard.type == E820_MEM ? KMAPENT_MEM : KMAPENT_RSVD;
		p->base = ard.base;
		p->size = ard.size;
	} while (bcp.ebx);
	
	if ((uintptr_t)p == conv_mem_hbrk)
	{
		memset(--p, 0, sizeof *p);
		p->type  = KMAPENT_MEM;
		p->base  = 0x100000;
		p->size  = mem_size_88;
		p->size *= 1024;
	}
	
	mem_cnt	      = (struct kmapent *)(intptr_t)conv_mem_size - p;
	mem_map	      = p;
	pm_esp	      = (intptr_t)p;
	conv_mem_hbrk = (intptr_t)p;
}

void *mem_alloc(size_t sz, int ma)
{
	sz +=  15;
	sz &= ~15;
	switch (ma)
	{
	case MA_SYSLOAD:
		if (mem_lbrk + sz > mem_hbrk)
			fail("Memory exhausted", "");
		mem_hbrk -= sz;
		return (void *)mem_hbrk;
	case MA_SYSTEM:
	case MA_KERN:
		if (mem_lbrk + sz > mem_hbrk)
			fail("Memory exhausted", "");
		mem_lbrk += sz;
		return (void *)(mem_lbrk - sz);
	default:
		fail("Invalid memory area specification", "");
	}
}

void mem_init(void)
{
	int i;
	
	if (!mem_cnt)
		return;

	mem_size = 0;
	for (i = 0; i < mem_cnt; i++)
		if (mem_map[i].type == KMAPENT_MEM)
			mem_size += mem_map[i].size;
}

static void mem_rmov(int i)
{
	memmove(&mem_map[i], &mem_map[i + 1], sizeof *mem_map * (mem_cnt - i - 1));
	mem_cnt--;
}

void mem_adjmap(void)
{
	int i;
	
	for (i = 0; i < mem_cnt; i++)
	{
		if (mem_map[i].type != KMAPENT_MEM)
			continue;
		if (mem_map[i].base >= mem_lbrk)
			continue;
		
		if (mem_map[i].base == 0)
		{
			mem_map[i].size -= 0x20000;
			mem_map[i].base  = 0x20000;
			
			conv_mem_hbrk = (uintptr_t)--mem_map;
			mem_cnt++;
			i++;
			
			mem_map[0].type  = KMAPENT_SYSTEM;
			mem_map[0].base  = 0x00000;
			mem_map[0].size  = 0x20000;
			
			continue;
		}
		
		if (mem_map[i].base >= 0x100000 && mem_map[i].base + mem_map[i].size <= mem_lbrk)
		{
			mem_map[i].type = KMAPENT_SYSTEM;
			continue;
		}
		
		if (mem_map[i].base + mem_map[i].size <= 0x100000)
			continue;
		
		size_t adj = mem_lbrk - mem_map[i].base;
		
		mem_map[i].size -= adj;
		mem_map[i].base  = mem_lbrk;
		
		if (conv_mem_hbrk != (uintptr_t)mem_map)
			continue;
		
		mem_map--;
		mem_cnt++;
		i++;
		
		mem_map[0].base = mem_lbrk - adj;
		mem_map[0].size = adj;
		mem_map[0].type = KMAPENT_SYSTEM;
		
		conv_mem_hbrk = (uintptr_t)mem_map;
	}
}
