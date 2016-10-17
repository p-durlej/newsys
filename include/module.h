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

#ifndef _MODULE_H
#define _MODULE_H

#include <arch/archdef.h>
#include <stdint.h>

#if defined __ARCH_I386__ || defined __ARCH_AMD64__
#else
#error Unknown arch
#endif

#define MH_MAGIC	0
#define MH_REL_START	8
#define MH_REL_COUNT	12
#define MH_SYM_START	16
#define MH_SYM_COUNT	20
#define MH_CODE_START	24
#define MH_CODE_SIZE	28

#define MR_T_NORMAL	0
#define MR_T_ABS	1

#define SYM_EXPORT	0x0001
#define SYM_IMPORT	0x0002

#ifndef __ASSEMBLER__

struct modhead
{
	char	 magic[8];
	unsigned rel_start;
	unsigned rel_count;
	unsigned sym_start;
	unsigned sym_count;
	unsigned got_start;
	unsigned got_count;
	unsigned code_start;
	unsigned code_size;
};

struct modsym
{
	unsigned short flags;
	unsigned char  shift;
	unsigned char  size;
	unsigned       addr;
	unsigned       name_len;
	char	       name[32];
};

struct modrel
{
	unsigned char shift;
	unsigned char size;
	unsigned char type;
	unsigned      addr;
};

#endif

#endif
