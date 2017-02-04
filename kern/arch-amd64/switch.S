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

#include <kern/machine/machine.h>

	.globl	switch_stack
	.globl	fpu_save
	.globl	fpu_load
	.globl	fpu_init

	.text

fpu_init:
	fninit
	ldmxcsr	0f
	ret
0:	.quad	0

fpu_save:
	fxsave	(%rdi)
	ret

fpu_load:
	fninit
	fxrstor	(%rdi)
	ret

switch_stack:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%rbx
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15
	pushfq
	pushq	ureg
	
	movq	%rsp, 32760(%rdi)
	movq	32760(%rsi), %rbx	/* offset of curr->k_sp == 32760 */
	cmpq	$0, %rbx
	je	1f
	cmpq	$1, %rbx
	je	2f
	
	movq	%rsi, curr
	movq	%rbx, %rsp
	
	movq	%rsi, %rax
	addq	$32760, %rax		/* sizeof(curr->k_stack) == 32760 */
	movq	%rax, tss + 4
	
	popq	ureg
	popfq
	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%rbx
	popq	%rbp
	ret
1:	movq	%rsi, curr
	movq	$0, ureg
	movq	$0, %rbp
	
	movq	%rsi, %rax
	addq	$32760, %rax
	movq	%rax, tss + 4
	movq	%rax, %rsp
	sti
	jmp	new_task_entry
2:	movq	%rsi, curr
	movq	$0, ureg
	movq	$0, %rbp
	
	movq	%rsi, %rax
	addq	$32760, %rax
	movq	%rax, tss + 4
	movq	%rax, %rsp
	
	addq	$32760, %rsi		 /* sizeof(curr->k_stack) == 32760 */
	subq	$192, %rsi		 /* sizeof(struct intr_regs) == 192 */
	movq	%rsi, %rsp
	movq	%rsi, %rdi
	
	sti
	jmp	after_fork
