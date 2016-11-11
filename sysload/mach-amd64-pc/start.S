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

#include <arch/cpu.h>

#include <sysload/head.h>

#define PREINIT_STACK	1024

#define PML4_BASE	0x18000
#define PDP_BASE	0x19000
#define PD_BASE		0x1a000

#define CS16	0x08
#define DS16	0x10
#define CS32	0x18
#define DS32	0x20
#define CS64	0x28
#define DS64	0x30

	.globl	_start
	.globl	bdev
	.globl	halt
	
	.globl	conv_mem_size
	.globl	conv_mem_hbrk
	.globl	conv_mem_lbrk
	.globl	bangpxe
	.globl	pm_esp
	.globl	bioscall
	.globl	farcall
	.globl	bounce
	.globl	btype
	.globl	bcp
	.globl	fcp
	.globl	bbd
	.globl	cdrom
	
	.code16

	.text
_start:	jmp	start0

	.org	_start + 16
btype:	.long	BOOT_TYPE_DISK
base:	.long	_start
size:	.long	_end - _start
	.org	_start + 32
bdev:	.space	32

start0:
	movw	%cs, %ax
	movw	%ax, %ds
	movw	%ax, %es
	
	call	0f
0:	popw	%bx
	addw	$btype - 0b, %bx
	movb	(%bx), %al
	
	cmpb	$BOOT_TYPE_DISK, %al
	je	disk_boot
	cmpb	$BOOT_TYPE_PXE, %al
	je	pxe_boot
	cmpb	$BOOT_TYPE_CDROM, %al
	je	cdrom_boot
	jmp	halt
	
pxe_boot:
	std
	movw	$0x07c0, %ax
	movw	$0x0800, %bx
	movw	%ax, %ds
	movw	%bx, %es
	xorw	%si, %si
	xorw	%di, %di
	movw	$0xffff, %cx
	rep
	movsb
	
	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es
	
	movl	4(%esp), %eax
	movl	%eax, bangpxe
	
	ljmpw	$0, $start
	
cdrom_boot:
disk_boot:
	ljmpw	$0, $start
	
start:	cli
	cld
	
	// lidt	idt_desc
	
	movw	%cs,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%ss
	movw	$_start, %sp
	movb	%dl,bbd
	
	int	$0x12
	movzx	%ax,%eax
	shll	$10,%eax
	movl	%eax,conv_mem_size
	movl	%eax,conv_mem_hbrk
	
	/* XXX disable NMI */
	cli
	lidt	idt_desc
	lgdt	gdt_desc
	movl	%cr0,%eax
	incl	%eax
	movl	%eax,%cr0
	ljmpl	$CS32,$0f
	
	.code32
0:	movw	$DS32,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss
	
	/* uncompress the page tables */
	
	movl	$PML4_BASE, %edi
	movl	$0x4000, %ecx
	xorl	%eax, %eax
	rep
	stosl
	
	movl	$page_tables, %esi
0:	movl	 (%esi), %edi /* target base address */
	movl	4(%esi), %ecx /* next descriptor */
	addl	$8, %esi
	subl	%esi, %ecx
	rep
	movsb
	cmp	$0, (%esi)
	jne	0b
	
	movl	%cr4,%eax
	orl	$CR4_PAE | CR4_OSFXSR,%eax
	movl	%eax,%cr4
	movl	$PML4_BASE, %eax
	movl	%eax,%cr3
	movl	$MSR_EFER,%ecx
	rdmsr
	orl	$MSR_EFER_LME,%eax
	wrmsr
	movl	%cr0, %ebx
	orl	$CR0_PG,%ebx
	movl	%ebx,%cr0
	ljmpl	$CS64,$0f
	
	.code64
0:	movw	$DS64,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	xorw	%ax,%ax
	movw	%ax,%ss
	
	movq	$0x83, %rax
	movq	$2048, %rcx
	movq	$PD_BASE, %rdi
	cld
0:	stosq
	addq	$0x200000, %rax
	loop	0b
	
	xorb	%al,%al
	movq	$_end,%rcx
	subq	$_edata,%rcx
	movq	$_edata,%rdi
	rep
	stosb
	movq	$_end + PREINIT_STACK,%rsp
	
	pushfq
	popq	%rax
	movl	%eax, bcp
	call	hw_init
	call	mem_preinit
	movl	pm_esp,%esp
	call	main
	int	$0x55

bioscall:
	lidt	real_idt_desc
	pushq	%rbp
	pushq	%rbx
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	movq	%rsp, pm_esp
	ljmpl	*0f
0:	.long	0f
	.word	CS16
	
	.code16
0:	movw	$DS16,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss
	movl	$_start,%esp
	movl	%cr0,%eax
	andl	$0x7ffffffe, %eax
	movl	%eax,%cr0
	ljmpw	$0,$0f
