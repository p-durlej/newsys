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

#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <err.h>

#include "module.h"
#include "elf.h"

#define VERBOSE			0
#define DEBUG			0

typedef uint64_t		symval; /* XXX */

static char *			_progname;

static char *			mod_name;
static char *			obj_name;
static char *			exp_name;

static int			exp_cnt;
static char **			exp;

static symval			got_off;
static symval			got_cnt;

static struct modsym *		mod_sym;
static int			mod_nsym;
static struct modrel *		mod_rel;
static int			mod_nrel;

static void **			obj_sect_image_c;
static void **			obj_sect_image;
static struct elf64_sect_head *	obj_sect;
static struct elf64_head	obj_head64;
static struct elf32_head	obj_head32;
static int			obj_sect0 = -1;
static int			obj_elf64;

static int			obj_fd;

static size_t			image_size;
static uint8_t *		image;

static int			kern_flag;

static void *x_malloc(size_t sz)
{
	void *p;

	p = malloc(sz);
	if (!p)
		err(1, "malloc: %lu bytes", (long)sz);
	memset(p, 0, sz);
	return p;
}

static void *x_realloc(void *p, size_t sz)
{
	void *n;

	n = realloc(p, sz);
	if (sz && !n)
		err(1, "realloc: %li bytes", (long)sz);
	return n;
}

static void obj_seek(off_t off)
{
	if (lseek(obj_fd, off, SEEK_SET) < 0)
		err(1, "%s: lseek", obj_name);
}

static void obj_read(void *buf, size_t sz)
{
	ssize_t cnt;
	int se;

	cnt = read(obj_fd, buf, sz);
	if (cnt < 0)
		err(1, "%s: read", obj_name);
	if (cnt != sz)
		errx(1, "%s: read: Short read\n", obj_name);
}

static void load_head_conv32(void)
{
	memcpy(obj_head64.magic, obj_head32.magic, sizeof obj_head64.magic);
	obj_head64.type			= obj_head32.type;
	obj_head64.mach			= obj_head32.mach;
	obj_head64.version		= obj_head32.version;
	obj_head64.start		= obj_head32.start;
	obj_head64.ph_offset		= obj_head32.ph_offset;
	obj_head64.sh_offset		= obj_head32.sh_offset;
	obj_head64.flags		= obj_head32.flags;
	obj_head64.eh_size		= obj_head32.eh_size;
	obj_head64.ph_elem_size		= obj_head32.ph_elem_size;
	obj_head64.ph_elem_cnt		= obj_head32.ph_elem_cnt;
	obj_head64.sh_elem_size		= obj_head32.sh_elem_size;
	obj_head64.sh_elem_cnt		= obj_head32.sh_elem_cnt;
	obj_head64.sh_string_idx	= obj_head32.sh_string_idx;
}

static void load_head(void)
{
	obj_read(&obj_head32, sizeof obj_head32);
	obj_seek(0);
	obj_read(&obj_head64, sizeof obj_head64);
	
	if (!memcmp(obj_head64.magic, ELF64_MAGIC, sizeof ELF64_MAGIC - 1))
		obj_elf64 = 1;
	else if (!memcmp(obj_head32.magic, ELF32_MAGIC, sizeof ELF32_MAGIC - 1))
		load_head_conv32();
	else
		errx(1, "%s: Bad ELF magic", obj_name);
	
	if (obj_head64.type != ELF_TYPE_RELOC)
		errx(1, "%s: Not an object file", obj_name);
	if (obj_head64.mach != ELF_MACH_I386 && obj_head64.mach != ELF_MACH_AMD64)
		errx(1, "%s: Unsupported machine %i", obj_name, (int)obj_head64.mach);
}

static void *load_sect(int i)
{
	if (i < 0 || i >= obj_head64.sh_elem_cnt)
		errx(1, "%s: load_sect: Bad section index %i", obj_name, i);
	
	if (obj_sect_image[i])
		return obj_sect_image[i];
	
	obj_sect_image[i] = x_malloc(obj_sect[i].size);
	obj_seek(obj_sect[i].offset);
	obj_read(obj_sect_image[i], obj_sect[i].size);
	return obj_sect_image[i];
}

