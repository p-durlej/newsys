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

#include <kern/signal.h>
#include <kern/config.h>
#include <kern/errno.h>
#include <kern/umem.h>
#include <kern/page.h>
#include <kern/task.h>
#include <kern/lib.h>

#include <kern/console.h>
#include <kern/printk.h>
#include <kern/stat.h>

static void skip(vpage *ip, int shift)
{
	*ip &= -1LU << shift;
	*ip |= (1LU << shift) - 1;
}

void dump_core(int status)
{
	static vpage *const pml4 = pg2vap(PAGE_PML4);
	static vpage *const pdp  = pg2vap(PAGE_PDP);
	static vpage *const dir  = pg2vap(PAGE_DIR);
	static vpage *const tab  = pg2vap(PAGE_TAB);
	
	struct core_page_head phd;
	struct core_head hd;
	struct fs_rwreq rw;
	struct fso *fso;
	uoff_t off;
	vpage i;
	
	if (fs_creat(&fso, "core", S_IFREG | S_IRUSR | S_IWUSR, 0))
		return;
	memset(&phd, 0, sizeof phd);
	memset(&hd, 0, sizeof phd);
	
	strcpy(hd.pathname, curr->exec_name);
	if (ureg != NULL)
		memcpy(&hd.regs, ureg, sizeof hd.regs);
	else
		memset(&hd.regs, 0xff, sizeof hd.regs);
	
	hd.status   = status;
	rw.fso	    = fso;
	rw.no_delay = 0;
	rw.offset   = 0;
	rw.count    = sizeof hd;
	rw.buf	    = &hd;
	if (fso->fs->type->write(&rw))
		goto fail;
	
	off = sizeof hd;
	for (i = PAGE_USER; i < PAGE_USER_END; i++)
	{
		if (!pml4[i >> 27])
		{
			skip(&i, 27);
			continue;
		}
		
		if (!pdp[i >> 18])
		{
			skip(&i, 18);
			continue;
		}
		
		if (!dir[i >> 9])
		{
			skip(&i, 9);
			continue;
		}
		
		if (tab[i] & PGF_PRESENT)
		{
			phd.addr  = (uintptr_t)pg2vap(i);
			phd.size  = PAGE_SIZE;
			rw.buf	  = &phd;
			rw.offset = off;
			rw.count  = sizeof phd;
			if (fso->fs->type->write(&rw))
				goto fail;
			off += sizeof phd;
			
			rw.buf	  = (void *)(i << 12);
			rw.offset = off;
			rw.count  = PAGE_SIZE;
			if (fso->fs->type->write(&rw))
				goto fail;
			off += PAGE_SIZE;
		}
	}
	
fail:
	fs_putfso(fso);

}

void uclean(void)
{
	static vpage *const pml4 = pg2vap(PAGE_PML4);
	static vpage *const pdp  = pg2vap(PAGE_PDP);
	static vpage *const dir  = pg2vap(PAGE_DIR);
	
	vpage i;
	
	for (i = PAGE_USER; i < PAGE_USER_END; i++)
	{
		if (!pml4[i >> 27])
		{
			skip(&i, 27);
			continue;
		}
		
		if (!pdp[i >> 18])
		{
			skip(&i, 18);
			continue;
		}
		
		if (!dir[i >> 9])
		{
			skip(&i, 9);
			continue;
		}
		
		if (pg_tab[i] & PGF_COW)
			continue;
		
		if (pg_tab[i] & PGF_PRESENT)
		{
			pg_free(pg_tab[i] >> 12);
			pg_tab[i] = 0;
		}
	}
	
	for (i = (PAGE_USER + 511) & ~511L; i < PAGE_USER_END; i += (1L << 9))
	{
		if (!pml4[i >> 27])
		{
			skip(&i, 27);
			continue;
		}
		
		if (!pdp[i >> 18])
		{
			skip(&i, 18);
			continue;
		}
		
		if (!dir[i >> 9])
			continue;
		
		pg_free(dir[i >> 9] >> 12);
		dir[i >> 9] = 0;
	}
	
	for (i = (PAGE_USER + 262143) & ~262143L; i < PAGE_USER_END; i += (1L << 18))
	{
		if (!pml4[i >> 27])
		{
			skip(&i, 27);
			continue;
		}
		
		if (!pdp[i >> 18])
			continue;
		
		pg_free(pdp[i >> 18] >> 12);
		pdp[i >> 18] = 0;
	}
	
#if 0
	for (i = PAGE_USER; i < PAGE_USER_END; i += (1L << 27))
	{
		if (!pml4[i >> 27])
			continue;
		
		pg_free(pml4[i >> 27] >> 12);
		pml4[i >> 27] = 0;
	}
#endif
	
	pg_utlb();
	curr->pg_count = 0;
}

