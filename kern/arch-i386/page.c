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

#include <sysload/kparam.h>

#include <kern/console.h>
#include <kern/config.h>
#include <kern/signal.h>
#include <kern/printk.h>
#include <kern/config.h>
#include <kern/start.h>
#include <kern/errno.h>
#include <kern/page.h>
#include <kern/intr.h>
#include <kern/task.h>
#include <kern/umem.h>
#include <kern/lib.h>

#include <sys/signal.h>

vpage *pg_dir = (void *)(PAGE_DIR << 12);
vpage *pg_tab = (void *)(PAGE_TAB << 12);

unsigned *pg_flist;
ppage pg_flist_base = -1U;
ppage pg_flist_end;
ppage pg_vfree = PAGE_DYN_COUNT;
ppage pg_nfree = 0;
ppage pg_nfree_dma = 0;
ppage pg_total = 0;

char pg_dma_map[4096];

void pg_enable(void);
void pg_lcr3(ppage v);
void pg_utlb(void);

static void pg_count(void)
{
	struct kmapent *me;
	int i;
	
	me = kparam.mem_map;
	pg_total = 0;
	for (i = 0; i < kparam.mem_cnt; i++, me++)
		if (me->type == KMAPENT_MEM /* && me->base >= 0x100000 */)
		{
			uint32_t base = me->base;
			uint32_t size = me->size;
			uint32_t end  = base + size;
			
			if (base != me->base)
				continue;
			if (size != me->size)
				continue;
			if (base > base + end)
				continue;
			
			if (base & PAGE_PMASK)
				base += PAGE_SIZE;
			base >>= PAGE_SHIFT;
			end  >>= PAGE_SHIFT;
			
#if KVERBOSE
			printk("pg_count: free pages: %i ... %i\n", base, end + 1);
#endif
			pg_total += end - base;
			
			if (me->base >= 0x100000 && pg_flist_base > base)
			{
				pg_flist_base = base;
				pg_flist_end  = end;
			}
		}
#if KVERBOSE
	printk("pg_count: %i total pages\n", pg_total);
#endif
}

void pg_collect(void)
{
	struct kmapent *me;
	uint32_t n;
	int i;
	
	me = kparam.mem_map;
	for (i = 0; i < kparam.mem_cnt; i++, me++)
		if (me->type == KMAPENT_MEM && me->base >= 0x100000)
		{
			uint32_t base = me->base;
			uint32_t size = me->size;
			uint32_t end  = base + size;
			
			if (base != me->base)
				continue;
			if (size != me->size)
				continue;
			if (base > base + end)
				continue;
			
			if (base >= pg_flist_base && base < pg_flist_end)
				base = pg_flist_end;
			if (end >= pg_flist_base && end < pg_flist_end)
				end = pg_flist_base;
			if (end <= base)
				continue;
			
			if (base & PAGE_PMASK)
				base += PAGE_SIZE;
			base >>= PAGE_SHIFT;
			end  >>= PAGE_SHIFT;
			/* if (base < first_page)
				base = first_page; */
			
#if KVERBOSE
			printk("pg_collect: free pages: %i ... %i\n", base, end + 1);
#endif
			
			for (n = base; n < end && (pg_nfree + pg_nfree_dma) < pg_total; n++)
				pg_free(n);
		}
#if KVERBOSE
	printk("pg_collect: %i free pages\n", pg_nfree_dma + pg_nfree);
#endif
}

static void pg_kprot(void *start, void *end)
{
	unsigned *pg_itab = (void *)(PAGE_ITAB << 12);
	unsigned s, e, i;
	
	s = (unsigned)(start + 4095)	>> 12;
	e = (unsigned) end		>> 12;
	
	for (i = s; i < e; i++)
		pg_itab[i] = ((PAGE_IDENT + i) << 12) | PGF_KERN_RO;
}

void pg_ena0(void)
{
	unsigned *pg_itab = (void *)(PAGE_ITAB << 12);
	
	pg_itab[0] = PGF_KERN;
}

void pg_dis0(void)
{
	unsigned *pg_itab = (void *)(PAGE_ITAB << 12);
	
	pg_itab[0] = 0;
}