static void *load_rela_sect(int i, int *rel_cnt)
{
	struct elf32_rela *s;
	struct elf64_rela *d;
	int ri, rcnt;
	
	if (obj_elf64)
	{
		*rel_cnt = obj_sect[i].size / sizeof *d;
		return load_sect(i);
	}
	
	if (obj_sect_image_c[i])
	{
		*rel_cnt = obj_sect[i].size / sizeof *s;
		return obj_sect_image_c[i];
	}
	
	rcnt = obj_sect[i].size / sizeof *s;
	d    = x_malloc(rcnt * sizeof *d);
	s    = load_sect(i);
	
	for (ri = 0; ri < rcnt; ri++)
	{
		d[ri].offset = s[ri].offset;
		d[ri].info   = s[ri].info;
		d[ri].addend = s[ri].addend;
	}
	
	obj_sect_image_c[i] = d;
	*rel_cnt = rcnt;
	return d;
}

static void *load_rel_sect(int i, int *rel_cnt)
{
	struct elf32_rel *s;
	struct elf64_rel *d;
	int ri, rcnt;
	
	if (obj_elf64)
	{
		*rel_cnt = obj_sect[i].size / sizeof *d;
		return load_sect(i);
	}
	
	if (obj_sect_image_c[i])
	{
		*rel_cnt = obj_sect[i].size / sizeof *s;
		return obj_sect_image_c[i];
	}
	
	rcnt = obj_sect[i].size / sizeof *s;
	d    = x_malloc(rcnt * sizeof *d);
	s    = load_sect(i);
	
	for (ri = 0; ri < rcnt; ri++)
	{
		d[ri].offset = s[ri].offset;
		d[ri].info   = s[ri].info;
	}
	
	obj_sect_image_c[i] = d;
	*rel_cnt = rcnt;
	return d;
}

static void *load_sym_sect(int i, int *sym_cnt)
{
	struct elf32_sym *s;
	struct elf64_sym *d;
	int si, scnt;
	
	if (obj_elf64)
	{
		*sym_cnt = obj_sect[i].size / sizeof *d;
		return load_sect(i);
	}
	
	if (obj_sect_image_c[i])
	{
		*sym_cnt = obj_sect[i].size / sizeof *s;
		return obj_sect_image_c[i];
	}
	
	scnt = obj_sect[i].size / sizeof *s;
	d    = x_malloc(scnt * sizeof *d);
	s    = load_sect(i);
	
	for (si = 0; si < scnt; si++)
	{
		d[si].name  = s[si].name;
		d[si].value = s[si].value;
		d[si].size  = s[si].size;
		d[si].info  = s[si].info;
		d[si].other = s[si].other;
		d[si].sect  = s[si].sect;
	}
	
	obj_sect_image_c[i] = d;
	*sym_cnt = scnt;
	return d;
}

static void show_sh(void)
{
	struct elf64_sect_head *s;
	struct elf64_sect_head *e;
	int i;

	fprintf(stderr, "show_sh:     type    flags     addr     size    align     link     info   offset\n");
	e = obj_sect + obj_head64.sh_elem_cnt;
	s = obj_sect;
	while (s < e)
	{
		fprintf(stderr, "%4i     %8lx %8lx %8lx %8lx %8lx %8lu %8lu %8lu\n",
			(int)(s - obj_sect),
			(unsigned long)s->type,
			(unsigned long)s->flags,
			(unsigned long)s->addr,
			(unsigned long)s->size,
			(unsigned long)s->align,
			(unsigned long)s->link,
			(unsigned long)s->info,
			(unsigned long)s->offset);
		s++;
	}
}

static void load_sh_32(void)
{
	struct elf32_sect_head s32;
	struct elf64_sect_head *s;
	struct elf64_sect_head *e;
	off_t off;
	
	obj_sect_image_c = x_malloc(sizeof *obj_sect_image * obj_head64.sh_elem_cnt);
	obj_sect_image	 = x_malloc(sizeof *obj_sect_image * obj_head64.sh_elem_cnt);
	obj_sect	 = x_malloc(sizeof *obj_sect	 * obj_head64.sh_elem_cnt);
	
	e   = obj_sect + obj_head64.sh_elem_cnt;
	s   = obj_sect;
	off = obj_head64.sh_offset;
	while (s < e)
	{
		obj_seek(off);
		obj_read(&s32, sizeof s32);
		
		s->name		= s32.name;
		s->type		= s32.type;
		s->flags	= s32.flags;
		s->addr		= s32.addr;
		s->offset	= s32.offset;
		s->size		= s32.size;
		s->link		= s32.link;
		s->info		= s32.info;
		s->align	= s32.align;
		s->elem_size	= s32.elem_size;
		
		off += obj_head64.sh_elem_size;
		s++;
	}
#if VERBOSE
	// show_sh();
#endif
}

