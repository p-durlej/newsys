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
#include <kern/arch/intr_regs.h>
#include <kern/page.h>

	.globl	get_bios_buf
	.globl	get_bios_buf_size
	.globl	get_bios_regs
	.globl	bios_init
	.globl	v86_init
	.globl	v86_call
	.globl	apm_off

	.globl	bios_buf
	.globl	irq_dcount

#define ABS16(v)	((v) - c16_start + CODE_BASE)

#define CODE_BASE	0x08000
#define STK_TOP		0x0fff0
#define BUF_BASE	0x10000
#define BUF_SIZE	0x10000
#define PDIR_BASE	0x20000
#define PTAB_BASE	0x21000
#define IRQ_BASE	0x40

	.code32

get_bios_regs:
	movl	$ABS16(bios_regs),%eax
	ret

get_bios_buf_size:
	movl	$BUF_SIZE,%eax
	ret

get_bios_buf:
	movl	$BUF_BASE,%eax
	ret

bios_init:
	/* copy the 16-bit code */
	
	movl	$c16_end - c16_start,%ecx
	movl	$c16_start,%esi
	movl	$ABS16(c16_start),%edi
	cld
	rep
	movsb
	
	ret

apm_off:
	jmp	ABS16(low_apm_off)

c16_start:

low_apm_off:
	cli
	movl	$STK_TOP,%esp
	movl	%cr0,%eax
	andl	$0x7fffffff,%eax
	movl	%eax,%cr0
	xorl	%eax,%eax
	movl	%eax,%cr3
	ljmp	$KERN16_CS, $ABS16(apm_off_16)

v86_init:
	/* Initialize the page table for V86 mode. The current code doesn't
	 * care for the address wraparound.
	 *
	 */
	
	movl	$0x007, %eax /* Mark the pages user, read-write and present. */
	movl	$PTAB_BASE, %edi
	movl	$1024, %ecx
0:	stosl
	addl	$0x1000, %eax
	loopl	0b
	
	/* Install the first PDE. The remaining entries will be copied when
	 * v86_call is called as they may change when the system is allocating
	 * address space.
	 *
	 */
	
	movl	$PTAB_BASE | 0x007, PDIR_BASE
	ret

v86_call:
	pushal
	
	/* Use a TSS with a higher limit that covers entire IOPM to allow
	 * all IOIO
	 */
	
	movb	$0x89, kv86_tss_desc + 5 /* unbusy the TSS */
	movw	$KV86_TSS, %ax
	ltr	%ax
	
	/* Update the V86 PDEs. */
	
	call	pg_getdir
	movl	%eax, %esi
	addl	$4, %esi
	movl	$PDIR_BASE + 4, %edi
	movl	$1023, %ecx
	rep
	movsl
	
	jmp	ABS16(low_v86_call)

low_v86_call:
	SAVE_REGS
	pushfl
	movl	%esp, ABS16(kern_esp)
	
	cli
	
	sgdt	ABS16(kern_gdt_desc)
	sidt	ABS16(kern_idt_desc)
	
	movl	%cr0, %eax
	movl	%eax, ABS16(kern_cr0)
	
	movl	%cr3, %eax
	movl	%eax, ABS16(kern_cr3)
	
	movl	$PDIR_BASE, %eax
	movl	%eax, %cr3
	
	movb	ABS16(bios_intr), %al
	movb	%al, ABS16(low_v86_intr + 1)
	
	pushl	ABS16(bios_gs)
	pushl	ABS16(bios_fs)
	pushl	ABS16(bios_ds)
	pushl	ABS16(bios_es)
	pushl	$0 /* SS */
	pushl	$STK_TOP
	movl	ABS16(bios_eflags), %eax
	orl	$0x00023000, %eax /* enable VM, IOPL = 3 */
	pushl	%eax
	pushl	$0 /* CS */
	pushl	$ABS16(low_v86_intr)
	
	movl	ABS16(bios_eax), %eax
	movl	ABS16(bios_ebx), %ebx
	movl	ABS16(bios_ecx), %ecx
	movl	ABS16(bios_edx), %edx
	movl	ABS16(bios_esi), %esi
	movl	ABS16(bios_edi), %edi
	movl	ABS16(bios_ebp), %ebp
	
	iret

	.code16
