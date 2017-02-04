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

	.globl	fault_jmp_set
	
	.globl	fault
	.globl	fault_fini
	.globl	fault_abort
	
	.data
fault_jmp_set:		.long	0
fault_jmp_rbx:		.quad	0
fault_jmp_rcx:		.quad	0
fault_jmp_rdx:		.quad	0
fault_jmp_rsi:		.quad	0
fault_jmp_rdi:		.quad	0
fault_jmp_rbp:		.quad	0
fault_jmp_r8:		.quad	0
fault_jmp_r9:		.quad	0
fault_jmp_r10:		.quad	0
fault_jmp_r11:		.quad	0
fault_jmp_r12:		.quad	0
fault_jmp_r13:		.quad	0
fault_jmp_r14:		.quad	0
fault_jmp_r15:		.quad	0
fault_jmp_rsp:		.quad	0
fault_jmp_rflags:	.quad	0
fault_jmp_rip:		.quad	0

	.text
fault:
	cmpb	$0, fault_jmp_set
	jne	fault_panic
	movb	$1, fault_jmp_set
	
	movq	%rbx, fault_jmp_rbx
	movq	%rcx, fault_jmp_rcx
	movq	%rdx, fault_jmp_rdx
	movq	%rsi, fault_jmp_rsi
	movq	%rdi, fault_jmp_rdi
	movq	%r8,  fault_jmp_r8
	movq	%r9,  fault_jmp_r9
	movq	%r10, fault_jmp_r10
	movq	%r11, fault_jmp_r11
	movq	%r12, fault_jmp_r12
	movq	%r13, fault_jmp_r13
	movq	%r14, fault_jmp_r14
	movq	%r15, fault_jmp_r15
	movq	%rbp, fault_jmp_rbp
	movq	%rsp, fault_jmp_rsp
	pushfq
	popq	fault_jmp_rflags
	movq	(%rsp), %rax
	movq	%rax, fault_jmp_rip
	
	xorl	%eax, %eax
	ret
fault_panic:
	movq	$fault_msg, %rdi
	call	panic

fault_fini:
	cmpb	$1, fault_jmp_set
	jne	fault_fini_panic
	movb	$0, fault_jmp_set
	ret
fault_fini_panic:
	movq	$fault_fini_msg, %rdi
	call	panic

fault_abort:
	cmpb	$1, fault_jmp_set
	jne	fault_fini_panic
	movb	$0, fault_jmp_set
	
	movq	fault_jmp_rbx, %rbx
	movq	fault_jmp_rcx, %rcx
	movq	fault_jmp_rdx, %rdx
	movq	fault_jmp_rsi, %rsi
	movq	fault_jmp_rdi, %rdi
	movq	fault_jmp_rbp, %rbp
	movq	fault_jmp_r8,  %r8
	movq	fault_jmp_r9,  %r9
	movq	fault_jmp_r10, %r10
	movq	fault_jmp_r11, %r11
	movq	fault_jmp_r12, %r12
	movq	fault_jmp_r13, %r13
	movq	fault_jmp_r14, %r14
	movq	fault_jmp_r15, %r15
	movq	fault_jmp_rsp, %rsp
	pushq	fault_jmp_rflags
	popfq
	movq	fault_jmp_rip, %rax
	movq	%rax, (%rsp)
	
	movl	$1, %eax
	ret
fault_abort_panic:
	movq	$fault_abort_msg, %rdi
	call	panic

fault_msg:		.asciz	"fault.S: fault: fault longjmp already set"
fault_fini_msg:		.asciz	"fault.S: fault_fini: fault longjmp not set"
fault_abort_msg:	.asciz	"fault.S: fault_abort: fault longjmp not set"