static void load_sh_64(void)
{
	struct elf64_sect_head *s;
	struct elf64_sect_head *e;
	off_t off;
	
	obj_sect_image = x_malloc(sizeof *obj_sect_image * obj_head64.sh_elem_cnt);
	obj_sect       = x_malloc(sizeof *obj_sect	 * obj_head64.sh_elem_cnt);
	
	e   = obj_sect + obj_head64.sh_elem_cnt;
	s   = obj_sect;
	off = obj_head64.sh_offset;
	while (s < e)
	{
		obj_seek(off);
		obj_read(s, sizeof *s);
		
		off += obj_head64.sh_elem_size;
		s++;
	}
	/* show_sh(); */
}

static void load_sh(void)
{
	if (obj_elf64)
		load_sh_64();
	else
		load_sh_32();
}

static void init_sa(void)
{
	struct elf64_sect_head *s;
	struct elf64_sect_head *e;

	e = obj_sect + obj_head64.sh_elem_cnt;
	s = obj_sect;
	while (s < e)
	{
		if (s->flags & ELF_SHF_ALLOC)
		{
			if (!image_size)
				obj_sect0 = s - obj_sect;
			s->addr  = image_size;
			s->addr +=   s->align - 1;
			s->addr &= ~(s->align - 1);

			image_size = s->addr + s->size;
		}
		s++;
	}
	/* show_sh(); */
}

static void init_img(void)
{
	struct elf64_sect_head *s;
	struct elf64_sect_head *e;
	int i;

	image = x_malloc(image_size);

	e = obj_sect + obj_head64.sh_elem_cnt;
	for (s = obj_sect; s < e; s++)
	{
		if (!(s->flags & ELF_SHF_ALLOC))
			continue;
		if (s->type == ELF_SHT_NOBITS)
			continue;
		obj_seek(s->offset);
		obj_read(image + s->addr, s->size);
	}
}

static void strchk(const struct elf64_sect_head *sect, const char *strtab, uint32_t off)
{
	if (off >= sect->size)
		errx(1, "strchk: bad offset");
	if (strtab[sect->size - 1])
		errx(1, "strchk: unterminated table");
}

static symval grow_image(size_t sz, size_t align)
{
	size_t nsz;
	symval v;

	v = image_size + align - 1 & ~(align - 1);

	nsz   = v + sz;
	image = x_realloc(image, nsz);
	memset(image + image_size, 0, nsz - image_size);
	image_size = nsz;

	return v;
}

static void alloc_com(struct elf64_sym *sym, const char *strtab)
{
#if VERBOSE
	fprintf(stderr, "%s: common %s\n", obj_name, strtab + sym->name); /* XXX */
#endif
	sym->value = grow_image(sym->size, sym->value);
	sym->sect  = obj_sect0;
	if (!obj_sect0)
		errx(1, "%s: alloc_com: no sections", obj_name);

#if 0
	sym->value = image_size + (sym->value - 1) & ~(sym->value - 1);
	sym->sect  = obj_sect0; /* XXX */
	if (!obj_sect0)
		errx(1, "%s: alloc_com: no sections", obj_name);

	nsz   = sym->value + sym->size;
	image = x_realloc(image, nsz);
	memset(image + image_size, 0, nsz - image_size);
	image_size = nsz;
#endif
}

static symval sym_val(const struct elf64_sym *sym, const struct elf64_sect_head *strs, const char *strtab)
{
	struct elf64_sect_head *s;
	
	if (sym->sect == ELF_SHN_UNDEF)
	{
		strchk(strs, strtab, sym->name);
		errx(1, "%s: sym_val: %s: undefined symbol", obj_name, strtab + sym->name);
	}
	if (sym->sect == ELF_SHN_COMMON)
		abort(); // alloc_com(sym, strtab);
	if (sym->sect == ELF_SHN_ABS) /* XXX */
		return sym->value;
	if (sym->sect >= obj_head64.sh_elem_cnt)
	{
		strchk(strs, strtab, sym->name);
		errx(1, "%s: sym_val: %s: bad section index %i", obj_name, strtab + sym->name, (int)sym->sect);
	}
	s = &obj_sect[sym->sect];
	if (!(s->flags & ELF_SHF_ALLOC))
		errx(1, "%s: sym_val: section %i not allocated", obj_name, (int)sym->sect);
	return sym->value + s->addr;
}