int u_alloc(vpage start, vpage end)
{
	vpage i;
	int err;
	
	if (start >= end)
		goto fault;
	
	if (start < PAGE_USER || start >= PAGE_USER_END)
		goto fault;
	
	if (end <= PAGE_USER || end > PAGE_USER_END)
		goto fault;
	
	for (i = start; i < end; i++)
	{
		err = pg_altab(i, PGF_USER | PGF_PRESENT | PGF_WRITABLE);
		if (err)
			return err;
		
		if (!pg_tab[i])
			pg_tab[i] = PGF_AUTO;
	}
	
	return 0;
fault:
#if SIGSEGV_EFAULT
	signal_raise(SIGSEGV);
#endif
	return EFAULT;
}

int u_free(vpage start, vpage end)
{
	static vpage *const pml4 = pg2vap(PAGE_PML4);
	static vpage *const pdp  = pg2vap(PAGE_PDP);
	static vpage *const dir  = pg2vap(PAGE_DIR);
	
	vpage i;
	
	if (start >= end)
		goto fault;
	
	if (start < PAGE_USER || start >= PAGE_USER_END)
		goto fault;
	
	if (end <= PAGE_USER || end > PAGE_USER_END)
		goto fault;
	
	for (i = start; i < end; i++)
	{
		if (!pg_tab[i])
			goto fault;
		
		if (!(pml4[i >> 27] & PGF_PRESENT))
			goto fault;
		
		if (!(pdp[i >> 18] & PGF_PRESENT))
			goto fault;
		
		if (!(dir[i >> 9] & PGF_PRESENT))
			goto fault;
		
		if (pg_tab[i] & PGF_PRESENT)
		{
			pg_free(pg_tab[i] >> 12);
			curr->pg_count--;
		}
		
		pg_tab[i] = 0;
	}
	pg_utlb();
	
	return 0;
fault:
#if SIGSEGV_EFAULT
	signal_raise(SIGSEGV);
#endif
	return EFAULT;
}

int uaa(void *p, unsigned sz, unsigned nm, int flags)
{
	size_t s = sz * nm;
	if (s < sz)
		return EINVAL;
	
	return uga(p, s, flags);
}

int usa(const char **p, unsigned max, int le)
{
	const char *s = *p, *p1;
	int err;
	
	if (!max)
		panic("usa: !max");
	
	err = pg_uauto(vap2pg(s));
	if (err)
		return err;
	p1 = s;
	
	do
	{
		if (p1 - s > max)
			return le;
		
		if ((uintptr_t)p1 & 0xfff)
			continue;
		
		err = pg_uauto(vap2pg(p1));
		if (err)
			return err;
	} while (*p1++);
	
	return 0;
}

int uga(void *pp, unsigned size, int flags)
{
	void *p = *(void **)pp;
	vpage last;
	vpage pg;
	vpage i;
	int err;
	
	if (!size)
		return 0;
	
	pg   =  (uintptr_t)p		 >> 12;
	last = ((uintptr_t)p + size - 1) >> 12;
	
	if (pg > last)
		goto fault;
	
	if (pg < PAGE_USER || pg >= PAGE_USER_END)
		goto fault;
	
	if (last < PAGE_USER || last >= PAGE_USER_END)
		goto fault;
	
	for (i = pg; i <= last; i++)
	{
		err = pg_uauto(i);
		if (err)
			return err;
	}
	
	return 0;
fault:
#if SIGSEGV_EFAULT
	signal_raise(SIGSEGV);
#endif
	return EFAULT;
}

int urchk(const void *p, unsigned size)
{
	return uga(&p, size, UA_READ);
}

int uwchk(const void *p, unsigned size)
{
	return uga(&p, size, UA_WRITE);
}

int tucpy(void *d, const void *s, unsigned size)
{
	int err;
	
	err = uga(&d, size, UA_WRITE);
	if (err)
		return err;
	
	memcpy(d, s, size);
	return 0;
}

int fucpy(void *d, const void *s, unsigned size)
{
	int err;
	
	err = uga(&s, size, UA_READ);
	if (err)
		return err;
	
	memcpy(d, s, size);
	return 0;
}

int fustr(char *d, const char *s, unsigned max)
{
	int err;
	
	err = usa(&s, max - 1, ENAMETOOLONG);
	if (err)
		return err;
	
	strcpy(d, s);
	return 0;
}

int uset(void *p, unsigned v, unsigned size)
{
	int err;
	
	err = uga(&p, size, UA_WRITE);
	if (err)
		return err;
	
	memset(p, v, size);
	return 0;
}

int upush(unsigned long v)
{
	int err;
	
	ureg->rsp -= sizeof v;
	
	err = tucpy((void *)ureg->rsp, &v, sizeof v);
	if (err)
		signal_raise(SIGSTK);
	
	return err;
}
