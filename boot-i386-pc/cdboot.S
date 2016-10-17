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

	.globl	_start

	.code16
	.org	2048

_start:
	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	$msg + 0x7800, %si
	cld
	
loop:
	lodsb
	cmp	$0, %al
	je	halt
	movb	$0x0e, %ah
	movw	$0x0007, %bx
	int	$0x10
	jmp	loop
halt:
	hlt
	jmp	halt
	
msg:
	.asciz	"\r\ncdboot: Missing sysload\r\n"
	
	.org	0x11 * 2048
	
	/* Boot Record */
	
	.byte	0				/* Boot Record Indicator */
	.ascii	"CD001"				/* ISO-9660 Identifier */
	.byte	1				/* Version of this descriptor */
	.ascii	"EL TORITO SPECIFICATION"
	.space	0x29
	.long	0x12

	.org	0x12 * 2048
	
	/* Validation Entry */
	
	.byte	1				/* Header ID */
	.byte	0				/* Platform ID */
	.word	0
	.ascii	"                        "	/* ID string */
	.word	0				/* Checksum Word */
	.word	0xaa55
	
	/* Initial/Default Entry */
	
	.byte	0x88
	.byte	0				/* Media type: no emulation */
	.word	0x800				/* Load Segment */
	.byte	0xcc				/* System Type */
	.byte	0
	.word	64				/* Sector count */
	.long	1				/* Load RBA */
	.space	20
	
	.org	0x13 * 2048
