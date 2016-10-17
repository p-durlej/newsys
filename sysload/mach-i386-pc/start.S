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

#include <sysload/head.h>

#define PREINIT_STACK	1024

#define CS16	0x08
#define DS16	0x10
#define CS32	0x18
#define DS32	0x20

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
	.globl	bcp
	.globl	fcp
	.globl	bbd
	.globl	cdrom
	
	.code16

	.text
_start:	jmp	start0

	.org	_start + 16
type:	.long	BOOT_TYPE_DISK
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
	addw	$type - 0b, %bx
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
	movw	$0x7c00 + _edata - _start - 1, %si
	movw	$_edata - 1, %di
	movw	$_edata - _start, %cx
	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es
	rep
	movsb
	
	movl	4(%esp), %eax
	movl	%eax, bangpxe
	
	ljmpw	$0, $start
	
cdrom_boot:
	movl	$1, cdrom
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
	xorb	%al,%al
	/* movl	$_end - _edata,%ecx */
	movl	$_end,%ecx
	subl	$_edata,%ecx
	movl	$_edata,%edi
	rep
	stosb
	movl	$_end + PREINIT_STACK,%esp
	pushfl
	popl	bcp
	call	hw_init
	call	mem_preinit
	movl	pm_esp,%esp
	call	main
	int	$0x55

bioscall:
	lidt	real_idt_desc
	pushal
	movl	%esp,pm_esp
	ljmpw	$CS16,$0f
	
	.code16
0:	movw	$DS16,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss
	movl	$_start,%esp
	movl	%cr0,%eax
	dec	%eax
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
	
	/* addw	$8, %sp */
	
bcrestore:
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
	movl	%cr0,%eax
	incl	%eax
	movl	%eax,%cr0
	ljmp	$CS32,$0f
	
	.code32
0:	movw	$DS32,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss
	movl	pm_esp,%esp
	popal
	ret

farcall:
	lidt	real_idt_desc
	pushfl
	pushal
	movl	%esp,pm_esp
	ljmpw	$CS16,$0f
	
	.code16
0:	movw	$DS16,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ax,%ss
	movl	$_start,%esp
	movl	%cr0,%eax
	decl	%eax
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
	
	lidt	%cs:idt_desc
	lgdt	%cs:gdt_desc
	movl	%cr0, %eax
	incl	%eax
	movl	%eax, %cr0
	ljmpl	$CS32, $0f
	.code32
0:	movw	$DS32, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss
	movl	pm_esp, %esp
	popal
	popfl
	ret
	
halt:
	hlt
	jmp	halt

	.code16

idt_desc:
	.quad	0

real_idt_desc:
	.word	0x03ff
	.long	0

gdt_desc:
	.word	0x07ff
	.long	gdt
gdt:
	.quad	0x0000000000000000 /* null descriptor */
	.quad	0x00009a000000ffff /* CS16 */
	.quad	0x000092000000ffff /* DS16 */
	.quad	0x00cf9a000000ffff /* CS32 */
	.quad	0x00cf92000000ffff /* DS32 */

conv_mem_size:	.long	0
conv_mem_hbrk:	.long	0
conv_mem_lbrk:	.long	0x20000
bangpxe:	.long	0
pm_esp:		.long	0
bcp:		.space	44
fcp:		.space	24
bounce:		.space	2048
bbd:		.long	0
cdrom:		.long	0
