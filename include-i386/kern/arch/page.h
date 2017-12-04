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

#define PAGE_VMASK		0xfffff000
#define PAGE_PMASK		0x00000fff
#define PAGE_SIZE		4096
#define PAGE_SHIFT		12

#define PAGE_DIR		2
#define PAGE_ITAB		3 /* four pages */
#define PAGE_TMP		7
#define PAGE_TMP_COUNT		8

#define PAGE_IDENT		0
#define PAGE_IDENT_END		PAGE_TAB
#define PAGE_IDENT_COUNT	(PAGE_IDENT_END - PAGE_IDENT)
#define PAGE_TAB		0x00001000
#define PAGE_DYN		0x00001400
#define PAGE_DYN_COUNT		(PAGE_USER - PAGE_DYN)
#define PAGE_USER		0x00040000
#define PAGE_USER_END		0x00100000
#define PAGE_LDR		0x0007fc00
#define PAGE_LDR_END		0x00080000
#define PAGE_IMAGE		0x00040000	/* XXX belongs to userspace */
#define PAGE_IMAGE_END		0x0007f800	/* XXX belongs to userspace */
#define PAGE_LIBX		0x0007f800
#define PAGE_LIBX_END		0x0007fc00
#define PAGE_HEAP		0x00080000	/* XXX belongs to userspace */
#define PAGE_HEAP_END		0x000ff800	/* XXX belongs to userspace */
#define PAGE_STACK		0x000ff800
#define PAGE_STACK_END		0x000fffff

#define PAGE_USER_COUNT		(PAGE_USER_END  - PAGE_USER)
#define PAGE_STACK_COUNT	(PAGE_STACK_END - PAGE_STACK)

#define PGF_PRESENT		1
#define PGF_WRITABLE		2
#define PGF_USER		4
#define PGF_PWT			8
#define PGF_PCD			16
#define PGF_ACCESSED		32
#define PGF_DIRTY		64

#define PGF_PHYS		512
#define PGF_SPACE_IN_USE	1024
#define PGF_AUTO		1024
#define PGF_COW			2048

#define PGF_KERN		(PGF_PRESENT | PGF_WRITABLE | PGF_SPACE_IN_USE)
#define PGF_KERN_RO		(PGF_PRESENT | PGF_SPACE_IN_USE)

#if PAGE_IDENT % 1024
#error PAGE_IDENT % 1024 != 0
#endif

#if PAGE_TAB % 1024
#error PAGE_TAB % 1024 != 0
#endif

#if PAGE_DYN % 1024
#error PAGE_DYN % 1024 != 0
#endif

#if PAGE_DYN_COUNT % 1024
#error PAGE_DYN_COUNT % 1024 != 0
#endif

#if PAGE_USER % 1024
#error PAGE_USER % 1024 != 0
#endif

#if PAGE_USER_END % 1024
#error PAGE_USER_END % 1024 != 0
#endif

#ifndef __ASSEMBLER__

typedef unsigned vpage;
typedef unsigned ppage;

extern vpage *pg_dir;
extern vpage *pg_tab;

#endif
