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

#include <priv/elf.h>
#include <arch/elf.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "elo.h"

#define DEBUG	0

#if DEBUG
#define elo_debug	printf
#else
#define elo_debug(x, ...)
#endif

static void load_seg_abs(struct elo_data *ed, struct elf_prog_head *phd)
{
	unsigned start;
	unsigned end;
	
	elo_debug("load_seg_abs: phd->offset    = 0x%08lx\n", (long)phd->offset);
	elo_debug("              phd->v_addr    = 0x%08lx\n", (long)phd->v_addr);
	elo_debug("              phd->file_size = 0x%08lx\n", (long)phd->file_size);
	elo_debug("              phd->mem_size  = 0x%08lx\n", (long)phd->mem_size);
	
	start =  phd->v_addr >> ELO_PAGE_SHIFT;
	end   = (phd->v_addr + phd->mem_size + ELO_PAGE_SIZE - 1) >> ELO_PAGE_SHIFT;
	
	if (elo_pgalloc(start, end))
		elo_fail("unable to allocate memory");
	
	if (elo_seek(ed->fd, phd->offset))
		elo_fail("seek failed on program image");
	
	if (elo_read(ed->fd, (void *)phd->v_addr, phd->file_size))
		elo_fail("unable to load segment");
	
	elo_debug("              done\n");
}

static void load_seg_rel(struct elo_data *ed, struct elf_prog_head *phd)
{
	elo_debug("load_seg_rel: phd->offset    = 0x%08lx\n", (long)phd->offset);
	elo_debug("              phd->v_addr    = 0x%08lx\n", (long)phd->v_addr);
	elo_debug("              phd->file_size = 0x%08lx\n", (long)phd->file_size);
	elo_debug("              phd->mem_size  = 0x%08lx\n", (long)phd->mem_size);
	
	if (elo_seek(ed->fd, phd->offset))
		elo_fail("seek failed on program image");
	
	if (elo_read(ed->fd, ed->image + phd->v_addr - ed->base, phd->file_size))
		elo_fail("unable to load segment");
	
	elo_debug("              done\n");
}

static void load_seg(struct elo_data *ed, struct elf_prog_head *phd)
{
	if (ed->absolute)
		load_seg_abs(ed, phd);
	else
		load_seg_rel(ed, phd);
}

static void sync_seg(struct elo_data *ed, struct elf_prog_head *phd)
{
	if (ed->absolute)
		elo_csync((void *)phd->v_addr, phd->mem_size);
	else
		elo_csync(ed->image + phd->v_addr - ed->base, phd->mem_size);
}

static void do_rel(struct elo_data *ed, struct elf_rel *rel, size_t relsz, size_t relent)
{
	struct elf_rel *end;
	struct elf_rel *p;
	elo_caddr_t *addr;
	
	if (relent != sizeof *p)
		elo_fail("invalid relocation entry size");
	
	end = (struct elf_rel *)((char *)rel + relsz);
	for (p = rel; p < end; p++)
	{
		if (ed->absolute)
			addr = (elo_caddr_t *)p->offset;
		else
			addr = (elo_caddr_t *)(ed->image + p->offset - ed->base);
		
		if (p->info >> 8)
		{
			elo_mesg(ed->pathname);
			elo_mesg(": do_rel: image contains unresolved symbols\n");
		}
		elo_reloc(ed, addr, p->info & 255);
	}
}

