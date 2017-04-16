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

#include <kern/arch/intr_regs.h>
#include <kern/arch/selector.h>

#define IRQENTER			\
	incl	irq_level;		\
	cmpl	$1,irq_level;		\
	jne	1f;			\
	pushl	ureg;			\
	movl	$0,ureg;		\
	movl	%esp,%eax;		\
	movl	irq_stack, %ebx;	\
	addl	$65536, %ebx;		\
	movl	%ebx, %esp;		\
	pushl	%eax;			\
	/* movl	$0,%ebp; */		\
1:					\
	sti

#define IRQLEAVE			\
	cli;				\
	decl	irq_level;		\
	cmpl	$0,irq_level;		\
	jne	1f;			\
	popl	%eax;			\
	movl	%eax,%esp;		\
	popl	ureg;			\
1:

#define IRQHAND(nr)			\
	.globl asm_irq_ ## nr;		\
asm_irq_ ## nr ## :			\
	cld;				\
	SAVE_REGS;			\
	movw	%ds,%ax;		\
	cmpw	$KERN_DS,%ax;		\
	je	1f;			\
	movl	$0,%ebp;		\
	LOAD_SEGS;			\
1:					\
	IRQENTER;			\
	movl	%esp,%eax;		\
	pushl	$nr;			\
	pushl	%eax;			\
	call	intr_enter;		\
	pushl	$nr;			\
	call	intr_irq;		\
	addl	$4,%esp;		\
	call	intr_leave;		\
	add	$8,%esp;		\
	IRQLEAVE;			\
	RESTORE_REGS;			\
	iret

IRQHAND(0)
IRQHAND(1)
IRQHAND(2)
IRQHAND(3)
IRQHAND(4)
IRQHAND(5)
IRQHAND(6)
IRQHAND(7)
IRQHAND(8)
IRQHAND(9)
IRQHAND(10)
IRQHAND(11)
IRQHAND(12)
IRQHAND(13)
IRQHAND(14)
IRQHAND(15)