static int sym_exported(const char *name)
{
	int i;

	if (!exp)
		return 1;
	for (i = 0; i < exp_cnt; i++)
		if (!strcmp(exp[i], name))
			return 1;
	return 0;
}

static void add_sym(const struct modsym *s)
{
	mod_sym = x_realloc(mod_sym, (mod_nsym + 1) * sizeof *mod_sym);
	mod_sym[mod_nsym++] = *s;
}

static void add_imp(const char *name, uint32_t off, int size, int shift)
{
	struct modsym sym;
	size_t len;

#if VERBOSE
	fprintf(stderr, "add_imp: 0x%08lx, size = %i, shift = %i\n", (long)off, size, shift);
#endif
	len = strlen(name);
	if (len >= sizeof sym.name)
		errx(1, "%s: symbol name too long", name);

	memset(&sym, 0, sizeof sym);
	sym.addr     = off;
	sym.flags    = SYM_IMPORT;
	sym.name_len = len;
	sym.size     = size;
	sym.shift    = shift;
	strcpy(sym.name, name);
	add_sym(&sym);
}

static void add_exp(const char *name, uint32_t off)
{
	struct modsym sym;
	size_t len;

	if (!sym_exported(name))
		return;

	len = strlen(name);
	if (len >= sizeof sym.name)
		errx(1, "%s: %s: symbol name too long", obj_name, name);

#if VERBOSE
	fprintf(stderr, "%s: exporting %s\n", obj_name, name);
#endif
	memset(&sym, 0, sizeof sym);
	sym.addr     = off;
	sym.flags    = SYM_EXPORT;
	sym.name_len = len;
	strcpy(sym.name, name);
	add_sym(&sym);
}

static void add_rel(uint32_t off, int size, int shift, int type)
{
#if VERBOSE
	fprintf(stderr, "add_rel: %i: 0x%08lx, type %i, size %i\n", mod_nrel, (long)off, type, size);
#endif
	mod_rel = x_realloc(mod_rel, (mod_nrel + 1) * sizeof *mod_rel);
	mod_rel[mod_nrel].addr = off;
	mod_rel[mod_nrel].type = type;
	mod_rel[mod_nrel].size = size;
	mod_nrel++;
}

static uint32_t read24(symval off)
{
	return		   image[off	]	|
		((uint32_t)image[off + 1] << 8)	|
		((uint32_t)image[off + 2] << 16);
}

static void write24(symval off, uint32_t v)
{
	image[off    ] = v;
	image[off + 1] = v >> 8;
	image[off + 2] = v >> 16;
}

static uint32_t read32(symval off)
{
	return		   image[off	]		|
		((uint32_t)image[off + 1] << 8)		|
		((uint32_t)image[off + 2] << 16)	|
		((uint32_t)image[off + 3] << 24);
}

static void write32(symval off, uint32_t v)
{
	image[off    ] = v;
	image[off + 1] = v >> 8;
	image[off + 2] = v >> 16;
	image[off + 3] = v >> 24;
}

static uint32_t read64(symval off)
{
	return		   image[off	]		|
		((uint64_t)image[off + 1] << 8)		|
		((uint64_t)image[off + 2] << 16)	|
		((uint64_t)image[off + 3] << 24)	|
		((uint64_t)image[off + 4] << 32)	|
		((uint64_t)image[off + 5] << 40)	|
		((uint64_t)image[off + 6] << 48)	|
		((uint64_t)image[off + 7] << 56);
}

static void write64(symval off, uint64_t v)
{
	image[off    ] = v;
	image[off + 1] = v >> 8;
	image[off + 2] = v >> 16;
	image[off + 3] = v >> 24;
	image[off + 4] = v >> 32;
	image[off + 5] = v >> 40;
	image[off + 6] = v >> 48;
	image[off + 7] = v >> 56;
}

