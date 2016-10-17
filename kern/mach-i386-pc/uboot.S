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
#include <kern/page.h>
#include <priv/exec.h>

	.globl	uboot
	.globl	uexec

uboot:
	movl	curr,%eax
	addl	$8192,%eax // XXX wrong stack size
	movl	%eax,tss+4
	movl	%eax,%esp
	
	movw	$USER_DS,%dx
	movw	%dx,%ds
	movw	%dx,%es
	movw	%dx,%fs
	movw	%dx,%gs
	pushl	$USER_DS
	pushl	$PAGE_STACK_END << 12 - 32
	pushl	$USER_CS
	pushl	$LDR_BASE + LDR_HEAD_SIZE
	movl	$0x00000000,%eax
	movl	$0x00000000,%ebx
	movl	$0x00000000,%ecx
	movl	$0x00000000,%edx
	movl	$0x00000000,%esi
	movl	$0x00000000,%edi
	movl	$0x00000000,%ebp
	lret

uexec:
	movl	ureg,%eax
	movl	%eax,%esp
	cli
	popl	%gs
	popl	%fs
	popl	%es
	popl	%ds
	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	iret