void pg_init(void)
{
	extern void _data;
	extern void _text;
	
	unsigned *pg_itab = (void *)(PAGE_ITAB << 12);
	unsigned fsz;
	unsigned i;
	
	for (i = 0; i < 1024; i++)
		pg_dir[i] = 0;
	
	for (i = 1; i < PAGE_IDENT_COUNT; i++)
		pg_itab[i] = ((PAGE_IDENT + i) << 12) | PGF_KERN;
	
	for (i = (PAGE_IDENT >> 10); i < (PAGE_IDENT_END >> 10); i++)
		pg_dir[i] = ((PAGE_ITAB + i) << 12) | PGF_KERN;
	
	pg_kprot(&_text, &_data);
	pg_itab[0] = 0;
	
	pg_dir[PAGE_TAB >> 10] = (PAGE_DIR << 12) | PGF_KERN;
	
	pg_enable();
	
	pg_count();
	fsz = (pg_total + 1023) / 1024;
	if (fsz > pg_flist_end - pg_flist_base)
		panic("pg_init: too many memory pages"); /* XXX */
	pg_flist_end = pg_flist_base + fsz;
	pg_flist = (void *)(pg_flist_base << 12);
#if KVERBOSE
	printk("pg_flist_base = %i\n", pg_flist_base);
	printk("pg_flist_end  = %i\n", pg_flist_end);
	printk("pg_total      = %i\n", pg_total);
	printk("pg_flist      = %p\n", pg_flist);
#endif
	pg_collect();
}

int pg_alloc(ppage *p)
{
	int err;
	
	if (!pg_flist)
		panic("pg_alloc: pg_flist not initialized");
	
	if (!pg_nfree)
	{
		err = pg_alloc_dma(p, 1);
		if (err)
			printk("pg_alloc: memory exhausted\n");
		return err;
	}
	
	*p = pg_flist[--pg_nfree];
	if (!*p)
	{
		printk("pg_alloc: pg_nfree = %u", pg_nfree);
		panic("pg_alloc: null frame in list");
	}
	return 0;
}

void pg_free(ppage p)
{
	if (!p)
		panic("pg_free: !p");
	if (!pg_flist)
		panic("pg_free: pg_flist not initialized");
	
	if (p < 4096)
	{
		pg_free_dma(p, 1);
		return;
	}
	
	pg_flist[pg_nfree++] = p;
}

int pg_alloc_dma(ppage *p, ppage s)
{
	unsigned start = 0;
	unsigned i;
	
	for (i = 0; i < 4096; i++)
	{
		if (i - start >= s)
		{
			while (--i != start)
				pg_dma_map[i] = 0;
			pg_dma_map[i] = 0;
			*p = start;
			pg_nfree_dma -= s;
			return 0;
		}
		if (i % 15 == 0)
			start = i;
		if (!pg_dma_map[i])
			start = i + 1;
	}
	
	return ENOMEM;
}

void pg_free_dma(ppage p, ppage s)
{
	unsigned i;
	
	if (p >= 4096 || p + s > 4096)
		panic("pg_free_dma: page not in the DMA area");
	
	for (i = 0; i < s; i++)
	{
		if (pg_dma_map[p + i])
		{
			printk("pg_free_dma: page %i is already free\n", p + i);
			panic("pg_free_dma: page is already free");
		}
		pg_dma_map[p + i] = 1;
	}
	pg_nfree_dma += s;
}

static void pg_wradir(unsigned index, unsigned value)
{
	struct task **e = task + TASK_MAX;
	struct task **p = task;
	
	while (p < e)
	{
		if (*p)
			(*p)->pg_dir[index] = value;
		p++;
	}
	pg_dir[index] = value;
}

int pg_adget(vpage *p, vpage s)
{
	unsigned start = PAGE_DYN;
	unsigned i = PAGE_DYN;
	unsigned dir;
	int err;
	
#if PG_ADGET_DEBUG
	printk("pg_adget(%p, %i);\n", p, (int)s);
#endif
	
	while (i - start < s)
	{
		if (i >= PAGE_DYN + PAGE_DYN_COUNT)
		{
			printk("pg_adget: kernel address space is full\n");
			return ENOMEM;
		}
		if (!pg_dir[i >> 10])
		{
			i += 4096;
			continue;
		}
		if (pg_tab[i])
			start = i + 1;
		i++;
	}
	
	for (i = start >> 10; i < (start + s + 1023) >> 10; i++)
	{
		if (!pg_dir[i])
		{
			err = pg_alloc(&dir);
			if (err)
			{
#if PG_ADGET_DEBUG
				perror("pg_adget: pg_alloc", err);
#endif
				return err;
			}
			pg_wradir(i, (dir << 12) | PGF_KERN);
			pg_utlb();
			memset(&pg_tab[i << 10], 0, PAGE_SIZE);
		}
	}
	
	for (i = start; i < start + s; i++)
		pg_tab[i] = PGF_SPACE_IN_USE;
	pg_vfree -= s;
	pg_utlb();
	
#if PG_ADGET_DEBUG
	printk("pg_adget: start = %i\n", (int)start);
#endif
	*p = start;
	return 0;
}

