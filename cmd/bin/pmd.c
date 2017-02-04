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

#include <arch/archdef.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <os386.h>
#include <err.h>

struct cph
{
	struct core_page_head ph;
	off_t off;
};

static struct cph *ch;
static int ch_c, ch_s;

static char *pathname = "core";
static FILE *file;

static uint8_t read8(uint32_t a)
{
	struct cph *p;
	int i;
	
	for (i = 0; i < ch_c; i++)
	{
		p = &ch[i];
		if (p->ph.addr <= a && p->ph.addr + p->ph.size > a)
		{
			if (fseek(file, p->off + a - p->ph.addr, SEEK_SET))
				err(1, "%s: fseek", pathname);
			return fgetc(file);
		}
	}
	errx(1, "Bad address 0x%08x", (unsigned)a);
}

static uint32_t read32(uint32_t a)
{
	uint32_t v;
	
	v  =           read8(a);
	v |= (uint32_t)read8(a + 1) << 8;
	v |= (uint32_t)read8(a + 2) << 16;
	v |= (uint32_t)read8(a + 3) << 24;
	
	return v;
}

#ifdef __ARCH_I386__

static void show_regs(struct core_head *hd)
{
	printf(" GS     = 0x%08lx\n", (long)hd->regs.gs);
	printf(" FS     = 0x%08lx\n", (long)hd->regs.fs);
	printf(" ES     = 0x%08lx\n", (long)hd->regs.es);
	printf(" DS     = 0x%08lx\n", (long)hd->regs.ds);
	printf(" EBP    = 0x%08lx\n", (long)hd->regs.ebp);
	printf(" EDI    = 0x%08lx\n", (long)hd->regs.edi);
	printf(" ESI    = 0x%08lx\n", (long)hd->regs.esi);
	printf(" EDX    = 0x%08lx\n", (long)hd->regs.edx);
	printf(" ECX    = 0x%08lx\n", (long)hd->regs.ecx);
	printf(" EBX    = 0x%08lx\n", (long)hd->regs.ebx);
	printf(" EAX    = 0x%08lx\n", (long)hd->regs.eax);
	printf(" EIP    = 0x%08lx\n", (long)hd->regs.eip);
	printf(" CS     = 0x%08lx\n", (long)hd->regs.cs);
	printf(" EFLAGS = 0x%08lx\n", (long)hd->regs.eflags);
	printf(" ESP    = 0x%08lx\n", (long)hd->regs.esp);
	printf(" SS     = 0x%08lx\n", (long)hd->regs.ss);
}

static void backtrace(struct core_head *hd)
{
	uint32_t rta;
	uint32_t bp = hd->regs.ebp;
	
	while (bp)
	{
		rta = read32(bp + 4);
		bp  = read32(bp);
		
		printf(" 0x%08x\n", rta);
	}
}

#elif defined __ARCH_AMD64__

static uint64_t read64(uint32_t a)
{
	uint64_t v;
	
	v  =           read8(a);
	v |= (uint64_t)read8(a + 1) << 8;
	v |= (uint64_t)read8(a + 2) << 16;
	v |= (uint64_t)read8(a + 3) << 24;
	v |= (uint64_t)read8(a + 4) << 32;
	v |= (uint64_t)read8(a + 5) << 40;
	v |= (uint64_t)read8(a + 6) << 48;
	v |= (uint64_t)read8(a + 7) << 56;
	
	return v;
}

static void show_regs(struct core_head *hd)
{
	printf(" GS     = 0x%08lx\n", (long)hd->regs.gs);
	printf(" FS     = 0x%08lx\n", (long)hd->regs.fs);
	printf(" ES     = 0x%08lx\n", (long)hd->regs.es);
	printf(" DS     = 0x%08lx\n", (long)hd->regs.ds);
	printf(" RBP    = 0x%08lx\n", (long)hd->regs.rbp);
	printf(" RDI    = 0x%08lx\n", (long)hd->regs.rdi);
	printf(" RSI    = 0x%08lx\n", (long)hd->regs.rsi);
	printf(" RDX    = 0x%08lx\n", (long)hd->regs.rdx);
	printf(" RCX    = 0x%08lx\n", (long)hd->regs.rcx);
	printf(" RBX    = 0x%08lx\n", (long)hd->regs.rbx);
	printf(" RAX    = 0x%08lx\n", (long)hd->regs.rax);
	printf(" R8     = 0x%08lx\n", (long)hd->regs.r8);
	printf(" R9     = 0x%08lx\n", (long)hd->regs.r9);
	printf(" R10    = 0x%08lx\n", (long)hd->regs.r10);
	printf(" R11    = 0x%08lx\n", (long)hd->regs.r11);
	printf(" R12    = 0x%08lx\n", (long)hd->regs.r12);
	printf(" R13    = 0x%08lx\n", (long)hd->regs.r13);
	printf(" R14    = 0x%08lx\n", (long)hd->regs.r14);
	printf(" R15    = 0x%08lx\n", (long)hd->regs.r15);
	printf(" RIP    = 0x%08lx\n", (long)hd->regs.rip);
	printf(" CS     = 0x%08lx\n", (long)hd->regs.cs);
	printf(" RFLAGS = 0x%08lx\n", (long)hd->regs.rflags);
	printf(" RSP    = 0x%08lx\n", (long)hd->regs.rsp);
	printf(" SS     = 0x%08lx\n", (long)hd->regs.ss);
}

static void backtrace(struct core_head *hd)
{
	uint64_t rta;
	uint64_t bp = hd->regs.rbp;
	
	while (bp)
	{
		rta = read64(bp + 8);
		bp  = read64(bp);
		
		printf(" 0x%016x\n", rta);
	}
}

#else
#error Unknown arch
#endif

static void load_pghs(void)
{
	off_t off = sizeof(struct core_head);
	struct cph *p;
	
	for (;;)
	{
		if (ch_c >= ch_s)
		{
			ch_s += 16;
			ch = realloc(ch, ch_s * sizeof *ch);
			if (ch == NULL)
				err(1, "realloc");
		}
		p = &ch[ch_c++];
		
		if (!fread(&p->ph, 1, sizeof p->ph, file))
			break;
		p->off = off + sizeof p->ph;
		
		off += sizeof p->ph + p->ph.size;
		if (fseek(file, off, SEEK_SET) < 0)
			err(1, "fseek");
	}
	if (ferror(file))
		err(1, "%s", pathname);
}

int main(int argc, char **argv)
{
	struct core_head hd;
	uint32_t raddr = 0;
	int c;
	
	while (c = getopt(argc, argv, "r:"), c > 0)
		switch (c)
		{
		case 'r':
			raddr = strtoul(optarg, NULL, 0);
			break;
		default:
			return 1;
		}
	argc -= optind;
	argv += optind;
	
	if (argc > 1)
		errx(255, "wrong nr of args");
	
	if (argc == 1)
		pathname = argv[optind];
	
	file = fopen(pathname, "r");
	if (file == NULL)
		err(1, "%s", pathname);
	
	if (fread(&hd, sizeof hd, 1, file) != 1)
		errx(255, "s: can't read\n", pathname);
	hd.pathname[sizeof hd.pathname - 1] = 0;
	
	load_pghs();
	
	if (raddr)
	{
		printf("*0x%08x = 0x%08x\n", (unsigned)raddr, read32(raddr));
		return 0;
	}
	
	printf("\nPost-mortem dump of %s (status 0x%04x):\n\n", hd.pathname, hd.status);
	show_regs(&hd);
	
	printf("\nBacktrace:\n\n");
	backtrace(&hd);
	putchar('\n');
	
	return 0;
}
