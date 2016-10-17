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
#include <kern/arch/intr_regs.h>
#include <kern/printk.h>

void dump_regs(struct intr_regs *r, char *prefix)
{
	printk("%srip = 0x%x\n", prefix, r->rip);
	printk("%srsp = 0x%x\n", prefix, r->rsp);
	printk("%srax = 0x%x\n", prefix, r->rax);
	printk("%srbx = 0x%x\n", prefix, r->rbx);
	printk("%srcx = 0x%x\n", prefix, r->rcx);
	printk("%srdx = 0x%x\n", prefix, r->rdx);
	printk("%srsi = 0x%x\n", prefix, r->rsi);
	printk("%srdi = 0x%x\n", prefix, r->rdi);
	printk("%srbp = 0x%x\n", prefix, r->rbp);
	printk("%sr8  = 0x%x\n", prefix, r->r8);
	printk("%sr9  = 0x%x\n", prefix, r->r9);
	printk("%sr10 = 0x%x\n", prefix, r->r10);
	printk("%sr11 = 0x%x\n", prefix, r->r11);
	printk("%sr12 = 0x%x\n", prefix, r->r12);
	printk("%sr13 = 0x%x\n", prefix, r->r13);
	printk("%sr14 = 0x%x\n", prefix, r->r14);
	printk("%sr15 = 0x%x\n", prefix, r->r15);
}