static void proc_i386_rel(symval off, int type, int rel, symval a, symval addend)
{
	uint32_t goff;
	uint32_t v;
	
	switch (type)
	{
	case ELF_REL_386_32:
		if (rel)
			add_rel(off, 4, 0, MR_T_NORMAL);
		v  = read32(off);
		v += a;
		write32(off, v);
		break;
	case ELF_REL_386_PC32:
		if (!rel)
			add_rel(off, 4, 0, MR_T_ABS);
		v  = read32(off);
		v += a;
		v -= off;
		write32(off, v);
		break;
	case ELF_REL_386_GOT32:
		goff = grow_image(4, 4);
		write32(goff, a);
		
		if (!rel)
			add_rel(off, 4, 0, MR_T_ABS);
		v  = read32(off);
		v += goff;
		v -= off;
		write32(off, v);
		abort();
		break;
	case ELF_REL_386_PLT32:
		abort();
		break;
	case ELF_REL_386_COPY:
		abort();
		break;
	case ELF_REL_386_GLOB_DAT:
		abort();
		break;
	case ELF_REL_386_JMP_SLOT:
		abort();
		break;
	case ELF_REL_386_RELATIVE:
		abort();
		break;
	case ELF_REL_386_GOTOFF:
		fprintf(stderr, "ELF_REL_386_GOTOFF @ 0x%lx\n", (unsigned long)off);
		if (!rel)
			add_rel(off, 4, 0, MR_T_ABS);
		v  = read32(off);
		v += a;
		v -= got_off;
		write32(off, v);
		break;
	case ELF_REL_386_GOTPC:
		fprintf(stderr, "ELF_REL_386_GOTPC @ 0x%lx\n", (unsigned long)off);
		if (!rel)
			add_rel(off, 4, 0, MR_T_ABS);
		v  = read32(off);
		v += got_off;
		v += addend;
		v -= off;
		write32(off, v);
		break;
	case ELF_REL_386_16:
		if (rel)
			add_rel(off, 2, 0, MR_T_NORMAL);
		v  = read32(off);
		v += a;
		write32(off, v);
		break;
	case ELF_REL_386_PC16:
		if (!rel)
			add_rel(off, 2, 0, MR_T_ABS);
		v = read32(off);
		v += a;
		v -= off;
		write32(off, v);
		break;
	default:
		errx(1, "proc_i386_rel: unsupported type %i", (int)type);
	}
}

static void proc_amd64_rel(symval off, int type, int rel, symval a, symval addend)
{
	uint32_t goff;
	uint32_t v;
	
	switch (type)
	{
#if 0
	case ELF_REL_386_32:
		if (rel)
			add_rel(off, 4, 0, MR_T_NORMAL);
		v  = read32(off);
		v += a;
		write32(off, v);
		break;
	case ELF_REL_386_PC32:
		if (!rel)
			add_rel(off, 4, 0, MR_T_ABS);
		v  = read32(off);
		v += a;
		v -= off;
		write32(off, v);
		break;
	case ELF_REL_386_GOT32:
		goff = grow_image(4, 4);
		write32(goff, a);
		
		if (!rel)
			add_rel(off, 4, 0, MR_T_ABS);
		v  = read32(off);
		v += goff;
		v -= off;
		write32(off, v);
		abort();
		break;
	case ELF_REL_386_PLT32:
		abort();
		break;
	case ELF_REL_386_COPY:
		abort();
		break;
	case ELF_REL_386_GLOB_DAT:
		abort();
		break;
	case ELF_REL_386_JMP_SLOT:
		abort();
		break;
	case ELF_REL_386_RELATIVE:
		abort();
		break;
	case ELF_REL_386_GOTOFF:
		fprintf(stderr, "ELF_REL_386_GOTOFF @ 0x%x\n", off);
		if (!rel)
			add_rel(off, 4, 0, MR_T_ABS);
		v  = read32(off);
		v += a;
		v -= got_off;
		write32(off, v);
		break;
	case ELF_REL_386_GOTPC:
		fprintf(stderr, "ELF_REL_386_GOTPC @ 0x%x\n", off);
		if (!rel)
			add_rel(off, 4, 0, MR_T_ABS);
		v  = read32(off);
		v += got_off;
		v += addend;
		v -= off;
		write32(off, v);
		break;
	case ELF_REL_386_16:
		if (rel)
			add_rel(off, 2, 0, MR_T_NORMAL);
		v  = read32(off);
		v += a;
		write32(off, v);
		break;
	case ELF_REL_386_PC16:
		if (!rel)
			add_rel(off, 2, 0, MR_T_ABS);
		v = read32(off);
		v += a;
		v -= off;
		write32(off, v);
		break;
#endif
	case ELF_REL_AMD64_64:
		if (rel)
			add_rel(off, 8, 0, MR_T_NORMAL);
		v  = read64(off);
		v += a;
		write64(off, v);
		break;
	case ELF_REL_AMD64_PC32:
		if (!rel)
			add_rel(off, 4, 0, MR_T_ABS);
		v  = read32(off);
		v += a;
		v -= off;
		write32(off, v);
		break;
	case ELF_REL_AMD64_32:
	case ELF_REL_AMD64_32S:
		if (rel)
			add_rel(off, 4, 0, MR_T_NORMAL);
		v  = read32(off);
		v += a;
		write32(off, v);
		break;
	default:
		/* errx(1, "proc_amd64_rel: unsupported type %i", (int)type); */
		warnx("proc_amd64_rel: unsupported type %i", (int)type);
	}
}

