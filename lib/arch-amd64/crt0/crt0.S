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

#include <sysver.h>

	.text
	
	.globl	_base
	.globl	_start

	.globl	errno
	.globl	environ
	.globl	stdin
	.globl	stdout
	.globl	stderr
	.globl	libc_entry

_base:
	.ascii	"\0X64"
	
	.long	1,0
	.long	0
	
	.long	_base
	.long	_start
	.long	_end
	.long	0
	.long	0

_start:
	movq	%rcx, libc_entry
	
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	
	movq	$environ, %rdi
	call	_set_environ_ptr
	
	movq	$errno, %rdi
	call	_set_errno_ptr
	
	call	_get_stdin
	movq	%rax, stdin
	
	call	_get_stdout
	movq	%rax, stdout
	
	call	_get_stderr
	movq	%rax, stderr
	
	movq	$SYSVER_MAJOR, %rdi
	movq	$SYSVER_MINOR, %rsi
	callq	_minver
	
	popq	%rdx
	popq	%rsi
	popq	%rdi
	
	jmp	main

	.data
	.balign	8

environ:	.quad	0
stdin:		.quad	0
stdout:		.quad	0
stderr:		.quad	0
libc_entry:	.quad	0
errno:		.long	0
