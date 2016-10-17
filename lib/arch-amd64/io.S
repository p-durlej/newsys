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

	.globl	inb
	.globl	outb
	.globl	inw
	.globl	outw
	.globl	inl
	.globl	outl
	
	.globl	insb
	.globl	insw
	.globl	insl
	
	.globl	outsb
	.globl	outsw
	.globl	outsl
	
	.text
	.code64
	
outb:
	movl	%edi,%edx
	movl	%esi,%eax
	outb	%al,%dx
	ret

outw:
	movl	%edi,%edx
	movl	%esi,%eax
	outw	%ax,(%dx)
	ret

outl:
	movl	%edi,%edx
	movl	%esi,%eax
	outl	%eax,(%dx)
	ret
	
inb:
	movl	%edi,%edx
	inb	%dx,%al
	ret

inw:
	movl	%edi,%edx
	inw	(%dx),%ax
	ret

inl:
	movl	%edi,%edx
	inl	(%dx),%eax
	ret

insb:
	pushq	%rdx
	pushq	%rdi
	pushq	%rcx
	movq	%rdx, %rcx
	movl	%edi, %edx
	movq	%rsi, %rdi
	rep
	insb
	popq	%rcx
	popq	%rdi
	popq	%rdx
	ret
	jmp	.

insw:
	pushq	%rdx
	pushq	%rdi
	pushq	%rcx
	movq	%rdx, %rcx
	movl	%edi, %edx
	movq	%rsi, %rdi
	rep
	insw
	popq	%rcx
	popq	%rdi
	popq	%rdx
	ret

insl:
	pushq	%rdx
	pushq	%rdi
	pushq	%rcx
	movq	%rdx, %rcx
	movl	%edi, %edx
	movq	%rsi, %rdi
	rep
	insl
	popq	%rcx
	popq	%rdi
	popq	%rdx
	ret

outsb:
	movl	%edi, %edx
	movq	%rdx, %rcx
	rep
	outsb
	ret

outsw:
	movl	%edi, %edx
	movq	%rdx, %rcx
	rep
	outsw
	ret

outsl:
	movl	%edi, %edx
	movq	%rdx, %rcx
	rep
	outsl
	ret
