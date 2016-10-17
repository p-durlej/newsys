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

#ifndef _SYSLOAD_I386_PC_H
#define _SYSLOAD_I386_PC_H

#include <stdint.h>

extern struct bioscall
{
	uint32_t eflags;	/* 0*/
	uint32_t eax;		/* 4 */
	uint32_t ebx;		/* 8 */
	uint32_t ecx;		/* 12 */
	uint32_t edx;		/* 16 */
	uint32_t esi;		/* 20 */
	uint32_t edi;		/* 24 */
	uint32_t ebp;		/* 28 */
	uint16_t ds;		/* 32 */
	uint16_t es;		/* 34 */
	uint16_t fs;		/* 36 */
	uint16_t gs;		/* 38 */
	union			/* 40 */
	{
		uint32_t addr;
		uint8_t	 intr;
	};
} bcp;

extern struct farcall
{
	uint32_t	eax;		/* 0 */
	uint32_t	edx;		/* 4 */
	uint32_t	eflags;		/* 8 */
	uint32_t	addr;		/* 12 */
	uint16_t	stack[4];	/* 16 */
} fcp;

extern uint32_t	conv_mem_size;
extern uint32_t	conv_mem_hbrk;
extern uint32_t	conv_mem_lbrk;
extern uint32_t	bangpxe;
extern uint32_t	pm_esp;
extern uint8_t	bbd;
extern uint32_t	cdrom;

extern char bounce[2048];

void bioscall(void);
void farcall(void);

struct kfb *vga_init(void);
void pxe_init(void);

#endif
