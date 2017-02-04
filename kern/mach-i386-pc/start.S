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

#include <kern/arch/selector.h>

	.globl _start
	.globl	head

	.globl	fpu_present

	.globl	gdt_desc
	.globl	tss_desc
	.globl	idt_desc
	.globl	kv86_tss_desc
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
	lgdt	gdt_desc
	lidt	idt_desc
	ljmp	$KERN_CS,$0f
0:	movw	$KERN_DS,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss
	// movw	$_start,%sp
	call	chkfpu /* XXX intr disabled */

	cmpl	$0x00000000,fpu_present
	je	1f

	movl	%cr0,%eax
	orl	$0x00010023,%eax
	movl	%eax,%cr0
	jmp	start32

1:	movl	%cr0,%eax
	orl	$0x00010005,%eax
	movl	%eax,%cr0
	jmp	start32

	.balign	4

fpu_present:	.long	0

chkfpu_stw:
	.word	0xffff
chkfpu:
	fninit
	fnstsw	chkfpu_stw
	cmpw	$0x0000,chkfpu_stw
	jne	chkfpu_ret
	movw	$0x0001,fpu_present
chkfpu_ret:
	ret

gdt_desc:
	.word	0xffff
	.long	gdt

idt_desc:
	.word	0x07ff
	.long	idt
dma_buf:
	.space	4096

start32:
	clts
	movl	4(%esp),%ebx
	movl	(%ebx),%esp /* XXX */
	movl	%esp,tss + 4
	pushl	%ebx
	movl	$0x00000000,%ebp
	movw	$0x0000,%ax
	lldt	%ax
	movw	$KERN_TSS,%ax
	ltr	%ax
fillbss:
	cld
	movl	$_end,%ecx
	subl	$_edata,%ecx
	movl	$_edata,%edi
	xorb	%al,%al
	rep
	stosb

	movl	%cr0,%eax
	andl	$~(3 << 29),%eax /* enable caching */
	movl	%eax,%cr0

	call	kmain
	jmp	.

	.data
	.balign	16
tss:
	.long	0
	.long	0
	.long	KERN_DS
	.space	90
	.word	104

kv86_tss:
	.long	0
	.long	kv86_intr_stack_top
	.long	KERN_DS
	.space	90
	.word	104
	
	.space	8195

gdt:
	.quad	0x0000000000000000 /* null descriptor */
	.quad	0x00cf9a000000ffff /* kern cs */
	.quad	0x00cf92000000ffff /* kern ds */
	.quad	0x00cffa000000ffff /* user cs */
	.quad	0x00cff2000000ffff /* user ds */

tss_desc:
	.word	103 /* tss */
	.long	0x89000000 + tss
	.byte	0x00,0x00

	.quad	0x00009a000000ffff /* kern16 cs */
	.quad	0x000092000000ffff /* kern16 ds */

kv86_tss_desc:
	.word	103 + 8195 /* tss */
	.long	0x89000000 + kv86_tss
	.byte	0x00,0x00
	
	.org	gdt + 0x10000

idt:
	.space	0x0800

	.data
	.space	8192
kv86_intr_stack_top:
