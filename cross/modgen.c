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

 /*
  * WARNING: This program is for i386 only.
  */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <bfd.h>

#include "module.h"

bfd *		abfd;

int		sec_count;
asection **	sec;

int		sym_count;
asymbol	**	sym;

int		export_count = 0;
char **		export;

int		modsym_count = 0;
struct modsym *	modsym = NULL;

int		modrel_count = 0;
unsigned *	modrel = NULL;

struct modhead	modhead;

char *		obj_name;
char *		mod_name;
char *		exp_name;

unsigned char *	image;

void *x_malloc(unsigned size)
{
	void *p = malloc(size);
	
	if (!p)
	{
		perror("malloc");
		exit(errno);
	}
	
	memset(p, 0, size);
	return p;
}

void *x_realloc(void *p, unsigned size)
{
	void *n = realloc(p, size);
	
	if (size && !n)
	{
		perror("realloc");
		exit(errno);
	}
	
	return n;
}

void sym_add(struct modsym *s)
{
	modsym = x_realloc(modsym, (modsym_count + 1) * sizeof *modsym);
	modsym[modsym_count++] = *s;
}

void rel_add(unsigned a)
{
	modrel = x_realloc(modrel, (modrel_count + 1) * sizeof *modrel);
	modrel[modrel_count++] = a;
}

void read_u32(unsigned addr, unsigned *v)
{
	*v = *((unsigned *)(image + addr));
}

void write_u32(unsigned addr, unsigned *v)
{
	*((unsigned *)(image + addr)) = *v;
}

void init_head(void)
{
	memcpy(modhead.magic, "\0KMODULE", 8);
	modhead.code_size=0;
	modhead.rel_count=0;
	modhead.sym_count=0;
}

void init_vma(bfd *b, asection *s, void *p)
{
	unsigned a = 1 << s->alignment_power;
	
	if (modhead.code_size & (a-1))
	{
		modhead.code_size &= ~(a-1);
		modhead.code_size += a;
	}
	s->vma = modhead.code_size;
	modhead.code_size += bfd_section_size(b, s);
	// modhead.code_size += s->/* _raw_ */size;
	printf("%-16s @ 0x%08x (aligned to %i)\n", s->name, (unsigned)s->vma, a);
}

void copy_image(bfd *b, asection *s, void *p)
{
	// if (!bfd_get_section_contents(b, s, image + s->vma, 0, s->/* _raw_*/ size))
	if (!bfd_get_section_contents(b, s, image + s->vma, 0, bfd_section_size(b, s)))
	{
		bfd_perror(obj_name);
		exit(255);
	}
}

void do_rel(bfd *b, asection *s, void *p)
{
	int rel_count;
	arelent **rel;
	long size;
	int i;
	
	size = bfd_get_reloc_upper_bound(b, s);
	if (size < 0)
	{
		bfd_perror(obj_name);
		exit(255);
	}
	rel = x_malloc(size);
	rel_count = bfd_canonicalize_reloc(b, s, rel, sym);
	
	for (i = 0; i < rel_count; i++)
	{
		unsigned v;
		asymbol *rs;
		arelent *r;
		
		r = rel[i];
		rs = *(r->sym_ptr_ptr);
		
		read_u32(s->vma + r->address, &v);
		if (!strcmp(r->howto->name, "R_386_PC32"))
			v -= r->address;
		else
		{
			if (!strcmp(r->howto->name, "R_386_32"))
				rel_add(s->vma + r->address);
			else
			{
				fprintf(stderr, "%s: unsupported relocation type: %s\n", obj_name, r->howto->name);
				exit(255);
			}
		}
		
		v += r->addend;
		if (rs->flags)
			v += rs->value + rs->section->vma;
		else
		{
			struct modsym ms;
			
			ms.value = r->address + s->vma;
			ms.flags = SYM_IMPORT;
			ms.name_len = sizeof ms.name;
			ms.name[sizeof ms.name - 1] = 0;
			strncpy(ms.name, rs->name, sizeof ms.name - 1);
			
			sym_add(&ms);
		}
		write_u32(s->vma + r->address, &v);
	}
}

