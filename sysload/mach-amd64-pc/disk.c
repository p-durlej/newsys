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

#include <sysload/i386-pc.h>
#include <sysload/console.h>
#include <sysload/itoa.h>
#include <sysload/disk.h>
#include <sysload/main.h>
#include <sysload/mem.h>

#include <stdint.h>
#include <string.h>
#include <bioctl.h>
#include <errno.h>

#define DEBUG		0

struct pc_part
{
	uint8_t	 boot;
	uint8_t	 start_head;
	uint8_t	 start_sect;
	uint8_t	 start_cyl;
	uint8_t	 type;
	uint8_t	 end_head;
	uint8_t	 end_sect;
	uint8_t	 end_cyl;
	uint32_t offset;
	uint32_t size;
} __attribute__((packed));

struct disk *disk_boot, *disk_ownpart, *disk_actpart;

static int disk_read_chs(struct disk *dk, blk_t blk, void *buf)
{
	long c, h, s, t;
	
	if (!dk->nsect || !dk->nhead)
		return EINVAL;
	
	s = blk % dk->nsect;
	t = blk / dk->nsect;
	h = t	% dk->nhead;
	c = t	/ dk->nhead;
	s++;
	
	bcp.eax	 = 0x0201;
	bcp.ecx	 = (c << 8) | s | ((c & 0x300) >> 2);
	bcp.edx	 = (h << 8) | dk->unit;
	bcp.es	 = 0;
	bcp.ebx	 = (intptr_t)bounce;
	bcp.intr = 0x13;
	bioscall();
	if (bcp.eflags & 1)
		return EIO;
	memcpy(buf, bounce, 512);
	return 0;
}

struct edd_dap
{
	uint8_t		size;
	uint8_t		gap0;
	uint8_t		count;
	uint8_t		gap1;
	uint16_t	buf_off;
	uint16_t	buf_seg;
	uint64_t	blk;
	uint64_t	buf;
} __attribute__((packed));

static int disk_read_edd(struct disk *dk, blk_t blk, void *buf)
{
	static struct edd_dap dap =
	{
		.size	 = sizeof dap,
		.count	 = 1,
		.buf_off = 0xffff,
		.buf_seg = 0xffff,
	};
	
	dap.buf_off = (intptr_t)bounce & 15;
	dap.buf_seg = (intptr_t)bounce >> 4;
	dap.blk	    = blk;
	
	bcp.intr = 0x13;
	bcp.eax	 = 0x4200;
	bcp.edx	 = dk->unit;
	bcp.esi	 = (intptr_t)&dap & 15;
	bcp.ds	 = (intptr_t)&dap >> 4;
	bioscall();
	
	if (bcp.eflags & 1)
		return EIO;
	memcpy(buf, bounce, 512);
	return 0;
}

static int disk_read_cdrom(struct disk *dk, blk_t blk, void *buf)
{
	static struct edd_dap dap =
	{
		.size	 = sizeof dap,
		.count	 = 1,
		.buf_off = 0xffff,
		.buf_seg = 0xffff,
	};
	
	dap.buf_off = (intptr_t)bounce & 15;
	dap.buf_seg = (intptr_t)bounce >> 4;
	dap.blk	    = blk / 4;
	
	bcp.intr = 0x13;
	bcp.eax	 = 0x4200;
	bcp.edx	 = dk->unit;
	bcp.esi	 = (intptr_t)&dap & 15;
	bcp.ds	 = (intptr_t)&dap >> 4;
	bioscall();
	
	if (bcp.eflags & 1)
		return EIO;
	memcpy(buf, bounce + (blk % 4) * 512, 512);
	return 0;
}

static void disk_cpart(struct disk *dk, int nr, blk_t start, blk_t size, int type, int act)
{
	struct disk *dkp;
	char *p;
	
	dkp = mem_alloc(sizeof *dkp, MA_SYSLOAD);
	*dkp = *dk;
	p = strchr(dkp->name, 0);
	*p++ = ',';
	itoa(nr, p);
	dkp->offset = start;
	dkp->size   = size;
	
	dkp->next = disk_list;
	disk_list = dkp;
	
	if (dkp->unit == bbd)
	{
		if (disk_ownpart == NULL && type == 0xcc)
			disk_ownpart = dkp;
		if (disk_actpart == NULL && act)
			disk_actpart = dkp;
	}
}

static void disk_cparts(struct disk *dk)
{
	struct pc_part *p, *e;
	char buf[512];
	int i;
	
	if (disk_read(dk, 0, buf))
		return; /* XXX */
	p = (struct pc_part *)(buf + 0x1be);
	e = p + 4;
	i = 0;
	for (; p < e; p++)
		if (p->size)
			disk_cpart(dk, i++, p->offset, p->size, p->type, p->boot & 0x80);
	
	if (disk_actpart)
		disk_boot = disk_actpart;
	if (disk_ownpart)
		disk_boot = disk_ownpart;
}

struct edd_dpb
{
	uint16_t	size;
	uint16_t	flags;
	uint32_t	ncyl;
	uint32_t	nhead;
	uint32_t	nsect;
	uint64_t	blocks;
	uint16_t	bps;
} __attribute__((packed));

