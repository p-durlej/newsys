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

#include <sysload/kparam.h>
#include <sysload/flags.h>

#include <kern/printk.h>
#include <kern/start.h>

static void show_mem_size(void)
{
	struct kmapent *me;
	uint64_t total = 0;
	int i;
	
	me = kparam.mem_map;
	for (i = 0; i < kparam.mem_cnt; i++, me++)
		if (me->type == KMAPENT_MEM || me->type == KMAPENT_SYSTEM)
			total += me->size;
	
	printk("Memory size: ");
	if (total > 10000)
		printk("%i MiB\n", total / 1048576);
	else
		printk("%i KiB\n", total / 1024);
}

static void show_mem_map(void)
{
	struct kmapent *me;
	int i;
	
	printk("memory map:\n\n");
	me = kparam.mem_map;
	for (i = 0; i < kparam.mem_cnt; i++, me++)
	{
		const char *ts = "UNKNOWN";
		
		switch (me->type)
		{
		case KMAPENT_MEM:
			ts = "MEM";
			break;
		case KMAPENT_RSVD:
			ts = "RSVD";
			break;
		case KMAPENT_SYSTEM:
			ts = "SYSTEM";
			break;
		default:
			;
		}
		
		printk(" base = 0x%x%x, size = 0x%x%x, type = %s\n",
			(unsigned)(me->base >> 32),
			(unsigned) me->base,
			(unsigned)(me->size >> 32),
			(unsigned) me->size,
			ts);
	}
}

static void show_fpu(void)
{
	if (fpu_present)
		printk("FPU present\n");
	else
		printk("FPU not present\n");
}

void mach_boot(void)
{
	if (kparam.boot_flags & BOOT_VERBOSE)
	{
		printk("conv_mem_size = %i\n", kparam.conv_mem_size);
		show_mem_map();
		printk("\n");
		show_fpu();
	}
	else
	{
		show_mem_size();
		show_fpu();
	}
	printk("\n");
}