void open_bfd(char *p)
{
	long size;
	
	obj_name = p;
	
	abfd = bfd_openr(p, "default");
	if (!abfd)
	{
		bfd_perror(p);
		exit(255);
	}
	
	if (!bfd_check_format(abfd, bfd_object))
	{
		bfd_perror(p);
		exit(255);
	}
	
	size = bfd_get_symtab_upper_bound(abfd);
	if (size < 0)
	{
		bfd_perror(p);
		exit(255);
	}
	sym = x_malloc(size);
	sym_count = bfd_canonicalize_symtab(abfd, sym);
}

int sym_exported(const char *name)
{
	int i;
	
	if (!export)
		return 1;
	for (i = 0; i < export_count; i++)
		if (!strcmp(export[i], name))
			return 1;
	return 0;
}

void sym_export(void)
{
	struct modsym s;
	int i;
	
	for (i = 0; i < sym_count; i++)
		if (sym[i]->flags & BSF_EXPORT)
		{
			if (!sym_exported(sym[i]->name))
				continue;
			
			s.value = sym[i]->value + sym[i]->section->vma;
			s.flags = SYM_EXPORT;
			s.name_len = sizeof s.name;
			s.name[sizeof s.name - 1] = 0;
			strncpy(s.name, sym[i]->name, sizeof s.name - 1);
			
			sym_add(&s);
		}
}

void mod_save(void)
{
	int fd = open(mod_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	
	modhead.rel_start  = sizeof modhead;
	modhead.sym_start  = modhead.rel_start + modrel_count * sizeof *modrel;
	modhead.code_start = modhead.sym_start + modsym_count * sizeof *modsym;
	modhead.rel_count  = modrel_count;
	modhead.sym_count  = modsym_count;
	
	write(fd, &modhead, sizeof modhead);
	lseek(fd, (long)modhead.rel_start, SEEK_SET);
	write(fd, modrel, modrel_count * sizeof *modrel);
	write(fd, modsym, modsym_count * sizeof *modsym);
	write(fd, image, modhead.code_size);
	
	close(fd);
}

void load_exports(void)
{
	char buf[81];
	char *p;
	FILE *f;
	
	if (!exp_name)
		return;
	f = fopen(exp_name, "r");
	if (!f)
	{
		perror(exp_name);
		exit(errno);
	}
	while (fgets(buf, sizeof buf, f))
	{
		p = strchr(buf, '\n');
		if (p)
			*p = 0;
		if (!*buf)
			continue;
		
		export = x_realloc(export, (export_count + 1) * sizeof *export);
		p = strdup(buf);
		if (!p)
		{
			perror("strdup");
			exit(errno);
		}
		export[export_count++] = p;
	}
	if (ferror(f))
	{
		perror(exp_name);
		exit(errno);
	}
	fclose(f);
}

int main(int argc, char **argv)
{
	if (argc < 3 || argc > 4)
	{
		fprintf(stderr, "Usage: modgen NEWLIB OBJECT [EXPORTS]\n");
		return 255;
	}
	
	mod_name = argv[1];
	obj_name = argv[2];
	if (argc > 3)
		exp_name = argv[3];
	
	bfd_init();
	open_bfd(obj_name);
	
	init_head();
	bfd_map_over_sections(abfd, init_vma, NULL);
	printf("code size is 0x%x\n", modhead.code_size);
	image = x_malloc(modhead.code_size);
	bfd_map_over_sections(abfd, copy_image, NULL);
	bfd_map_over_sections(abfd, do_rel, NULL);
	load_exports();
	sym_export();
	mod_save();
	
	bfd_close(abfd);
	return 0;
}