void pg_adput(vpage p, vpage s)
{
	unsigned i;
	
	for (i = p; i < p + s; i++)
		pg_tab[i] = 0;
	pg_vfree += s;
	pg_utlb();
}

int pg_atmem(vpage p, vpage s, int reuse)
{
	unsigned i;
	unsigned m;
	int err;
	
	if (pg_nfree + pg_nfree_dma < s)
		return ENOMEM;
	
	for (i = p; i < p + s; i++)
	{
		if (!pg_tab[i])
		{
			printk("pg_tab[%i] = 0x%x\n", i, pg_tab[i]);
			panic("pg_atmem: call pg_adget first");
		}
		
		if (pg_tab[i] != PGF_SPACE_IN_USE)
		{
			if (reuse)
				continue;
			
			printk("pg_tab[%i] = 0x%x\n", i, pg_tab[i]);
			panic("pg_atmem: address space already mapped");
		}
		
		err = pg_alloc(&m);
		if (err)
		{
			printk("pg_nfree = %u\n", pg_nfree);
			panic("pg_atmem: pg_alloc() failed");
		}
		pg_tab[i] = (m << 12) | PGF_KERN;
	}
	
	pg_utlb();
	return 0;
}

void pg_dtmem(vpage p, vpage s)
{
	unsigned i;
	
	for (i = p; i < p + s; i++)
	{
		if (pg_tab[i] & PGF_PRESENT)
		{
			pg_free(pg_tab[i] >> 12);
			pg_tab[i] = PGF_SPACE_IN_USE;
		}
		else
		{
			printk("pg_dtmem: pg_tab[%i] = 0x%x\n", i, pg_tab[i]);
			panic("pg_dtmem: page not present");
		}
	}
	
	pg_utlb();
}

int pg_atdma(vpage p, vpage s)
{
	unsigned m;
	unsigned i;
	int err;
	
	err = pg_alloc_dma(&m, s);
	if (err)
		return err;
	
	for (i = 0; i < s; i++)
		pg_tab[i + p] = (m++ << 12) | PGF_KERN;
	
	pg_utlb();
	return 0;
}

void pg_dtdma(vpage p, vpage s)
{
	unsigned i;
	
	pg_free_dma(pg_tab[p] >> 12, s);
	for (i = 0; i < s; i++)
		pg_tab[i + p] = PGF_SPACE_IN_USE;
	
	pg_utlb();
}

int pg_atphys(vpage p, vpage s, ppage phys, int cache)
{
	unsigned i;
	
	for (i = 0; i < s; i++)
	{
		if (!pg_tab[i + p])
			panic("pg_atphys: call pg_adget first");
		
		if (pg_tab[i + p] != PGF_SPACE_IN_USE)
			panic("pg_atphys: address space already mapped");
		
		pg_tab[i + p] = ((phys + i) << 12) | PGF_KERN |	PGF_PHYS;
		switch (cache)
		{
		case 0:
		case WRITE_CACHE:
			pg_tab[i + p] |= PGF_PCD;
			break;
		case READ_CACHE | WRITE_CACHE:
			break;
		case READ_CACHE:
			pg_tab[i + p] |= PGF_PWT;
			break;
		default:
			panic("pg_atphys: bad cache flags");
		}
	}
	
	pg_utlb();
	return 0;
}

void pg_dtphys(vpage p, vpage s)
{
	unsigned i;
	
	for (i = p; i < p + s; i++)
	{
		if (!(pg_tab[i] & PGF_PRESENT))
			panic("pg_dtmem: page not present");
		
		pg_tab[i] = PGF_SPACE_IN_USE;
	}
	
	pg_utlb();
}