static void proc_single_rela(const struct elf64_sect_head *s,
			     const struct elf64_sym *ssi,
			     int ssi_cnt,
			     const struct elf64_sect_head *strs,
			     const char *strtab,
			     const struct elf64_rela *ra)
{
	const struct elf64_sym *sym;
	uint32_t symi;
	symval symv;
	symval off;
	uint8_t type;
	uint32_t *p;
	int is_rel = 1;
	
	if (obj_elf64)
	{
		type = ra->info  & 0xffffffff;
		symi = ra->info >> 32;
		off  = ra->offset + s->addr;
	}
	else
	{
		type = ra->info & 255;
		symi = ra->info >> 8;
		off  = ra->offset + s->addr;
	}
	
#if DEBUG
	fprintf(stderr, "proc_single_rela: offset = %li\n", (long)ra->offset);
	fprintf(stderr, "                  type   = %i\n", type);
	fprintf(stderr, "                  symi   = %i\n", symi);
#endif
	
	if (symi >= ssi_cnt)
		errx(1, "proc_single_rela: bad symbol index");
	sym = &ssi[symi];
	
#if DEBUG
	fprintf(stderr, "                  sym->sect = %i\n", (int)sym->sect);
#endif
	
	if (off + 4 > image_size)
		errx(1, "proc_single_rela: bad offset");
	p = (uint32_t *)((uint8_t *)image + off);
	
	if (symi != ELF_STN_UNDEF)
	{
		strchk(strs, strtab, sym->name);
		
		if (sym->sect != ELF_SHN_UNDEF)
			symv = sym_val(sym, strs, strtab);
		else if (!strcmp(strtab + sym->name, "_GLOBAL_OFFSET_TABLE_"))
			symv = got_off;
		else
		{
			add_imp(strtab + sym->name, off, 4, 0); /* XXX unshifted 32-bit only */
			symv = 0;
		}
	}
	else
		symv = 0;
	symv += ra->addend;
	
	if (sym->sect == ELF_SHN_ABS || !sym->sect) /* XXX !sym->sect is what gas generates */
		is_rel = 0;
	
	switch (obj_head64.mach)
	{
	case ELF_MACH_I386:
		proc_i386_rel(off, type, is_rel, symv, ra->addend);
		break;
	case ELF_MACH_AMD64:
		proc_amd64_rel(off, type, is_rel, symv, ra->addend);
		break;
	default:
		abort();
	}
}

static void proc_sect_coms(struct elf64_sect_head *syms)
{
	struct elf64_sect_head *strs;
	struct elf64_sym *ssi;
	char *str;
	int cnt;
	int i;
	
#if VERBOSE
	fprintf(stderr, "proc_sect_coms: processing section %i\n", (int)(syms - obj_sect));
#endif
	if (syms->link >= obj_head64.sh_elem_cnt)
	{
		fprintf(stderr, "proc_sect_coms: rs->link = %lu\n", (long)syms->link);
		fprintf(stderr, "  obj_head64.sh_elem_cnt = %lu\n", (long)obj_head64.sh_elem_cnt);
		errx(1, "proc_sect_rel: bad section index");
	}
	
	ssi  = load_sym_sect(syms - obj_sect, &cnt);
	strs = &obj_sect[syms->link];
	str  = load_sect(syms->link);
	
	for (i = 0; i < cnt; i++)
		if (ssi[i].sect == ELF_SHN_COMMON)
			alloc_com(&ssi[i], str);
#if VERBOSE
	fprintf(stderr, "proc_sect_coms: done\n");
#endif
}

