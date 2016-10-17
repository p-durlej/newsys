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

#include <priv/copyright.h>

#include <sysload/kparam.h>

#include <kern/machine/machine.h>
#include <kern/console.h>
#include <kern/config.h>
#include <kern/printk.h>
#include <kern/module.h>
#include <kern/clock.h>
#include <kern/start.h>
#include <kern/exec.h>
#include <kern/page.h>
#include <kern/task.h>
#include <kern/intr.h>
#include <kern/main.h>
#include <kern/lib.h>
#include <kern/hw.h>
#include <kern/fs.h>

void kmain(struct kparam *kp) __attribute__((noreturn));

struct kparam kparam;

void kmain(struct kparam *kp)
{
	int err;
	
	kparam = *kp;
	
	bootcon_init();
	intr_init();
	
	printk(COPYRIGHT "\n\n");
	//clock_delay(clock_hz() * 2);
	
	mach_boot();
	pg_init();
	malloc_boot();
	task_init();
	blk_init();
	fs_init();
	win_init();
	mod_boot();
	
	rd_boot();
	
	err = fs_mount("", kparam.root_dev, kparam.root_type, kparam.root_flags);
	if (err)
	{
		printk("fs_mount: (%s, type %s)", kparam.root_dev, kparam.root_type);
		perror("", err);
		panic("unable to mount root fs");
	}
	if (kparam.boot_flags & BOOT_VERBOSE)
		printk("mounted root (%s, type %s)\n", kparam.root_dev, kparam.root_type);
	
	task_exec("/sbin/init", NULL, 0);
	err = do_exec();
	curr->time_slice = 0;
	if (err)
	{
		perror("main: /sbin/init", err);
		panic("unable to execute /sbin/init");
	}
	
	if (kparam.boot_flags & BOOT_VERBOSE)
		printk("booting userspace\n");
	uboot();
}
