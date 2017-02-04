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

#ifndef HDFLAT
#error HDFLAT is not defined
#endif

#include "boot.h"

	.globl _start

	.code16
	.org 0
	jmp	_start

	.org 4
krnl_linear:	.long	2

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
	movw	$msg_wait,%bx
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
	pushw	%ax
	pushw	%bx
bios_puts_loop:
	movb	(%bx),%al
	incw	%bx
	cmpb	$0x00,%al
	jz	bios_puts_end
	call	bios_putc
	jmp	bios_puts_loop
bios_puts_end:
	popw	%bx
	popw	%ax
	ret

load:
	movb	$0x08,%ah		/* get drive parameters */
	movb	$0x80,%dl
	int	$0x13
	jc	error
	
	andb	$0x3f,%cl
	incb	%ch
	incb	%dh
	
	movl	$0,disk_c
	movl	$0,disk_h
	movl	$0,disk_s
	
	movb	%ch,disk_c
	movb	%cl,disk_s
	movb	%dh,disk_h
	
	shrb	$6,%cl
	movb	%cl,disk_c+1
	andb	$0x3f,disk_s
	
	movb	$0x41,%ah
	movw	$0x55aa,%bx
	movb	$0x80,%dl
	int	$0x13
	jc	switch_to_chs
	
	cmp	$0xaa55,%bx
	jne	switch_to_chs
	
1:
	call	load_sect
	addw	$0x0020,load_seg
	decw	krnl_size
	cmpw	$0,krnl_size
	jnz	1b
	ret

switch_to_chs:
	movw	$msg_chs,%bx
	call	bios_puts
	movb	$1,use_chs
	jmp	1b

/* lin2chs
 *
 * convert linear sector offset into CHS (not all disks support LBA)
 *
 * i286 cannot handle this code (32-bit opcodes used)
 *
 * does the same as the following C code:
 *
 * krnl_s = krnl_linear % disk_s;
 * krnl_h = krnl_linear / disk_s % disk_h;
 * krnl_c = krnl_linear / disk_s / disk_h;
 *
 */

lin2chs:
	movl	$0,%edx
	movl	krnl_linear,%eax
	divl	disk_s
	movl	%edx,krnl_s
	movl	$0,%edx
	divl	disk_h
	movl	%edx,krnl_h
	movl	%eax,krnl_c
	ret

/* load_sect
 *
 * load a single disk block, update kern_linear so that the next call
 * will load the next sector
 *
 */

load_sect:
	cmpb	$0,use_chs
	jne	load_sect_chs
	
	movw	load_seg,%ax
	movw	%ax,lba_packet_buf+2
	movl	krnl_linear,%eax
	movl	%eax,lba_packet_block
	
	movb	$0x42,%ah
	movb	$0x80,%dl
	movw	$lba_packet,%si
	int	$0x13
	jc	error
	incl	krnl_linear
	
	incw	cnt
	movw	cnt, %bx
	andw	$3, %bx
	movb	msg_rdash(%bx), %al
	call	bios_putc
	movb	$8, %al
	call	bios_putc
	ret

lba_packet:
	.byte	16
	.byte	0
	.byte	1
	.byte	0
lba_packet_buf:
	.word	0,0
lba_packet_block:
	.quad	0

load_sect_chs:
	call	lin2chs
	
	movw	$0,%bx
	movw	load_seg,%ax
	movw	%ax,%es
	movb	krnl_c,%ch
	movb	krnl_s,%cl
	incb	%cl
	mov	krnl_c+1,%al
	shl	$6,%al
	orb	%al,%cl
	movb	krnl_h,%dh
	movb	$0x80,%dl
	movb	$2,%ah
	movb	$1,%al
	int	$0x13
	jc	error
	
	incl	krnl_linear
	ret

error:
	mov	$msg_error,%bx
	call	bios_puts
halt:
	jmp	halt

msg_rdash:	.ascii	"|/-\\"
msg_wait:	.asciz	"Loading ... "
msg_error:	.asciz	"\r\n\r\nError!\r\n"
msg_chs:	.asciz	"CHS!\r\n"

cnt:		.word	0
disk_c:		.long	0
disk_h:		.long	0
disk_s:		.long	0

krnl_c:		.long	0
krnl_h:		.long	0
krnl_s:		.long	0
krnl_size:	.long	128

load_seg:	.long	LOAD_SEG

use_chs:	.byte	0

	.org	0x1ba

#if HDFLAT
	.org	0x1be
	.byte	0x80,	1,	1,	0
	.byte	0xcc,	15,	32,	31
	.long	1,	16383
#endif

	.org	0x1fe
	.word	0xaa55
