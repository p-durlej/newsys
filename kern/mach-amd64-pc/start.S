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

#include <kern/arch/selector.h>

	.globl _start
	.globl	head

	.globl	fpu_present

	.globl	gdt_desc
	.globl	tss_desc
	.globl	idt_desc
	.globl	idt
	.globl	tss

	.globl	dma_buf

_start:
	jmp	start

	.org	_start + 16
base:	.long	_start
size:	.long	_end - _start
head:	.long	0

start:
	cld
	lidt	idt_desc
	lgdt	gdt_desc
	movw	$KERN_DS, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	$KERN_TSS, %ax
	ltr	%ax
	xorw	%ax, %ax
	movw	%ax, %ss
	lldt	%ax
	ljmpl	*1f
1:	.long	1f
	.word	KERN_CS
1:	pushq	%rdi
	movq	$_end, %rcx
	subq	$_edata, %rcx
	movq	$_edata, %rdi
	rep
	stosb
	popq	%rdi
	call	kmain
	jmp	.

fpu_present:	.long	1

gdt_desc:
	.word	gdt_end - gdt - 1
	.quad	gdt

idt_desc:
	.word	0x1000
	.quad	idt
dma_buf:
	.space	4096

	.data
	.balign	16
tss:
	.long	0
	.long	0
	.long	KERN_DS
	.space	90
	.word	104

gdt:
	.quad	0x0000000000000000 /* null descriptor */
	.quad	0x0020980000000000 /* kern cs */
	.quad	0x0000920000000000 /* kern ds */
	.quad	0x0020f80000000000 /* user cs */
	.quad	0x0000f20000000000 /* user ds */

tss_desc:
	.word	103 /* tss */
	.long	0x89000000 + tss
	.byte	0x00,0x00
	
	.quad	0 /* extra space (bochs/qemu bug?) */
	.quad	0x00009a000000ffff /* kern 16-bit cs */
	.quad	0x000092000000ffff /* kern 16-bit ds */
gdt_end:

idt:
	.space	0x1000
