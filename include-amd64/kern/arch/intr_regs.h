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

#ifndef _KERN_ARCH_INTR_REGS_H
#define _KERN_ARCH_INTR_REGS_H

#ifdef __ASSEMBLER__

#define SAVE_REGS			\
	pushq	%rax;			\
	pushq	%rbx;			\
	pushq	%rcx;			\
	pushq	%rdx;			\
	pushq	%rsi;			\
	pushq	%rdi;			\
	pushq	%rbp;			\
	pushq	%r8;			\
	pushq	%r9;			\
	pushq	%r10;			\
	pushq	%r11;			\
	pushq	%r12;			\
	pushq	%r13;			\
	pushq	%r14;			\
	pushq	%r15;			\
	movq	%ds, %rbx;		\
	pushq	%rbx;			\
	movq	%es, %rbx;		\
	pushq	%rbx;			\
	pushq	%fs;			\
	pushq	%gs

#define RESTORE_REGS			\
	popq	%gs;			\
	popq	%fs;			\
	popq	%rbx;			\
	movq	%rbx, %es;		\
	popq	%rbx;			\
	movq	%rbx, %ds;		\
	popq	%r15;			\
	popq	%r14;			\
	popq	%r13;			\
	popq	%r12;			\
	popq	%r11;			\
	popq	%r10;			\
	popq	%r9;			\
	popq	%r8;			\
	popq	%rbp;			\
	popq	%rdi;			\
	popq	%rsi;			\
	popq	%rdx;			\
	popq	%rcx;			\
	popq	%rbx;			\
	popq	%rax

#define LOAD_SEGS			\
	movw	$KERN_DS, %ax;		\
	movw	%ax, %ds;		\
	movw	%ax, %es;		\
	movw	%ax, %fs;		\
	movw	%ax, %gs

#else

struct intr_regs /* XXX sizeof must be 16 * 4; see sched.c */
{
	unsigned long	gs;
	unsigned long	fs;
	unsigned long	es;
	unsigned long	ds;
	unsigned long	r15;
	unsigned long	r14;
	unsigned long	r13;
	unsigned long	r12;
	unsigned long	r11;
	unsigned long	r10;
	unsigned long	r9;
	unsigned long	r8;
	unsigned long	rbp;
	unsigned long	rdi;
	unsigned long	rsi;
	unsigned long	rdx;
	unsigned long	rcx;
	unsigned long	rbx;
	unsigned long	rax;
	
	unsigned long	rip;
	unsigned long	cs;
	unsigned long	rflags;
	unsigned long	rsp;
	unsigned long	ss;
};

#endif

#endif