int pg_uauto(vpage p)
{
	unsigned m;
	int err;
	
	if (!(pg_dir[p >> 10] & PGF_USER))
		return EFAULT;
	
	if (pg_tab[p] & PGF_COW)
	{
		err = pg_alloc(&m);
		if (err)
		{
#if PG_FAULT_DEBUG
			perror("pg_uauto: %i: COW allocation failed", err);
#endif
			return err;
		}
		
		pg_tab[PAGE_TMP] = (m << 12) | PGF_PRESENT | PGF_WRITABLE;
		pg_utlb();
		memcpy(pg2vap(PAGE_TMP), pg2vap(p), PAGE_SIZE);
		pg_tab[PAGE_TMP] = 0;
		pg_tab[p] = (m << 12) | PGF_PRESENT | PGF_USER | PGF_WRITABLE;
		pg_utlb();
		
		curr->pg_count++;
		return 0;
	}
	
	if (pg_tab[p] & PGF_PRESENT)
		return 0;
	
	if (pg_tab[p] & PGF_AUTO)
	{
		err = pg_alloc(&m);
		if (err)
		{
#if PG_FAULT_DEBUG
			perror("pg_uauto: lazy allocation failed", err);
#endif
			return err;
		}
		pg_tab[p] = (m << 12) | PGF_PRESENT | PGF_WRITABLE | PGF_USER;
		pg_utlb();
		memset((void *)(p << 12), 0, PAGE_SIZE);
		curr->pg_count++;
		return 0;
	}
#if PG_FAULT_DEBUG
	printk("pg_uauto: bad vpage 0x%x\n", p);
#endif
	
	return EFAULT;
}

unsigned read_cr2();
unsigned read_cr3();

int pg_fault(int umode)
{
	unsigned p = read_cr2() / PAGE_SIZE;
#if PG_FAULT_DEBUG
	unsigned cr3 = read_cr3();
	unsigned pde, pte = 0;
#endif
	int err;
	
#if PG_FAULT_DEBUG
	printk("pg_fault: umode = %i, p = 0x%x, p >> 10 = %p, cr3 = 0x%x\n",
		umode, p, p >> 10, cr3);
	pde = pg_dir[p >> 10];
	printk("          pde = 0x%x\n", pde);
	if (pde & 1)
		pte = pg_tab[p];
	printk("          pte = 0x%x\n", pte);
#endif
	
	if (p < PAGE_USER || p >= PAGE_USER_END)
	{
		if (umode)
		{
			signal_raise_fatal(SIGSEGV);
			return 0;
		}
		
		printk("pg_fault: kernel mode fault, cr2 = %p\n", read_cr2());
		return 1;
	}
	
	err = pg_uauto(p);
	if (err)
	{
#if PG_FAULT_DEBUG
		perror("pg_uauto", err);
#endif
		if (umode)
		{
			switch (err)
			{
			case ENOMEM:
				signal_raise_fatal(SIGOOM);
				break;
			case EFAULT:
				signal_raise_fatal(SIGSEGV);
				break;
			default:
				signal_raise_fatal(SIGBUS);
			}
		}
		else
			fault_abort();
	}
	
	return 0;
}

void *pg_getphys(void *p)
{
	return (void *)(pg_tab[(unsigned)p >> 12] & PAGE_VMASK);
}

int pg_newdir(vpage *buf)
{
	memcpy(buf, pg_dir, 4096);
	memset(buf + (PAGE_USER >> 10), 0, PAGE_USER_COUNT >> 8);
	
	buf[PAGE_TAB >> 10] = (unsigned)pg_getphys(buf) | PGF_KERN;
	return 0;
}

void pg_setdir(vpage *dir)
{
	pg_lcr3((unsigned)pg_getphys(dir));
	pg_dir = dir;
}

vpage *pg_getdir(void)
{
	return pg_dir;
}

int pg_altab(vpage pti, int flags)
{
	ppage pg;
	int err;
	
	pti >>= 10;
	
	if (!pg_dir[pti])
	{
		if (!flags)
			return EFAULT;
		
		err = pg_alloc(&pg);
		if (err)
			return err;
		
		pg_dir[pti] = (pg << 12) | flags;
		pg_utlb();
		
		memset(&pg_tab[pti << 10], 0, 4096);
		pg_utlb();
	}
	return 0;
}
