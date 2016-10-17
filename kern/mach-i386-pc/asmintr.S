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

#include <kern/arch/intr_regs.h>
#include <kern/arch/selector.h>

#define INTRHAND(proc,arg)		\
	cld;				\
	SAVE_REGS;			\
	movw	%ds,%ax;		\
	cmpw	$KERN_DS,%ax;		\
	je	1f;			\
	movl	$0x00000000,%ebp;	\
	LOAD_SEGS;			\
1: \
	movl	%esp,%eax; \
	pushl	$arg; \
	pushl	%eax; \
	call	intr_enter; \
	call	proc; \
	call	intr_leave; \
	add	$0x00000008,%esp; \
	RESTORE_REGS; \
	iret

	.globl asm_syscall
asm_syscall:
	sti
	INTRHAND(intr_syscall,0)

#define EXCHAND(nr,ecode) \
	.globl asm_exc_ ## nr; \
asm_exc_ ## nr ## : \
	add	$ecode,%esp; \
	INTRHAND(intr_exc,nr)

	.globl	asm_spurious

asm_spurious:
	iret

EXCHAND(0,0)
EXCHAND(1,0)
EXCHAND(2,0)
EXCHAND(3,0)
EXCHAND(4,0)
EXCHAND(5,0)
EXCHAND(6,0)
EXCHAND(7,0)
EXCHAND(8,4)
EXCHAND(9,4)
EXCHAND(10,4)
EXCHAND(11,4)
EXCHAND(12,4)
EXCHAND(13,4)
EXCHAND(14,4)
EXCHAND(15,0)
EXCHAND(16,0)
EXCHAND(17,4)
EXCHAND(18,0)

	.globl	intr_aena
	.globl	intr_res
	.globl	intr_ena
	.globl	intr_dis

intr_aena:
	pushfl
	popl	%eax
	andl	$0x00000200,%eax
	jz	0f
	ret
0:	pushl	$0f
	call	panic
0:	.asciz	"intr_aena: interrupts disabled"

intr_res:
	movl	4(%esp),%edx
	pushfl
	movl	(%esp),%eax
	andl	$0xfffffdff,(%esp)
	andl	$0x00000200,%edx
	orl	%edx,(%esp)
	popfl
	ret

intr_ena:
	pushfl
	pop	%eax
	sti
	ret

intr_dis:
	pushfl
	pop	%eax
	cli
	ret
