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

.globl setjmp
setjmp:
	movl 4(%esp),%eax
	movl %ebx,0(%eax)
	movl %ecx,4(%eax)
	movl %edx,8(%eax)
	movl %esi,12(%eax)
	movl %edi,16(%eax)
	movl %ebp,20(%eax)
	movl %esp,24(%eax)
	movl (%esp),%edx
	movl %edx,28(%eax)
	movl $0,%eax
	ret

.globl longjmp
longjmp:
	movl 8(%esp),%eax
	movl 4(%esp),%ebp
	movl 24(%ebp),%esp
	movl 28(%ebp),%edx
	movl %edx,(%esp)
	movl 0(%ebp),%ebx
	movl 4(%ebp),%ecx
	movl 8(%ebp),%edx
	movl 12(%ebp),%esi
	movl 16(%ebp),%edi
	movl 20(%ebp),%ebp
	ret
