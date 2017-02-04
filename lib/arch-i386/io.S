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
inb:
	push	%edx
	movw	8(%esp),%dx
	inb	%dx,%al
	and	$0x000000ff,%eax
	pop	%edx
	ret

outb:
	push	%edx
	movw	8(%esp),%dx
	movb	12(%esp),%al
	outb	%al,%dx
	pop	%edx
	ret

inw:
	push	%edx
	movw	8(%esp),%dx
	inw	%dx,%ax
	and	$0x0000ffff,%eax
	pop	%edx
	ret

outw:
	push	%edx
	movw	8(%esp),%dx
	movw	12(%esp),%ax
	outw	%ax,%dx
	pop	%edx
	ret

inl:
	push	%edx
	movw	8(%esp),%dx
	inl	%dx,%eax
	pop	%edx
	ret

outl:
	push	%edx
	movw	8(%esp),%dx
	movl	12(%esp),%eax
	outl	%eax,%dx
	pop	%edx
	ret

insb:
	pushl	%edi
	movw	8(%esp),  %dx
	movl	12(%esp), %edi
	movl	16(%esp), %ecx
	rep
	insb
	popl	%edi
	ret

insw:
	pushl	%edi
	movw	8(%esp),  %dx
	movl	12(%esp), %edi
	movl	16(%esp), %ecx
	rep
	insw
	popl	%edi
	ret

insl:
	pushl	%edi
	movw	8(%esp),  %dx
	movl	12(%esp), %edi
	movl	16(%esp), %ecx
	rep
	insl
	popl	%edi
	ret

outsb:
	pushl	%esi
	movw	8(%esp),  %dx
	movl	12(%esp), %esi
	movl	16(%esp), %ecx
	rep
	outsb
	popl	%esi
	ret

outsw:
	pushl	%esi
	movw	8(%esp),  %dx
	movl	12(%esp), %esi
	movl	16(%esp), %ecx
	rep
	outsw
	popl	%esi
	ret

outsl:
	pushl	%esi
	movw	8(%esp),  %dx
	movl	12(%esp), %esi
	movl	16(%esp), %ecx
	rep
	outsl
	popl	%esi
	ret
