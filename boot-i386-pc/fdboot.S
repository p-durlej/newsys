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

#include "boot.h"

	.globl	_start

	.code16
	.org 0

_start:
	movw	$LDR_SEG,%ax
	movw	%ax,%ds
	movw	%ax,%es
	cli
	movw	$0,%ax
	movw	%ax,%ss
	movw	$LDR_BASE,%sp
	sti
	pushw	%ds
	pushw	$boot
	lret

boot:
	movw	$msg_wait, %si
	call	bios_puts
	call	load
	push	$KRNL_CS
	push	$KRNL_IP
	lret
	
bios_putc:
	pushw	%ax
	pushw	%bx
	movb	$0x0e,%ah
	movw	$0x0007,%bx
	int	$0x10
	popw	%bx
	popw	%ax
	ret
	
bios_puts:
	movw	$0x0007, %bx
bios_puts_loop:
	lodsb
	cmpb	$0x00,%al
	jz	bios_puts_end
	movb	$0x0e, %ah
	int	$0x10
	jmp	bios_puts_loop
bios_puts_end:
	ret

load:
1:
	call	load_sect
	addw	$0x0020,load_seg
	decw	kern_size
	cmpw	$0,kern_size
	jnz	1b

/* load_sect
 * 
 * load single disk block, update kern_c, kern_h, kern_s so that the next call
 * will load the next sector
 *
 */

load_sect:
	movw	$0,%bx
	movw	load_seg,%ax
	movw	%ax,%es
	movb	kern_c,%ch
	movb	kern_s,%cl
	incb	%cl
	movb	kern_h,%dh
	movb	disk,%dl
	movb	$2,%ah
	movb	$1,%al
	int	$0x13
	jc	error
	
	movw	disk_s,%ax
	incw	kern_s
	cmpw	%ax,kern_s
	jl	load_sect_end
	
	movw	$0,kern_s
	incw	kern_h
	movw	disk_h,%ax
	cmpw	%ax,kern_h
	jl	load_sect_end
	
	movw	$0,kern_h
	incw	kern_c
	
load_sect_end:
	/* mov	$0x2e,%al */
	incw	cnt
	movw	cnt, %bx
	andw	$3, %bx
	movb	msg_rdash(%bx), %al
	call	bios_putc
	movb	$8, %al
	call	bios_putc
	ret
	
dp_error:
	mov	$msg_dp_error, %si
	call	bios_puts
	jmp	halt

error:
	mov	$msg_error, %si
	call	bios_puts
halt:	movw	$msg_halt, %si
	call	bios_puts
1:	hlt
	jmp	1b

cnt:	.word	0

msg_rdash:	.ascii	"|/-\\"
msg_wait:	.asciz	"Loading ... "
msg_dp_error:	.asciz	"\r\n\r\nINT 13/08 error!\r\n"
msg_error:	.asciz	"\r\n\r\nI/O error!\r\n"
msg_halt:	.asciz	"System halted.\r\n"

disk:
		.long	0
disk_c:		.long	80
disk_h:		.long	2
disk_s:		.long	18

kern_c:		.long	0
kern_h:		.long	0
kern_s:		.long	2
kern_size:	.long	128

load_seg:	.long	LOAD_SEG

	.org	0x1fe
	.word	0xaa55
