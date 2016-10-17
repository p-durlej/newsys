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

	.globl	switch_stack
	.globl	fpu_save
	.globl	fpu_load
	.globl	fpu_init

	.text

fpu_init:
	fninit
	ret

fpu_save:
	movl	4(%esp),%eax
	fnsave	(%eax)
	ret

fpu_load:
	movl	4(%esp),%eax
	fninit
	frstor	(%eax)
	ret

switch_stack:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%ebx
	pushl	%ecx
	pushl	%esi
	pushl	%edi
	pushfl
	pushl	ureg

	movl	8(%ebp),%eax
	movl	%esp,32760(%eax)
	movl	12(%ebp),%eax
	movl	32760(%eax),%ebx	 /* offset of curr->k_sp == 32760 */
	cmpl	$0,%ebx
	je	1f
	cmpl	$1,%ebx
	je	2f
	movl	%eax,curr
	movl	%ebx,%esp
	addl	$32760,%eax		 /* sizeof(curr->k_stack) == 32760 */
	movl	%eax,tss+4
	
	popl	ureg
	popfl
	popl	%edi
	popl	%esi
	popl	%ecx
	popl	%ebx
	popl	%ebp
	ret
1:
	movl	%eax,curr
	movl	$0,ureg
	movl	$0,%ebp
	addl	$32760,%eax		 /* sizeof(curr->k_stack) == 32760 */
	movl	%eax,tss+4
	movl	%eax,%esp
	sti
	jmp	new_task_entry
2:
	movl	%eax,curr
	movl	$0,ureg
	movl	$0,%ebp
	addl	$32760,%eax		 /* sizeof(curr->k_stack) == 32760 */
	movl	%eax,tss+4
	subl	$64,%eax		 /* sizeof(struct intr_regs) == 64 */
	movl	%eax,%esp
	pushl	%eax
	sti
	call	after_fork
