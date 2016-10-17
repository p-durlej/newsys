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

#include <kern/printk.h>
#include <kern/signal.h>
#include <kern/config.h>
#include <kern/errno.h>
#include <kern/umem.h>
#include <kern/page.h>
#include <kern/task.h>
#include <kern/lib.h>
#include <sys/stat.h>
#include <os386.h>

void dump_core(int status)
{
	struct core_page_head phd;
	struct core_head hd;
	struct fs_rwreq rw;
	struct fso *fso;
	unsigned i;
	uoff_t off;
	
	if (fs_creat(&fso, "core", S_IFREG | S_IRUSR | S_IWUSR, 0))
		return;
	memset(&phd, 0, sizeof phd);
	memset(&hd, 0, sizeof phd);
	
	strcpy(hd.pathname, curr->exec_name);
	if (ureg != NULL)
		memcpy(&hd.regs, ureg, sizeof hd.regs);
	else
		memset(&hd.regs, 0xff, sizeof hd.regs);
	
	hd.status = status;
	rw.fso = fso;
	rw.no_delay = 0;
	rw.offset = 0;
	rw.count = sizeof hd;
	rw.buf = &hd;
	if (fso->fs->type->write(&rw))
		goto fail;
	
	off = sizeof hd;
	for (i = PAGE_USER; i < PAGE_USER_END; i++)
	{
		if (!pg_dir[i >> 10])
		{
			i &= 0xfffffc00;
			i += 1023;
			continue;
		}
		
		if (!(pg_tab[i] & PGF_PRESENT))
			continue;
		
		phd.addr = i << 12;
		phd.size = PAGE_SIZE;
		rw.buf = &phd;
		rw.offset = off;
		rw.count = sizeof phd;
		if (fso->fs->type->write(&rw))
			goto fail;
		off += sizeof phd;
		
		rw.buf = (void *)(i << 12);
		rw.offset = off;
		rw.count = PAGE_SIZE;
		if (fso->fs->type->write(&rw))
			goto fail;
		off += PAGE_SIZE;
	}
	
fail:
	fs_putfso(fso);
}

void uclean(void)
{
	unsigned i;
	
	for (i = PAGE_USER >> 10; i < PAGE_USER_END >> 10; i++)
		if ((pg_dir[i] & PGF_PRESENT) && !(pg_dir[i] >> 12))
			panic("uclean: null frame in directory\n");
	
	for (i = PAGE_USER; i < PAGE_USER_END; i++)
	{
		if (!pg_dir[i >> 10])
		{
			i &= 0xfffffc00;
			i += 1023;
			continue;
		}
		if (pg_tab[i] & PGF_COW)
			continue;
		
		if (pg_tab[i] & PGF_PRESENT)
		{
			if (!(pg_tab[i] >> 12))
				panic("uclean: null frame in table\n");
			pg_free(pg_tab[i] >> 12);
		}
	}
	
	for (i = PAGE_USER >> 10; i < PAGE_USER_END >> 10; i++)
		if (pg_dir[i])
		{
			if (!(pg_dir[i] >> 12))
				panic("uclean: null frame in directory\n");
			pg_free(pg_dir[i] >> 12);
			pg_dir[i] = 0;
		}
	
	pg_utlb();
	curr->pg_count = 0;
}

int u_alloc(unsigned start, unsigned end)
{
	unsigned pg;
	unsigned i;
	int err;
	
	if (start >= end)
		goto fault;
	
	if (start < PAGE_USER || start >= PAGE_USER_END)
		goto fault;
	
	if (end <= PAGE_USER || end > PAGE_USER_END)
		goto fault;
	
	for (i = start >> 10; i <= ((end - 1) >> 10); i++)
		if (!pg_dir[i])
		{
			err = pg_alloc(&pg);
			if (err)
				return err;
			
			pg_dir[i] = (pg << 12) | PGF_PRESENT | PGF_WRITABLE | PGF_USER;
			pg_utlb();
			
			memset(&pg_tab[i << 10], 0, 4096);
			pg_utlb();
			curr->pg_count++;
		}
	
	for (i = start; i < end; i++)
		if (!pg_tab[i])
			pg_tab[i] = PGF_AUTO;
	
	return 0;
fault:
#if SIGSEGV_EFAULT
	signal_raise(SIGSEGV);
#endif
	return EFAULT;
}

int u_free(unsigned start, unsigned end)
{
	unsigned i;

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
		
		if (pg_dir[i >> 10] && (pg_tab[i] & PGF_PRESENT))
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

static int range_chk(const void *p, unsigned size)
{
	unsigned page0 =  (unsigned)p >> 12;
	unsigned page1 = ((unsigned)p + size - 1) >> 12;
	
	if (page0 < PAGE_USER || page0 >= PAGE_USER_END)
		goto fault;
	if (page1 < PAGE_USER || page1 >= PAGE_USER_END)
		goto fault;
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
	
	err = pg_uauto((unsigned)s >> 12);
	if (err)
		return err;
	p1 = s;
	
	do
	{
		if (p1 - s > max)
			return le;
		
		if (!((unsigned)p1 & PAGE_PMASK))
		{
			err = pg_uauto((unsigned)s >> 12); // XXX bug
			if (err)
				return err;
		}
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
	
	pg   =  (unsigned)p		>> 12;
	last = ((unsigned)p + size - 1) >> 12;
	
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
	int cnt=0;
	int err;
	
	if (fault())
		goto fault;
	do
	{
		if (cnt >= max)
		{
			fault_fini();
			return ERANGE;
		}
		err = range_chk(s, 1);
		if (err)
		{
			fault_fini();
			return err;
		}
		*(d++) = *(s++);
		cnt++;
	} while (s[-1]);
	
	fault_fini();
	return 0;
fault:
#if SIGSEGV_EFAULT
	signal_raise(SIGSEGV);
#endif
	return EFAULT;
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
	
	ureg->esp -= sizeof v;
	err = tucpy((void *)ureg->esp, &v, sizeof v);
	if (err)
		signal_raise(SIGSTK);
	
	return err;
}