static void disk_chk_edd(struct disk *dk)
{
	static struct edd_dpb dpb = { .size = sizeof dpb };
	
	bcp.intr = 0x13;
	bcp.eax	 = 0x4100;
	bcp.ebx	 = 0x55aa;
	bcp.edx	 = dk->unit;
	bioscall();
	
	if (bcp.eflags & 1)
	{
#if DEBUG
		con_puts("disk_chk_edd: int 13/41 failed for drive ");
		con_putx8(dk->unit);
		con_putc('\n');
#endif
		return;
	}
	/* if ((bcp.eax & 0xff00) != 0x3000)
		return; */
	if ((bcp.ebx & 0xffff) != 0xaa55)
	{
#if DEBUG
		con_puts("disk_chk_edd: int 13/41 BX != 0xaa55 for drive ");
		con_putx8(dk->unit);
		con_putc('\n');
#endif
		return;
	}
#if 0
	if (!(bcp.ecx & 4))
	{
#if DEBUG
		con_puts("disk_chk_edd: parameter table bit in CX not set for drive ");
		con_putx8(dk->unit);
		con_putc('\n');
#endif
		return;
	}
#endif
	
	bcp.eax = 0x4800;
	bcp.edx = dk->unit;
	bcp.esi = (intptr_t)&dpb & 15;
	bcp.ds	= (intptr_t)&dpb >> 4;
	bioscall();
	if (bcp.eflags & 1)
	{
#if DEBUG
		con_puts("disk_chk_edd: int 13/48 failed for drive ");
		con_putx8(dk->unit);
		con_putc('\n');
#endif
		return;
	}
	
#if DEBUG
	con_puts("disk_chk_edd: drive ");
	con_putx8(dk->unit);
	con_puts(" is in EDD mode\n");
#endif
	dk->size = dpb.blocks;
	dk->read = disk_read_edd;
}

void disk_init_chs(int unit)
{
	struct disk *dk;
	
#if DEBUG
	con_puts("disk_init_chs: trying drive ");
	con_putx8(unit);
	con_putc('\n');
#endif
	
	bcp.eax	 = 0x0800;
	bcp.edx	 = unit;
	bcp.intr = 0x13;
	bioscall();
	if (unit < 128 && (bcp.edx & 0xff) < unit)
		goto int13fail;
	if (bcp.eflags & 1)
		goto int13fail;
	
	dk = mem_alloc(sizeof *dk, MA_SYSLOAD);
	memset(dk, 0, sizeof *dk);
	dk->ncyl  = (bcp.ecx >> 8) & 0x0ff;
	dk->ncyl |= (bcp.ecx << 2) & 0x300;
	dk->nhead = (bcp.edx >> 8) & 0xff;
	dk->nsect =  bcp.ecx	   & 0x3f;
	dk->ncyl++;
	dk->nhead++;
	dk->size  = dk->ncyl;
	dk->size *= dk->nhead;
	dk->size *= dk->nsect;
	dk->unit  = unit;
	dk->read  = disk_read_chs;
	
	if (unit < 128)
	{
		strcpy(dk->name, "floppy");
		itoa(unit, dk->name + 6);
	}
	else
	{
		strcpy(dk->name, "disk");
		itoa(unit - 128, dk->name + 4);
	}
	
	dk->next  = disk_list;
	disk_list = dk;
	
	if (unit == bbd)
		disk_boot = dk;
	if (unit >= 128)
	{
		disk_chk_edd(dk);
		disk_cparts(dk);
	}
	
#if DEBUG
	con_puts("disk: ");
	con_puts(dk->name);
	con_puts(": ncyl = ");
	con_putn(dk->ncyl);
	con_puts(", nhead = ");
	con_putn(dk->nhead);
	con_puts(", nsect = ");
	con_putn(dk->nsect);
	con_puts(", size = ");
	con_putn(dk->size);
	con_putc('\n');
#endif
	return;
int13fail:
#if DEBUG
	con_puts("disk_init_chs: int13/08 failed for drive ");
	con_putx8(unit);
	con_putc('\n');
#endif
	;
}

static void disk_init_cdrom(int unit)
{
	struct disk *dk;
	
	dk = mem_alloc(sizeof *dk, MA_SYSLOAD);
	memset(dk, 0, sizeof *dk);
	dk->size = 16384; /* XXX */
	dk->unit = unit;
	dk->read = disk_read_cdrom;
	
	strcpy(dk->name, "cdrom0");
	
	dk->next  = disk_list;
	disk_list = dk;
	disk_boot = dk;
	
#if DEBUG
	con_puts("cdrom: ");
	con_puts(dk->name);
	con_puts(": size = ");
	con_putn(dk->size);
	con_putc('\n');
#endif
}

void disk_init(void)
{
	int i;
	
#if DEBUG
	con_setattr(0, 7, 0);
	con_gotoxy(0, 0);
	con_clear();
#endif
	
	for (i = 0; i < 4; i++)
		disk_init_chs(i);
	for (i = 128; i < 132; i++)
		if (bbd != i || !cdrom)
			disk_init_chs(i);
	if (bbd && cdrom)
		disk_init_cdrom(bbd);
	pxe_init();
	
	if (disk_boot == NULL)
		disk_boot = disk_actpart;
	
#if DEBUG
	con_puts("disk_init: done, press any key to continue\n");
	con_getch();
#endif
}
