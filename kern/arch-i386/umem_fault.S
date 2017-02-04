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

	.globl	fault_jmp_set
	
	.globl	fault
	.globl	fault_fini
	.globl	fault_abort
	
	.data
fault_jmp_set:		.long	0
fault_jmp_ebx:		.long	0
fault_jmp_ecx:		.long	0
fault_jmp_edx:		.long	0
fault_jmp_esi:		.long	0
fault_jmp_edi:		.long	0
fault_jmp_ebp:		.long	0
fault_jmp_esp:		.long	0
fault_jmp_eflags:	.long	0
fault_jmp_eip:		.long	0

	.text
fault:
	cmpb	$0x00,fault_jmp_set
	jne	fault_panic
	movb	$0x01,fault_jmp_set
	
	movl	%ebx,fault_jmp_ebx
	movl	%ecx,fault_jmp_ecx
	movl	%edx,fault_jmp_edx
	movl	%esi,fault_jmp_esi
	movl	%edi,fault_jmp_edi
	movl	%ebp,fault_jmp_ebp
	movl	%esp,fault_jmp_esp
	pushfl
	popl	fault_jmp_eflags
	movl	(%esp),%eax
	movl	%eax,fault_jmp_eip
	
	movl	$0x00000000,%eax
	ret
fault_panic:
	pushl	$fault_msg
	call	panic
1:
	jmp	1b

fault_fini:
	cmpb	$0x01,fault_jmp_set
	jne	fault_fini_panic
	movb	$0x00,fault_jmp_set
	ret
fault_fini_panic:
	pushl	$fault_fini_msg
	call	panic
1:
	jmp	1b

fault_abort:
	cmpb	$0x01,fault_jmp_set
	jne	fault_fini_panic
	movb	$0x00,fault_jmp_set
	
	movl	fault_jmp_ebx,%ebx
	movl	fault_jmp_ecx,%ecx
	movl	fault_jmp_edx,%edx
	movl	fault_jmp_esi,%esi
	movl	fault_jmp_edi,%edi
	movl	fault_jmp_ebp,%ebp
	movl	fault_jmp_esp,%esp
	pushl	fault_jmp_eflags
	popfl
	movl	fault_jmp_eip,%eax
	movl	%eax,(%esp)
	
	movl	$0x00000001,%eax
	ret
fault_abort_panic:
	pushl	$fault_abort_msg
	call	panic
1:
	jmp	1b

fault_msg:		.asciz	"fault.S: fault: fault longjmp already set"
fault_fini_msg:		.asciz	"fault.S: fault_fini: fault longjmp not set"
fault_abort_msg:	.asciz	"fault.S: fault_abort: fault longjmp not set"