static void proc_coms(void)
{
	struct elf64_sect_head *s;
	struct elf64_sect_head *e;
	int i;
	
	e = obj_sect + obj_head64.sh_elem_cnt;
	for (s = obj_sect; s < e; s++)
		if (s->type == ELF_SHT_SYMTAB)
			proc_sect_coms(s);
}

static void proc_sect_rel(struct elf64_sect_head *rs)
{
	struct elf64_sect_head *syms;
	struct elf64_sect_head *strs;
	struct elf64_sect_head *s;
	struct elf64_rela *ra;
	struct elf64_rel *r;
	struct elf64_sym *ssi;
	int ssi_cnt;
	char *str;
	int cnt;
	int i;
	
	if (rs->link >= obj_head64.sh_elem_cnt || rs->info >= obj_head64.sh_elem_cnt)
	{
		fprintf(stderr, "proc_sect_rel: rs->link = %lu\n", (long)rs->link);
		fprintf(stderr, "               rs->info = %lu\n", (long)rs->info);
		fprintf(stderr, " obj_head64.sh_elem_cnt = %lu\n", (long)obj_head64.sh_elem_cnt);
		errx(1, "proc_sect_rel: bad section index");
	}
	syms = &obj_sect[rs->link];
	s    = &obj_sect[rs->info];
	
	if (!(s->flags & ELF_SHF_ALLOC))
		return;
	
	ssi  = load_sym_sect(rs->link, &ssi_cnt);
	strs = &obj_sect[syms->link];
	str  = load_sect(syms->link);
	
	if (rs->type == ELF_SHT_RELA)
	{
		ra  = load_rela_sect(rs - obj_sect, &cnt);
		for (i = 0; i < cnt; i++)
			proc_single_rela(s, ssi, ssi_cnt, strs, str, &ra[i]);
		return;
	}
	
	r = load_rel_sect(rs - obj_sect, &cnt);
	for (i = 0; i < cnt; i++)
	{
		struct elf64_rela cra;
		
		cra.offset = r[i].offset;
		cra.info   = r[i].info;
		cra.addend = 0;
		proc_single_rela(s, ssi, ssi_cnt, strs, str, &cra);
	}
}

static int got_elem_size(void)
{
	switch (obj_head64.mach)
	{
	case ELF_MACH_I386:
		return 4;
	case ELF_MACH_AMD64:
		return 8; // XXX
	default:
		abort();
	}
}

static void proc_rel(void)
{
	struct elf64_sect_head *s;
	struct elf64_sect_head *e;
	int i;
	
	got_off = grow_image(0, 16);
	e = obj_sect + obj_head64.sh_elem_cnt;
	for (s = obj_sect; s < e; s++)
		if (s->type == ELF_SHT_REL || s->type == ELF_SHT_RELA)
			proc_sect_rel(s);
	got_cnt = (image_size - got_off) / got_elem_size();
}

static void export_symtab(struct elf64_sect_head *s)
{
	struct elf64_sect_head *strs;
	struct elf64_sym *sym;
	struct elf64_sym *p;
	struct elf64_sym *e;
	int sym_cnt;
	char *strtab;
	int type;
	int bind;
	
	sym    = load_sym_sect(s - obj_sect, &sym_cnt);
	strs   = &obj_sect[s->link];
	strtab = load_sect(s->link);
	
	e = sym + sym_cnt;
	for (p = sym; p < e; p++)
	{
		if (p->sect == ELF_SHN_COMMON) /* XXX is this really needed? */
			abort(); // alloc_com(p, strtab);
		if (p->sect == ELF_SHN_UNDEF)
			continue;
		
		type = p->info & 15;
		bind = p->info >> 4;
		if (bind != ELF_STB_GLOBAL && bind != ELF_STB_WEAK)
			continue;
		strchk(strs, strtab, p->name);
		add_exp(strtab + p->name, sym_val(p, strs, strtab));
	}
}

static void export(void)
{
	struct elf64_sect_head *s;
	struct elf64_sect_head *e;
	
	e = obj_sect + obj_head64.sh_elem_cnt;
	for (s = obj_sect; s < e; s++)
		if (s->type == ELF_SHT_SYMTAB)
			export_symtab(s);
}

