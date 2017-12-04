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

#define MSR_ABA		0x0000001b

#define MSR_EFER	0xc0000080
#define   MSR_EFER_LME	(1 <<  8)
#define   MSR_EFER_LMA	(1 << 10)
#define   MSR_EFER_NXE	(1 << 11)
#define   MSR_EFER_SVME	(1 << 12)
#define MSR_VM_CR	0xc0010114
#define MSR_IGNNE	0xc0010115
#define MSR_SMM_CTL	0xc0010116
#define MSR_HSAVE_PA	0xc0010117

#define CR0_PE		 1
#define CR0_EM		(1 <<  2)
#define CR0_TS		(1 <<  3)
#define CR0_NE		(1 <<  5)
#define CR0_NW		(1 << 29)
#define CR0_CD		(1 << 30)
#define CR0_PG		(1 << 31)

#define CR4_PSE		(1 << 4)
#define CR4_PAE		(1 << 5)
#define CR4_OSFXSR	(1 << 9)
