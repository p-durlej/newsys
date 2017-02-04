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

#ifndef _PRIV_ELF_H
#define _PRIV_ELF_H

#include <arch/archdef.h>

#define ELF32_MAGIC		"\177ELF\x01\x01\x01"
#define ELF64_MAGIC		"\177ELF\x02\x01\x01"

#define ELF_TYPE_RELOC		1
#define ELF_TYPE_EXEC		2
#define ELF_TYPE_SHLIB		3
#define ELF_TYPE_CORE		4

#define ELF_MACH_I386		3
#define ELF_MACH_AMD64		62

#define ELF_VERSION		1

#define ELF_PT_LOAD		1
#define ELF_PT_DYNAMIC		2

#define ELF_DT_REL		17
#define ELF_DT_RELSZ		18
#define ELF_DT_RELENT		19

#define ELF_SHT_PROGBITS	1
#define ELF_SHT_SYMTAB		2
#define ELF_SHT_STRTAB		3
#define ELF_SHT_RELA		4
#define ELF_SHT_NOBITS		8
#define ELF_SHT_REL		9

#define ELF_SHF_WRITE		1
#define ELF_SHF_ALLOC		2
#define ELF_SHF_EXEC		4

#define ELF_REL_386_32		1
#define ELF_REL_386_PC32	2
#define ELF_REL_386_GOT32	3
#define ELF_REL_386_PLT32	4
#define ELF_REL_386_COPY	5
#define ELF_REL_386_GLOB_DAT	6
#define ELF_REL_386_JMP_SLOT	7
#define ELF_REL_386_RELATIVE	8
#define ELF_REL_386_GOTOFF	9
#define ELF_REL_386_GOTPC	10

#define ELF_REL_386_16		20 /* XXX undocumented */
#define ELF_REL_386_PC16	21 /* XXX undocumented */

#define ELF_REL_AMD64_64	1
#define ELF_REL_AMD64_PC32	2
#define ELF_REL_AMD64_GOT32	3
#define ELF_REL_AMD64_PLT32	4
#define ELF_REL_AMD64_COPY	5
#define ELF_REL_AMD64_GLOB_DAT	6
#define ELF_REL_AMD64_JMP_SLOT	7
#define ELF_REL_AMD64_RELATIVE	8
#define ELF_REL_AMD64_GOTPCREL	9
#define ELF_REL_AMD64_32	10
#define ELF_REL_AMD64_32S	11
#define ELF_REL_AMD64_16	12
#define ELF_REL_AMD64_PC16	13
#define ELF_REL_AMD64_8		14
#define ELF_REL_AMD64_PC8	15
#define ELF_REL_AMD64_PC64	24
#define ELF_REL_AMD64_GOTOFF64	25
#define ELF_REL_AMD64_GOTPC32	26
#define ELF_REL_AMD64_SIZE32	32
#define ELF_REL_AMD64_SIZE64	33

#define ELF_STB_LOCAL		0
#define ELF_STB_GLOBAL		1
#define ELF_STB_WEAK		2

#define ELF_SHN_UNDEF		0
#define ELF_SHN_ABS		0xfff1
#define ELF_SHN_COMMON		0xfff2

#define ELF_STT_NOTYPE		0
#define ELF_STT_OBJECT		1
#define ELF_STT_FUNC		2

#define ELF_STN_UNDEF		0

#include <stdint.h>

struct elf32_head
{
	uint8_t		magic[16];
	uint16_t	type;
	uint16_t	mach;
	uint32_t	version;
	uint32_t	start;
	uint32_t	ph_offset;
	uint32_t	sh_offset;
	uint32_t	flags;
	uint16_t	eh_size;
	uint16_t	ph_elem_size;
	uint16_t	ph_elem_cnt;
	uint16_t	sh_elem_size;
	uint16_t	sh_elem_cnt;
	uint16_t	sh_string_idx;
};

struct elf64_head
{
	uint8_t		magic[16];
	uint16_t	type;
	uint16_t	mach;
	uint32_t	version;
	uint64_t	start;
	uint64_t	ph_offset;
	uint64_t	sh_offset;
	uint32_t	flags;
	uint16_t	eh_size;
	uint16_t	ph_elem_size;
	uint16_t	ph_elem_cnt;
	uint16_t	sh_elem_size;
	uint16_t	sh_elem_cnt;
	uint16_t	sh_string_idx;
};

struct elf32_prog_head
{
	uint32_t	type;
	uint32_t	offset;
	uint32_t	v_addr;
	uint32_t	p_addr;
	uint32_t	file_size;
	uint32_t	mem_size;
	uint32_t	flags;
	uint32_t	align;
};

struct elf64_prog_head
{
	uint32_t	type;
	uint32_t	flags;
	uint64_t	offset;
	uint64_t	v_addr;
	uint64_t	p_addr;
	uint64_t	file_size;
	uint64_t	mem_size;
	uint64_t	align;
};

struct elf32_sect_head
{
	uint32_t	name;
	uint32_t	type;
	uint32_t	flags;
	uint32_t	addr;
	uint32_t	offset;
	uint32_t	size;
	uint32_t	link;
	uint32_t	info;
	uint32_t	align;
	uint32_t	elem_size;
};

struct elf64_sect_head
{
	uint32_t	name;
	uint32_t	type;
	uint64_t	flags;
	uint64_t	addr;
	uint64_t	offset;
	uint64_t	size;
	uint32_t	link;
	uint32_t	info;
	uint64_t	align;
	uint64_t	elem_size;
};

struct elf_dyn
{
	uint32_t	tag;
	uint32_t	val;
};

struct elf32_rela
{
	uint32_t	offset;
	uint32_t	info;
	uint32_t	addend;
};

struct elf64_rela
{
	uint64_t	offset;
	uint64_t	info;
	uint64_t	addend;
};

struct elf32_rel
{
	uint32_t	offset;
	uint32_t	info;
};

struct elf64_rel
{
	uint64_t	offset;
	uint64_t	info;
};

struct elf32_sym
{
	uint32_t	name;
	uint32_t	value;
	uint32_t	size;
	uint8_t		info;
	uint8_t		other;
	uint16_t	sect;
};

struct elf64_sym
{
	uint32_t	name;
	uint8_t		info;
	uint8_t		other;
	uint16_t	sect;
	uint64_t	value;
	uint64_t	size;
};

#endif