low_v86_intr:
	int	$0xff
	
	pushfl
	popl	%cs:ABS16(bios_eflags)
	movl	%eax, %cs:ABS16(bios_eax)
	movl	%ebx, %cs:ABS16(bios_ebx)
	movl	%ecx, %cs:ABS16(bios_ecx)
	movl	%edx, %cs:ABS16(bios_edx)
	movl	%esi, %cs:ABS16(bios_esi)
	movl	%edi, %cs:ABS16(bios_edi)
	movl	%ebp, %cs:ABS16(bios_ebp)
	
	int	$0x80
	
	.code32
	
	movl	ABS16(kern_cr0), %eax
	movl	%eax, %cr0
	
	movl	ABS16(kern_cr3), %eax
	movl	%eax, %cr3
	
	movl	ABS16(kern_esp), %esp
	movl	$0xfee1dead, ABS16(kern_esp)
	popfl
	RESTORE_REGS
	
	/* Restore the normal TSS. */
	
	movb	$0x89, tss_desc + 5 /* unbusy the TSS */
	movw	$KERN_TSS, %ax
	ltr	%ax
	
	xorl	%eax, %eax
	popa
	ret

kern_irq_mask:	.long	0x00000000
kern_esp:	.long	0x00000000
kern_cr0:	.long	0x00000000
kern_cr3:	.long	0x00000000
real_irq_mask:	.long	0x00000000

bios_regs:
bios_eax:	.long	0x76543210
bios_ebx:	.long	0x00000000
bios_ecx:	.long	0x00000000
bios_edx:	.long	0x00000000
bios_esi:	.long	0x00000000
bios_edi:	.long	0x00000000
bios_ebp:	.long	0x00000000
bios_ds:	.long	0x00000000
bios_es:	.long	0x00000000
bios_fs:	.long	0x00000000
bios_gs:	.long	0x00000000
bios_eflags:	.long	0x00000000
bios_intr:	.long	0x00000000

	.code16

apm_off_16:
	movw	$KERN16_DS, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss
	
	lidt	ABS16(real_idt_desc)
	
	movl	%cr0, %eax
	andl	$0xfffffffe, %eax
	movl	%eax, %cr0
	
	ljmp	$0, $ABS16(1f)
1:
	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss
	
	movw	$0x0003, %ax
	int	$0x10
	
	movw	$0x0200, %ax
	movw	$0x0000, %bx
	movw	$0x0c1e, %dx
	int	$0x10
	
	cld
	
	movw	$0xb800, %ax
	movw	%ax,	 %es
	xorw	%di,	 %di
	movw	$1999,	 %cx
	movw	$0x1f20, %ax
	rep
	stosw
	movw	$0x1120, %ax
	stosw
	
	movw	$0x0e00, %ax
	movw	$0x0007, %bx
	movw	$ABS16(apm_off_msg), %si
1:	lodsb
	cmpb	$0, %al
	je	1f
	int	$0x10
	jmp	1b
1:
	movw	$0x0200, %ax
	movw	$0x0000, %bx
	movw	$0x184f, %dx
	int	$0x10
	
	movw	$0x5300, %ax
	movw	$0x0000, %bx
	int	$0x15
	jc	ABS16(halt_16)
	cmpw	$0x504d, %bx
	jne	ABS16(halt_16)
	
	movw	$0x5304, %ax
	movw	$0x0000, %bx
	int	$0x15
	
	movw	$0x5301, %ax
	movw	$0x0000, %bx
	int	$0x15
	
	movw	$0x530e, %ax
	movw	$0x0000, %bx
	movw	$0x0102, %cx
	int	$0x15
	
	movw	$0x5308, %ax
	movw	$0x0001, %bx
	movw	$0x0001, %cx
	int	$0x15
	
	movw	$0x5307, %ax
	movw	$0x0001, %bx
	movw	$0x0003, %cx
	int	$0x15
	
halt_16:
	hlt
	jmp	halt_16

apm_off_msg:
	.asciz	"**** POWER DOWN ****\r\n\n"

kern_idt_desc:
	.quad	0

kern_gdt_desc:
	.quad	0

real_idt_desc:
	.word	0x07ff
	.word	0x0000
	.word	0x0000

c16_end:
