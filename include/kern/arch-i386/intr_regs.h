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

#ifndef _KERN_ARCH_INTR_REGS_H
#define _KERN_ARCH_INTR_REGS_H

#ifdef __ASSEMBLER__

#define SAVE_REGS			\
	pushl	%eax;			\
	pushl	%ebx;			\
	pushl	%ecx;			\
	pushl	%edx;			\
	pushl	%esi;			\
	pushl	%edi;			\
	pushl	%ebp;			\
	pushl	%ds;			\
	pushl	%es;			\
	pushl	%fs;			\
	pushl	%gs

#define RESTORE_REGS			\
	popl	%gs;			\
	popl	%fs;			\
	popl	%es;			\
	popl	%ds;			\
	popl	%ebp;			\
	popl	%edi;			\
	popl	%esi;			\
	popl	%edx;			\
	popl	%ecx;			\
	popl	%ebx;			\
	popl	%eax

#define LOAD_SEGS			\
	movw	$KERN_DS, %ax;		\
	movw	%ax, %ds;		\
	movw	%ax, %es;		\
	movw	%ax, %fs;		\
	movw	%ax, %gs

#else

struct intr_regs /* sizeof must be 16 * 4; see sched.c */
{
	unsigned gs;
	unsigned fs;
	unsigned es;
	unsigned ds;
	unsigned ebp;
	unsigned edi;
	unsigned esi;
	unsigned edx;
	unsigned ecx;
	unsigned ebx;
	unsigned eax;
	
	unsigned eip;
	unsigned cs;
	unsigned eflags;
	unsigned esp;
	unsigned ss;
};

#endif

#endif
