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

#include <kern/machine/machine.h>
#include <kern/machine/start.h>
#include <kern/arch/selector.h>
#include <kern/console.h>
#include <kern/signal.h>
#include <kern/printk.h>
#include <kern/module.h>
#include <kern/start.h>
#include <kern/errno.h>
#include <kern/start.h>
#include <kern/exec.h>
#include <priv/exec.h>
#include <kern/task.h>
#include <kern/stat.h>
#include <kern/intr.h>
#include <kern/umem.h>
#include <kern/cpu.h>
#include <kern/lib.h>
#include <kern/fs.h>

#include <sysload/kparam.h>

#include <arch/archdef.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <limits.h>
#include <fcntl.h>

#ifdef __ARCH_I386__
#define LDR_COW		1
#else
#define LDR_COW		0
#endif

#define LDR_PATH	"/lib/sys/user.bin"

#define ESTACK		((uintptr_t)pg2vap(PAGE_STACK_END))
#define UARGS		(ESTACK - ARG_MAX)
#define XNAME		(UARGS  - PATH_MAX)
#define USTACK		XNAME

void *	 ldr_image;
int	 ldr_size;
unsigned ldr_code;

#if LDR_COW
static vpage ldr_page;
static vpage ldr_pgc;

static void load_ldr(void)
{
	struct fs_rwreq rq;
	struct fso *f;
	int err;
	
	if (kparam.boot_flags & BOOT_VERBOSE)
		printk("loading " LDR_PATH " ... ");
	
	err = fs_lookup(&f, LDR_PATH);
	if (err)
	{
		perror(LDR_PATH, err);
		panic("unable to open loader");
	}
	
	ldr_pgc = (f->size + PAGE_SIZE - 1) >> PAGE_SHIFT;
	
	err = pg_adget(&ldr_page, ldr_pgc);
	if (err)
		perror("load_ldr: pg_adget", err);
	
	err = pg_atmem(ldr_page, ldr_pgc, 0);
	if (err)
		perror("load_ldr: pg_atmem", err);
	
	ldr_image = pg2vap(ldr_page);
	ldr_size  = f->size;
	memset(ldr_image, 0, pg2size(ldr_pgc));
	
	rq.buf	  = ldr_image;
	rq.offset = 0;
	rq.count  = f->size;
	rq.fso	  = f;
	
	err = f->fs->type->read(&rq);
	if (err)
	{
		perror(LDR_PATH, err);
		panic("unable to read loader");
	}
	
	fs_putfso(f);
	
	if (kparam.boot_flags & BOOT_VERBOSE)
		printk("done\n");
}
#else
static void load_ldr(void)
{
	struct fs_rwreq rq;
	struct fso *f;
	int err;
	
	if (kparam.boot_flags & BOOT_VERBOSE)
		printk("loading " LDR_PATH " ... ");
	
	err = fs_lookup(&f, LDR_PATH);
	if (err)
	{
		perror(LDR_PATH, err);
		panic("unable to open loader");
	}
	
	err = kmalloc(&ldr_image, f->size, "loader");
	if (err)
		panic("unable to allocate memory for loader");
	ldr_size  = f->size;
	
	rq.buf	  = ldr_image;
	rq.offset = 0;
	rq.count  = f->size;
	rq.fso	  = f;
	
	err = f->fs->type->read(&rq);
	if (err)
	{
		perror(LDR_PATH, err);
		panic("unable to read loader");
	}
	
	fs_putfso(f);
	
	ldr_code = ((struct modhead *)ldr_image)->code_start;
	
	if (kparam.boot_flags & BOOT_VERBOSE)
		printk("done\n");
}
#endif

static void chk_ldr(void)
{
	unsigned cksum = 0;
	unsigned size;
	unsigned *p;
	
	p      = ldr_image;
	size   = ldr_size;
	size  += 3;
	size >>= 2;
	while (size--)
		cksum += *p++;
	
	if (cksum)
		panic("invalid loader checksum");
}

