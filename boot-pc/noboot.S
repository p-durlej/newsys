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
	movw	$msg, %si
	call	bios_puts
1:	hlt
	jmp	1b
	
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

msg:	.ascii	"\r\nNot a boot disk.\r\nSystem halted.\r\n"

	.org	0x1fe
	.word	0xaa55
