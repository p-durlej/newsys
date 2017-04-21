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

#ifndef _KERN_MACHINE_I386_PC_BIOSCALL_H
#define _KERN_MACHINE_I386_PC_BIOSCALL_H

#ifndef NULL
#define NULL (void *)0
#endif

struct real_regs
{
	union { unsigned eax; unsigned short ax; };
	union { unsigned ebx; unsigned short bx; };
	union { unsigned ecx; unsigned short cx; };
	union { unsigned edx; unsigned short dx; };
	union { unsigned esi; unsigned short si; };
	union { unsigned edi; unsigned short di; };
	union { unsigned ebp; unsigned short bp; };
	
	unsigned ds;
	unsigned es;
	unsigned fs;
	unsigned gs;
	
	union { unsigned eflags; unsigned short flags; };
	unsigned intr;
} __attribute__((packed));

void v86_call(void);

struct real_regs *	get_bios_regs(void);
unsigned	  	get_bios_buf_size(void);
void *			get_bios_buf(void);

#endif