0:	xorw	%ax,%ax
	movw	%ax,%ss
	movb	bcp + 40,%al
	movb	%al,1 + 0f
	
	pushw	bcp + 32
	pushw	bcp + 34
	pushw	bcp + 36
	pushw	bcp + 38
	pushl	bcp
	
	movl	bcp + 4, %eax
	movl	bcp + 8, %ebx
	movl	bcp + 12,%ecx
	movl	bcp + 16,%edx
	movl	bcp + 20,%esi
	movl	bcp + 24,%edi
	movl	bcp + 28,%ebp
	
	popfl
	popw	%gs
	popw	%fs
	popw	%es
	popw	%ds
	
	wbinvd	/* XXX bochs bug workaround */
	sti
0:	int	$0x55
	cli
	
	pushl	%eax
	pushw	%ds
	pushfl
	
	xorw	%ax,%ax
	movw	%ax,%ds
	
	movl	%ebx,bcp + 8
	movl	%ecx,bcp + 12
	movl	%edx,bcp + 16
	movl	%esi,bcp + 20
	movl	%edi,bcp + 24
	movl	%ebp,bcp + 28
	
	movw	%es,bcp + 34
	movw	%fs,bcp + 36
	movw	%gs,bcp + 38
	
	popl	bcp
	popw	bcp + 32
	popl	bcp + 4
	
	cli
	cld
	lidt	%cs:idt_desc
	lgdt	%cs:gdt_desc
	movl	%cr0,%eax
	orl	$0x80000001, %eax
	movl	%eax,%cr0
	ljmp	$CS64,$0f
	
	.code64
0:	movw	$DS64, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	xorw	%ax, %ax
	movw	%ax, %ss
	movq	pm_esp, %rsp
	
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	popq	%rbx
	popq	%rbp
	ret

farcall:
	lidt	real_idt_desc
	pushq	%rbp
	pushq	%rbx
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%r8
	pushq	%r9
	movq	%rsp, pm_esp
	ljmpl	*0f
0:	.long	0f
	.word	CS16
	
	.code16
0:	movw	$DS16,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss
	movl	$_start,%esp
	movl	%cr0,%eax
	andl	$0x7ffffffe, %eax
	movl	%eax,%cr0
	ljmpw	$0,$0f
0:	xorw	%ax,%ax
	movw	%ax,%ss
	movw	%ax,%ds
	
	pushl	fcp + 24
	pushl	fcp + 16
	
	wbinvd	/* XXX bochs bug workaround */
	sti
0:	lcallw	*%cs:(fcp + 12)
	cli
	
	movl	%eax, %cs:fcp
	movl	%edx, %cs:fcp + 4
	pushfl
	popl	%cs:fcp + 8
	
	addw	$8, %sp
	
	cli
	cld
	lidt	%cs:idt_desc
	lgdt	%cs:gdt_desc
	movl	%cr0,%eax
	orl	$0x80000001, %eax
	movl	%eax,%cr0
	ljmp	$CS64,$0f
	
	.code64
0:	movw	$DS64, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	xorw	%ax, %ax
	movw	%ax, %ss
	movq	pm_esp, %rsp
	
	popq	%r9
	popq	%r8
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	popq	%rbx
	popq	%rbp
	ret
	
halt:
	hlt
	jmp	halt

	.code16

idt_desc:
	.word	0
	.quad	0

real_idt_desc:
	.word	0x03ff
	.quad	0

gdt_desc:
	.word	0x07ff
	.quad	gdt
gdt:
	.quad	0x0000000000000000 /* null descriptor */
	.quad	0x00009a000000ffff /* CS16 */
	.quad	0x000092000000ffff /* DS16 */
	.quad	0x00cf9a000000ffff /* CS32 */
	.quad	0x00cf92000000ffff /* DS32 */
	.quad	0x0020980000000000 /* CS64 */
	.quad	0x0000900000000000 /* DS64 */

conv_mem_size:	.long	0
conv_mem_hbrk:	.long	0
conv_mem_lbrk:	.long	0x20000
bangpxe:	.long	0
pm_esp:		.quad	0
bcp:		.space	44
fcp:		.space	24
bounce:		.space	2048
bbd:		.long	0

page_tables:
	.long	PML4_BASE /* PML4 */
	.long	0f
	
	.quad	PDP_BASE + 3		/* BASE = pdp, R/W = 1, P = 1 */
0:
	.long	PDP_BASE /* PDP */
	.long	0f
	
	.quad	PD_BASE + 0x0003	/* BASE = pd,  R/W = 1, P = 1 */
	.quad	PD_BASE + 0x1003	/* BASE = pd,  R/W = 1, P = 1 */
	.quad	PD_BASE + 0x2003	/* BASE = pd,  R/W = 1, P = 1 */
	.quad	PD_BASE + 0x3003	/* BASE = pd,  R/W = 1, P = 1 */
0:
	.long	PD_BASE /* PD */
	.long	0f
	.quad	0x83			/* BASE = 0,  PS = 1, R/W = 1, P = 1 */

0:	.long	0
	.long	0
