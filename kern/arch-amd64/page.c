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
#include <kern/config.h>
#include <kern/printk.h>
#include <kern/signal.h>
#include <kern/start.h>
#include <kern/errno.h>
#include <kern/page.h>
#include <kern/task.h>
#include <kern/umem.h>
#include <kern/lib.h>

vpage *pg_cpml4;

vpage *pg_tabs[4] = { pg2vap(PAGE_TAB), pg2vap(PAGE_DIR), pg2vap(PAGE_PDP), pg2vap(PAGE_PML4) };
vpage *pg_tab	  =   pg2vap(PAGE_TAB);

ppage pg_vfree	   = PAGE_DYN_COUNT;
ppage pg_nfree_dma = 0;
ppage pg_nfree	   = 0;
ppage pg_total	   = 0;

char pg_dma_map[4096];

ppage  pg_flist_base = -1U;
ppage  pg_flist_end;
ppage *pg_flist;

void	pg_lcr3(ppage cr3);
vpage	read_cr2(void);
ppage	read_cr3(void);
void	pg_utlb(void);

static void pg_count(void)
{
	struct kmapent *me;
	int i;
	
	me = kparam.mem_map;
	pg_total = 0;
	for (i = 0; i < kparam.mem_cnt; i++, me++)
		if (me->type == KMAPENT_MEM /* && me->base >= 0x100000 */)
		{
			uint64_t base = me->base;
			uint64_t size = me->size;
			uint64_t end  = base + size;
			
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
			uint64_t base = me->base;
			uint64_t size = me->size;
			uint64_t end  = base + size;
			
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

static void pg_finit(void)
{
	vpage fsz;
	
	fsz = (pg_total + 1023) / 1024;
	if (fsz > pg_flist_end - pg_flist_base)
		panic("pg_init: too many memory pages"); /* XXX */
	
	pg_flist_end = pg_flist_base + fsz;
	pg_flist     = pg2vap(pg_flist_base);
#if KVERBOSE
	printk("pg_flist_base = %i\n", pg_flist_base);
	printk("pg_flist_end  = %i\n", pg_flist_end);
	printk("pg_total      = %i\n", pg_total);
	printk("pg_flist      = %p\n", pg_flist);
#endif
}

void pg_init(void)
{
	static vpage *pml4 = pg2vap(PAGE_IPML4),
		     *ipdp = pg2vap(PAGE_IPDP),
		     *idir = pg2vap(PAGE_IDIR),
		     *itab = pg2vap(PAGE_ITAB),
		     *tab0 = pg2vap(PAGE_TAB0);
	
	int i;
	
	memset(pml4, 0, 4096);
	memset(ipdp, 0, 4096);
	memset(idir, 0, 4096);
	memset(itab, 0, 4096);
	
	idir[PAGE_TMP >> 9] = (PAGE_ITAB << 12) | PGF_KERN;
	
	for (i = PAGE_IDENT; i < PAGE_IDENT + PAGE_IDENT_COUNT; i+= 512)
		idir[i >> 9] = (i << 12) | PGF_KERN | PGF_PS;
	
	for (i = 1; i < 512; i++)
		tab0[i] = (i << 12) | PGF_KERN;
	tab0[0] = 0;
	
	pml4[0] = (PAGE_IPDP << 12) | PGF_HIGHLEVEL;
	ipdp[0] = (PAGE_IDIR << 12) | PGF_KERN;
	idir[0] = (PAGE_TAB0 << 12) | PGF_KERN;
	
	pg_lcr3(PAGE_IPML4 << 12);
	
	pg_count();
	pg_finit();
	pg_collect();
	
	pg_newdir(pml4);
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
	ppage start = 0;
	ppage i;
	
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

static int pg_altab1(vpage *tab, vpage *subtab, vpage i, int shift, int flags)
{
	ppage pg;
	int err;
	
	i >>= shift;
	
	if (!tab[i])
	{
		if (!flags)
			return EFAULT;
		
		err = pg_alloc(&pg);
		if (err)
			return err;
		
		tab[i] = (pg << 12) | flags;
		pg_utlb();
		
		memset(&subtab[i << 9], 0, 4096);
		pg_utlb();
	}
	return 0;
}

int pg_altab(vpage pti, int flags)
{
	static vpage *const pml4 = pg2vap(PAGE_PML4);
	static vpage *const pdp  = pg2vap(PAGE_PDP);
	static vpage *const dir  = pg2vap(PAGE_DIR);
	static vpage *const tab  = pg2vap(PAGE_TAB);
	
	int err;
	
	err = pg_altab1(pml4, pdp, pti, 27, PGF_HIGHLEVEL);
	if (err)
		return err;
	
	err = pg_altab1(pdp, dir, pti, 18, PGF_HIGHLEVEL);
	if (err)
		return err;
	
	return pg_altab1(dir, tab, pti, 9, flags);
}

int pg_adget(vpage *p, vpage size)
{
	static vpage *const pml4 = pg2vap(PAGE_PML4);
	static vpage *const pdp  = pg2vap(PAGE_PDP);
	static vpage *const dir  = pg2vap(PAGE_DIR);
	static vpage *const tab  = pg2vap(PAGE_TAB);
	
	vpage start, end;
	vpage i;
	int err;
	
	for (start = PAGE_DYN, end = start; end < PAGE_DYN + PAGE_DYN_COUNT; end++)
	{
		if (end - start >= size)
		{
			end = start + size;
			*p  = start;
			for (i = start; i < end; i++)
			{
				err = pg_altab(i, PGF_KERN);
				if (err)
					return err;
				
				tab[i] = PGF_SPACE_IN_USE;
			}
			return 0;
		}
		
		if (!pml4[end >> 27])
		{
			end += 1L << 27;
			continue;
		}
		if (!pdp[end >> 18])
		{
			end += 1L << 18;
			continue;
		}
		if (!dir[end >> 9])
		{
			end += 1L << 9;
			continue;
		}
		if (tab[end])
		{
			start = end + 1;
			continue;
		}
	}
	return ENOMEM;
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
	vpage i;
	ppage m;
	int err;
	
	if (pg_nfree + pg_nfree_dma < s)
		return ENOMEM;
	
	for (i = p; i < p + s; i++)
	{
		err = pg_altab(i, PGF_KERN);
		if (err)
			panic("pg_atmem: pg_altab failed");
		
		if (!pg_tab[i])
		{
			printk("pg_tab[%i] = 0x%lx\n", i, (unsigned long)pg_tab[i]);
			panic("pg_atmem: call pg_adget first");
		}
		
		if (pg_tab[i] != PGF_SPACE_IN_USE)
		{
			if (reuse)
				continue;
			
			printk("pg_tab[%i] = 0x%lx\n", i, (unsigned long)pg_tab[i]);
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
	vpage i;
	
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
	ppage m;
	vpage i;
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
	vpage i;
	
	pg_free_dma(pg_tab[p] >> 12, s);
	for (i = 0; i < s; i++)
		pg_tab[i + p] = PGF_SPACE_IN_USE;
	
	pg_utlb();
}

int pg_atphys(vpage p, vpage s, ppage phys, int cache)
{
	vpage i;
	
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
	vpage i;
	
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
	ppage m;
	int err;
	
	if (p < PAGE_USER || p >= PAGE_USER_END)
		return EFAULT;
	
	err = pg_altab(p, 0);
	if (err)
		return err;
	
	if (pg_tab[p] & PGF_COW)
		panic("pg_uauto: PGF_COW is not supported on amd64");
	
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
		memset(pg2vap(p), 0, PAGE_SIZE);
		curr->pg_count++;
		return 0;
	}
	
	return EFAULT;
}

int pg_fault(int umode)
{
	unsigned p = read_cr2() / PAGE_SIZE;
#if PG_FAULT_DEBUG
	unsigned cr3 = read_cr3();
#endif
	int err;
	
#if PG_FAULT_DEBUG
	printk("pg_fault: umode = %i, p = 0x%x, p >> 10 = %p, cr3 = 0x%x\n",
		umode, p, p >> 10, cr3);
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
	vpage pg = vap2pg(p);
	
	if (pg < PAGE_IDENT_END)
		return p;
	
	return (void *)(pg_tab[pg] & PAGE_VMASK);
}

int pg_newdir(vpage *npml4_vir)
{
	static vpage *ttab  = pg2vap(PAGE_ITAB);
	
	ppage  npml4_phys = (uintptr_t)pg_getphys(npml4_vir);
	ppage  npdp_phys;
	vpage *npdp_vir;
	
	int err;
	
	err = pg_alloc(&npdp_phys);
	if (err)
		return err;
	npdp_phys <<= 12;
	
	ttab[0]  = npdp_phys | PGF_KERN;
	npdp_vir = pg2vap(PAGE_TMP);
	pg_utlb();
	
	memset(npml4_vir + 2, 0, 4080);
	memset(npdp_vir + 1, 0, 4088);
	
	npdp_vir[0] = (PAGE_IDIR << 12) | PGF_KERN;
	
	npml4_vir[0] = npdp_phys  | PGF_HIGHLEVEL;
	npml4_vir[1] = npml4_phys | PGF_KERN;
	
	ttab[0] = 0;
	pg_utlb();
	
	return 0;
}

void pg_setdir(vpage *dir)
{
	pg_lcr3((ppage)pg_getphys(dir));
	pg_cpml4 = dir;
}

vpage *pg_getdir(void)
{
	return pg2vap(PAGE_PML4);
}