static void x_write(int fd, const void *buf, size_t sz)
{
	ssize_t r;
	
	r = write(fd, buf, sz);
	if (r < 0)
		err(1, "%s: write", mod_name);
	if (r != sz)
		errx(1, "%s: write: Short write", mod_name);
}

static void save(void)
{
	struct modhead mh;
	int fd;
	
	memset(&mh, 0, sizeof mh);
	memcpy(mh.magic, "\0KMODULE", 8);
	mh.rel_start  = sizeof mh;
	mh.rel_count  = mod_nrel;
	mh.sym_start  = mh.rel_start + mod_nrel * sizeof *mod_rel;
	mh.sym_count  = mod_nsym;
	mh.got_start  = got_off;
	mh.got_count  = got_cnt;
	mh.code_start = mh.sym_start + mod_nsym * sizeof *mod_sym;
	mh.code_size  = image_size;
	
	fd = open(mod_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd < 0)
		err(1, "%s: open", mod_name);
	if (kern_flag)
	{
		((uint32_t *)image)[0x44 / 4] = image_size + mh.sym_start + /* XXX */
						mod_nsym * sizeof *mod_sym;
		((uint32_t *)image)[0x60 / 4] = image_size;
		x_write(fd, image, image_size);
		mh.rel_start += image_size;
		mh.sym_start += image_size;
		mh.code_start = 0;
	}
	x_write(fd, &mh, sizeof mh);
	x_write(fd, mod_rel, mod_nrel * sizeof *mod_rel);
	x_write(fd, mod_sym, mod_nsym * sizeof *mod_sym);
	if (!kern_flag)
		x_write(fd, image, image_size);
	if (close(fd))
		err(1, "%s: close", mod_name);
}

static void load_exports(void)
{
	char buf[81];
	char *p;
	FILE *f;
	
	if (!exp_name)
		return;
	f = fopen(exp_name, "r");
	if (!f)
		err(1, "%s: fopen", exp_name);
	while (fgets(buf, sizeof buf, f))
	{
		p = strchr(buf, '\n');
		if (p)
			*p = 0;
		if (!*buf)
			continue;
		
		exp = x_realloc(exp, (exp_cnt + 1) * sizeof *exp);
		p = strdup(buf);
		if (!p)
			err(1, "strdup");
		exp[exp_cnt++] = p;
	}
	if (ferror(f))
	{
		perror(exp_name);
		exit(errno);
	}
	fclose(f);
}

static void usage(void)
{
	fprintf(stderr, "usage: mgen [-k-] NEWLIB OBJECT [EXPORTS]\n");
}

static int procopt(int argc, char **argv)
{
	int stop = 0;
	int i, n;
	
	/* XXX should use getopt */
	
	for (i = 1; i < argc && !stop; i++)
	{
		if (*argv[i] != '-')
			break;
		for (n = 1; argv[i][n]; n++)
			switch (argv[i][n])
			{
			case '-':
				stop = 1;
				break;
			case 'k':
				kern_flag = 1;
				break;
			default:
				usage();
				exit(126);
			}
	}
	return i;
}

static void resolve(void)
{
	struct modsym *p;
	struct modsym *e;

	e = mod_sym + mod_nsym;
	p = mod_sym;
	for (; p < e; p++)
	{
		if ((p->flags & SYM_IMPORT) == 0)
			continue;
		if (!strncmp(p->name, "_end", 5) || !strncmp(p->name, "_edata", 7))
		{
			fprintf(stderr, "modgen: resolve: REALLY UGLY HACK\n");
			/* XXX this is really bad */
			write32(p->addr, image_size);
			add_rel(p->addr, 4, 0, -1);
			continue;
		}
		errx(255, "%s: unresolved symbol \"%s\"\n", obj_name, p->name);
	}
}

int main(int argc, char **argv)
{
	int i;
	
	i = procopt(argc, argv);
	_progname = argv[0];
	if (i + 1 >= argc)
	{
		usage();
		return 126;
	}
	mod_name  = argv[i];
	obj_name  = argv[i + 1];
	if (i + 2 < argc)
		exp_name = argv[i + 2];
	
	obj_fd = open(obj_name, O_RDONLY);
	if (obj_fd < 0)
		err(errno, "%s", obj_name);
	
	load_exports();
	load_head();
	load_sh();
	init_sa();
	init_img();
	proc_coms();
	proc_rel();
	export();
	if (kern_flag)
		resolve();
	save();
	return 0;
}