static void reloc_elf(struct elo_data *ed, off_t dyn_off, off_t dyn_size)
{
	struct elf_rel *rel = NULL;
	size_t relent = 0;
	size_t relsz  = 0;
	struct elf_dyn *dyn_end;
	struct elf_dyn *dyn;
	struct elf_dyn *p;
	
	elo_debug("reloc_elf: dyn_off  = %li\n", (long)dyn_off);
	elo_debug("           dyn_size = %li\n", (long)dyn_size);
	
	/* if (dyn_size % sizeof(*dyn))
		elo_fail("incorrect dynamic section size"); */
	
	dyn = elo_malloc(dyn_size);
	if (!dyn)
		elo_fail("unable to allocate memory");
	
	if (elo_seek(ed->fd, dyn_off))
		elo_fail("seek failed on program image");
	
	if (elo_read(ed->fd, dyn, dyn_size))
		elo_fail("unable to load the dynamic segment");	
	
	dyn_end = dyn + dyn_size / sizeof *dyn;
	for (p = dyn; p < dyn_end; p++)
		switch (p->tag)
		{
		case 0:
			goto brk_2;
		case ELF_DT_REL:
			if (ed->absolute)
				rel = (struct elf_rel *)ed->image; /* XXX */
			else
				rel = (struct elf_rel *)
					(ed->image + p->val - ed->base);
			break;
		case ELF_DT_RELENT:
			relent = p->val;
			break;
		case ELF_DT_RELSZ:
			relsz = p->val;
			break;
		default:
			;
		}
brk_2:
	elo_debug("reloc_elf: rel    = %p\n",  rel);
	elo_debug("           relent = %li\n", (long)relent);
	elo_debug("           relsz  = %li\n", (long)relsz);
	
	if (rel && relsz)
		do_rel(ed, rel, relsz, relent);
	elo_free(dyn);
	elo_debug("reloc_elf: fini\n");
}

void elo_load(struct elo_data *ed)
{
	struct elf_prog_head phd;
	struct elf_head hd;
	off_t dyn_size = 0;
	off_t dyn_off = 0;
	off_t off;
	int cnt;
	int i;
	
	if (elo_read(ed->fd, &hd, sizeof hd))
		elo_fail("can't read ELF header");
	
	if (memcmp(hd.magic, ELF_MAGIC, sizeof ELF_MAGIC - 1))
		elo_fail("bad magic");
	
	if (hd.version != ELF_VERSION)
		elo_fail("unsupported ELF version");
	if (hd.type != ELF_TYPE_EXEC && hd.type != ELF_TYPE_SHLIB) /* XXX */
		elo_fail("not a program");
	elo_check(&hd);
	
	ed->base = 0xffffffff; /* XXX */
	ed->end  = 0;
	for (i = 0, off = hd.ph_offset; i < hd.ph_elem_cnt; i++, off += hd.ph_elem_size)
	{
		if (elo_seek(ed->fd, off))
			elo_fail("seek failed");
		
		if (elo_read(ed->fd, &phd, sizeof phd))
			elo_fail("can't read program header");
		
		switch (phd.type)
		{
		case ELF_PT_LOAD:
		{
			unsigned seg_end;
			
			seg_end = phd.v_addr + phd.mem_size;
			if (seg_end > ed->end)
				ed->end = seg_end;
			if (ed->base > phd.v_addr)
				ed->base = phd.v_addr;
			
			if (ed->absolute)
				load_seg(ed, &phd);
		}
		case ELF_PT_DYNAMIC:
			dyn_size = phd.file_size;
			dyn_off	 = phd.offset;
			break;
		default:
			;
		}
	}
	
	if (!ed->absolute)
	{
		ed->image = elo_malloc(ed->end - ed->base);
		if (!ed->image)
			elo_fail("unable to allocate memory");
		memset(ed->image, 0, ed->end - ed->base);
		
		for (i = 0, off = hd.ph_offset; i < hd.ph_elem_cnt; i++, off += hd.ph_elem_size)
		{
			if (elo_seek(ed->fd, off))
				elo_fail("seek failed");
			
			if (elo_read(ed->fd, &phd, sizeof phd))
				elo_fail("can't read program header");
			
			if (phd.type == ELF_PT_LOAD)
				load_seg(ed, &phd);
		}
	}
	
	if (dyn_size)
		reloc_elf(ed, dyn_off, dyn_size);
	
	for (i = 0, off = hd.ph_offset; i < hd.ph_elem_cnt; i++, off += hd.ph_elem_size)
	{
		if (elo_seek(ed->fd, off))
			elo_fail("seek failed");
		
		if (elo_read(ed->fd, &phd, sizeof phd))
			elo_fail("can't read program header");
		
		if (phd.type == ELF_PT_LOAD)
			sync_seg(ed, &phd);
	}
	
	if (ed->absolute)
		ed->entry = hd.start;
	else
		ed->entry = (elo_caddr_t)(ed->image + hd.start - ed->base);
}
