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

#include <kern/syscall.h>
#include <kern/signal.h>
#include <kern/config.h>
#include <kern/printk.h>
#include <kern/errno.h>
#include <kern/task.h>
#include <kern/intr.h>
#include <kern/umem.h>

#include "../syslist/systab.h"

void uerr(int errno)
{
#if SYS_DEBUG
	printk("uerr: curr->errno = %p\n", curr->errno);
	printk("uerr: err = %i\n", errno);
#endif
	if (errno < 1)
		panic("uerr: bad error code");
	if (!curr->errno || tucpy((void *)curr->errno, &errno, sizeof errno))
		signal_raise_fatal(SIGBUS);
#if SYS_DEBUG
	printk("uerr: fini\n");
#endif
}

void uret(uint64_t val)
{
	ureg->edx = val >> 32;
	ureg->eax = val;
}

void syscall(void)
{
	int (*proc)(unsigned arg0, unsigned arg1, unsigned arg2, unsigned arg3,
		    unsigned arg4, unsigned arg5, unsigned arg6, unsigned arg7);
	unsigned *arg = (void *)(ureg->esp + 4);
	int err;
	int nr;
	
	nr = ureg->eax;
	intr_aena();
#if SYS_DEBUG
	printk("syscall: %s(%u): nr = %i\n", curr->exec_name, (unsigned)curr->pid, nr);
#endif
	
	if (nr >= NR_SYS || !syscall_tab[nr].proc)
	{
		uerr(ENOSYS);
		uret(-1);
		return;
	}
	
	if (curr->euid && syscall_tab[nr].uidz)
	{
		uerr(EPERM);
		uret(-1);
		return;
	}
	
	err = uga(&arg, sizeof *arg * 8, UA_READ);
	if (err)
	{
		uerr(err);
		uret(-1);
		return;
	}
	
	proc = syscall_tab[nr].proc;
	uret(proc(arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7]));
	intr_aena();
}