int do_exec(void)
{
	struct fs_file *file;
	struct fso *fso;
	unsigned fd;
	int err;
	int i;
	
	/* printk("do_exec: curr->exec_name = \"%s\"\n", curr->exec_name); */
	curr->exec = 0;
	
	for (i = 0; i < NSIG; i++)
		if (curr->signal_handler[i] != SIG_IGN)
			curr->signal_handler[i] = SIG_DFL;
	
	for (i = 0; i < OPEN_MAX; i++)
		if (curr->file_desc[i].close_on_exec)
			fs_putfd(i);
	
	curr->alarm_repeat = 0;
	curr->alarm = 0;
	
	win_clean();
	uclean();
	
	curr->event_high  = 0;
	curr->first_event = 0;
	curr->event_count = 0;
	curr->last_event  = -1;
	
	if (!ldr_image)
	{
		load_ldr();
		chk_ldr();
	}
	
	err = u_alloc(PAGE_STACK, PAGE_STACK_END);
	if (err)
		return err;
	
	err = u_alloc(PAGE_LDR, PAGE_LDR_END);
	if (err)
		return err;
	
#if LDR_COW
	for (i = 0; i < ldr_pgc; i++)
	{
		vpage upg = vap2pg(LDR_BASE) + i;
		vpage pte;
		
		err = pg_altab(upg, PGF_PRESENT | PGF_USER | PGF_WRITABLE);
		if (err)
			return err;
		
		pte = pg_tab[ldr_page + i];
		pte &= PAGE_VMASK;
		pte |= PGF_PRESENT | PGF_USER | PGF_COW;
		
		pg_tab[upg] = pte;
	}
#else
	err = tucpy((void *)LDR_BASE, ldr_image, ldr_size);
	if (err)
		return err;
#endif
	csync((void *)LDR_BASE, ldr_size);
	
	err = tucpy((void *)UARGS, curr->exec_arg, curr->exec_arg_size);
	if (err)
		return err;
	
	err = tucpy((void *)XNAME, curr->exec_name, strlen(curr->exec_name) + 1);
	if (err)
		return err;
	
	curr->errno	   = NULL;
	curr->signal_entry = NULL;
	
	err = fs_lookup(&fso, curr->exec_name);
	if (err)
		return err;
	
	err = fs_chk_perm(fso, X_OK, curr->euid, curr->egid);
	if (err)
	{
		fs_putfso(fso);
		return err;
	}
	
	if (!S_ISREG(fso->mode))
	{
		fs_putfso(fso);
		return EINVAL;
	}
	
	err = fs_getfile(&file, fso, O_RDONLY);
	if (err)
	{
		fs_putfso(fso);
		return err;
	}
	
	err = fs_getfd(&fd, file);
	if (err)
	{
		fs_putfile(file);
		return err;
	}
	
	if (fso->mode & S_ISUID)
		curr->euid = fso->uid;
	if (fso->mode & S_ISGID)
		curr->egid = fso->gid;
	
#ifdef __ARCH_I386__
	if (ureg)
	{
		ureg->cs = USER_CS;
		ureg->ds = USER_DS;
		ureg->es = USER_DS;
		ureg->fs = USER_DS;
		ureg->gs = USER_DS;
		ureg->ss = USER_DS;
		
		ureg->eip = LDR_BASE + sizeof(struct exehdr);
		ureg->esp = USTACK;
		
		ureg->eflags &= 0xffc0822a;
		/* ureg->eflags |= 0x00000000; */
		
		ureg->eax = UARGS;
		ureg->ecx = XNAME;
		
		ureg->ebx = curr->exec_arg_size;
		ureg->edx = fd;
		
		ureg->esi = 0x00000000;
		ureg->edi = 0x00000000;
		
		ureg->ebp = 0x00000000;
	}
	
	if (fpu_present)
		asm("fninit");
#elif defined __ARCH_AMD64__
	if (ureg)
	{
		ureg->cs = USER_CS;
		ureg->ds = USER_DS;
		ureg->es = USER_DS;
		ureg->fs = USER_DS;
		ureg->gs = USER_DS;
		ureg->ss = USER_DS;
		
		ureg->rip = LDR_BASE + sizeof(struct exehdr);
		ureg->rsp = USTACK;
		
		ureg->rflags &= 0xffc0822a;
		/* ureg->eflags |= 0x00000000; */
		
		ureg->rdi = UARGS;
		ureg->rsi = curr->exec_arg_size;
		ureg->rdx = XNAME;
		ureg->rcx = fd;
		
		ureg->rbp = 0x00000000;
	}
	
	asm("fninit");
#else
#error Unknown arch
#endif
	return 0;
}
