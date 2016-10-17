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

	.globl setjmp
	.globl longjmp

setjmp:
	movq	%rbx,    (%rdi)
	movq	%rcx,   8(%rdi)
	movq	%rdx,  16(%rdi)
	movq	%rsi,  24(%rdi)
	movq	%rdi,  32(%rdi)
	movq	%rbp,  40(%rdi)
	movq	%r8,   48(%rdi)
	movq	%r9,   56(%rdi)
	movq	%r10,  64(%rdi)
	movq	%r11,  72(%rdi)
	movq	%r12,  80(%rdi)
	movq	%r13,  88(%rdi)
	movq	%r14,  96(%rdi)
	movq	%r15, 104(%rdi)
	movq	%rsp, 112(%rdi)
	movq	(%rsp), %rax
	movq	%rax, 120(%rdi)
	
	movl	$0, %eax
	ret

longjmp:
	movl	%esi, %eax
	
	movq	   (%rdi), %rbx
	movq	  8(%rdi), %rcx
	movq	 16(%rdi), %rdx
	movq	 24(%rdi), %rsi
	movq	 40(%rdi), %rbp
	movq	 48(%rdi), %r8
	movq	 56(%rdi), %r9
	movq	 64(%rdi), %r10
	movq	 72(%rdi), %r11
	movq	 80(%rdi), %r12
	movq	 88(%rdi), %r13
	movq	 96(%rdi), %r14
	movq	104(%rdi), %r15
	movq	112(%rdi), %rsp
	pushq	120(%rdi)
	movq	 32(%rdi), %rdi
	
	ret
