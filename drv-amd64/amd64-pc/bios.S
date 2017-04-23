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
#include <kern/arch/intr_regs.h>
#include <kern/page.h>

#define ABS16(v)	((v) - c16_start + CODE_BASE)

#define PANIC_BUF	0x08000
#define CODE_BASE	0x09000
#define STK_TOP		0x0fff0

	.globl	do_panic
	.globl	apm_off
	.globl	halt

	.code64

do_panic:
	xchgw	%bx, %bx
	movq	%rdi, %rsi
	movq	$PANIC_BUF, %rdi
1:	lodsb
	stosb
	testb	%al, %al
	jnz	1b
	
	movw	$ABS16(real_panic), halt_proc
	jmp	do_halt

apm_off_idt:
	.word	1023
	.quad	0

apm_off:
	movw	$ABS16(real_apm_off), halt_proc
	jmp	do_halt

halt:
	movw	$ABS16(real_halt), halt_proc
	jmp	do_halt

do_halt:
	cli
	cld
	movq	$c16_start, %rsi
	movq	$CODE_BASE, %rdi
	movq	$c16_end - c16_start, %rcx
	rep
	movsb
	
	lidtq	apm_off_idt
	ljmpw	*1f
1:	.word	CODE_BASE
	.word	KERN16_CS
	
	.code16
c16_start:
	movw	$KERN16_DS, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss
	movl	%cr0, %eax
	andl	$0x7ffffffe, %eax
	movl	%eax, %cr0
	ljmp	$0, $ABS16(1f)
1:	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss
	movw	$STK_TOP, %sp
	
	jmp	*(ABS16(halt_proc))
real_apm_off:
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
	jc	halt_16
	cmpw	$0x504d, %bx
	jne	halt_16
	
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
	.asciz	"**** POWER DOWN ****"

real_panic:
	movw	$0x0003, %ax
	int	$0x10
	movw	$0x0e00, %ax
	movw	$0x0007, %bx
	movw	$ABS16(panic_msg), %si
1:	lodsb
	cmpb	$0, %al
	je	1f
	int	$0x10
	jmp	1b
1:	movw	$PANIC_BUF, %si
1:	lodsb
	cmpb	$0, %al
	je	1f
	int	$0x10
	jmp	1b
1:	movb	$'\r', %al
	int	$0x10
	movb	$'\n', %al
	int	$0x10
1:	hlt
	jmp	1b

panic_msg:
	.asciz	"panic: "

real_halt:
	movw	$0x0003, %ax
	int	$0x10
	call	blue_bg
	movw	$0x0200, %ax
	movw	$0x0000, %bx
	movw	$0x0c1c, %dx
	int	$0x10
	movw	$0x0e00, %ax
	movw	$0x0007, %bx
	movw	$ABS16(halt_msg), %si
1:	lodsb
	cmpb	$0, %al
	je	1f
	int	$0x10
	jmp	1b
1:	call	blue_bg_hc
1:	hlt
	jmp	1b

halt_msg:
	.asciz	"**** SYSTEM HALTED ****"
	.byte	0

halt_proc:
	.word	ABS16(real_apm_off)

blue_bg:
	movw	$0xb800, %ax
	movw	%ax,	 %es
	xorw	%di,	 %di
	movw	$1999,	 %cx
	movw	$0x1f20, %ax
	rep
	stosw
	movw	$0x1120, %ax
	stosw
	ret

blue_bg_hc:
	movw	$0x0200, %ax
	movw	$0x0000, %bx
	movw	$0x184f, %dx
	int	$0x10
	ret

c16_end:
