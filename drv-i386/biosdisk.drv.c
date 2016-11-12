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

#include <kern/machine-i386-pc/bioscall.h>
#include <kern/printk.h>
#include <kern/config.h>
#include <kern/mutex.h>
#include <kern/errno.h>
#include <kern/block.h>
#include <kern/clock.h>
#include <kern/intr.h>
#include <kern/umem.h>
#include <kern/task.h>
#include <kern/lib.h>
#include <kern/hw.h>

#include <devices.h>
#include <bioctl.h>
#include <stdint.h>
#include <os386.h>

#define DISKS			4
#define SECTIONS		5
#define UNITS			(SECTIONS * DISKS)

static int	bd_reset(void);
static int	bd_detect(int d);
static int	bd_load_sectab(int d);
static void	bd_shutdown(int type);

static int	bd_open(int unit);
static int	bd_close(int unit);
static int	bd_read(int unit, blk_t blk, void *buf);
static int	bd_write(int unit, blk_t blk, const void *buf);
static int	bd_ioctl(int unit, int cmd, void *buf);

#define DK_UNIT(i)	{ .refcnt = 0,		\
			  .name	  = NULL,	\
			  .unit	  = (i),	\
			  .open	  = bd_open,	\
			  .close  = bd_close,	\
			  .ioctl  = bd_ioctl,	\
			  .read	  = bd_read,	\
			  .write  = bd_write,	}

static struct bdev bdev[UNITS] =
{
	DK_UNIT(0),
	DK_UNIT(1),
	DK_UNIT(2),
	DK_UNIT(3),
	DK_UNIT(4),
	DK_UNIT(5),
	DK_UNIT(6),
	DK_UNIT(7),
	DK_UNIT(8),
	DK_UNIT(9),
	DK_UNIT(10),
	DK_UNIT(11),
	DK_UNIT(12),
	DK_UNIT(13),
	DK_UNIT(14),
	DK_UNIT(15),
	DK_UNIT(6),
	DK_UNIT(17),
	DK_UNIT(18),
	DK_UNIT(19),
};
#undef DK_UNIT

static struct unit
{
	int use_lba;
	int unit;
	int ncyl;
	int nhead;
	int nsect;
	
	blk_t offset;
	blk_t size;
	
	int os;
} unit[UNITS];

struct part
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
};

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

static void bd_chk_edd(struct unit *u)
{
	struct edd_dpb *dpb = get_bios_buf();
	struct real_regs *regs;
	
	dpb->size = sizeof *dpb;
	
	regs = get_bios_regs();
	regs->intr = 0x13;
	regs->eax  = 0x4100;
	regs->ebx  = 0x55aa;
	regs->edx  = 0x0080 | u->unit;
	v86_call();
	
	if (regs->eflags & 1)
		return;
	
	if ((regs->ebx & 0xffff) != 0xaa55)
		return;
	
	regs->eax = 0x4800;
	regs->edx = 0x0080 | u->unit;
	regs->esi = (uint32_t)dpb & 15;
	regs->ds  = (uint32_t)dpb >> 4;
	v86_call();
	
	if (regs->eflags & 1)
		return;
	
	u->size	   = dpb->blocks;
	u->use_lba = 1;
}

static void bd_init(void)
{
	int i, n;
	int err;
	
	for (i = 0; i < UNITS; i++)
	{
		unit[i].unit		= i / SECTIONS;
		unit[i].size		= 0;
		unit[i].os		= -1;
	}
	
	for (i = 0; i < DISKS; i++)
		if (!bd_detect(i))
		{
			err = blk_install(&bdev[i * SECTIONS]);
			if (err)
			{
				perror("biosdisk.drv: bd_init: blk_install", err);
				continue;
			}
			
			bd_load_sectab(i);
		}
	
#if KVERBOSE
	printk("biosdisk.drv: bd_init: ok\n");
#endif
	on_shutdown(bd_shutdown);
}

static void bd_shutdown(int type)
{
	long tmout;
	int i;
	
	if (type == SHUTDOWN_REBOOT)
		return;
	
	for (i = 0; i < UNITS; i += SECTIONS)
		if (unit[i].size)
		{
#if KVERBOSE
			printk("biosdisk.drv: bd_shutdown: spinning down %s\n", bdev[i].name);
#endif
		}
}

static int bd_reset(void)
{
	return 0;
}

static int bd_detect(int d)
{
	struct unit *u = &unit[d * SECTIONS];
	struct real_regs *regs;
	unsigned short buf[256];
	long t;
	int i;
	
#if KVERBOSE
	printk("biosdisk.drv: detect(%i): ", d);
#endif
	
	if (bd_reset())
	{
#if KVERBOSE
		printk("reset failed\n");
#endif
		return ENODEV;
	}
	
	regs = get_bios_regs();
	
	regs->ax   = 0x0800;
	regs->dx   = 0x0080 | d;
	regs->intr = 0x13;
	
	v86_call();
	
	if (regs->ax & 0xff00)
		goto nodisk;
	
	if (regs->flags & 1)
		goto nodisk;
	
	u->ncyl	  =  regs->cx >> 8;
	u->ncyl	 |= (regs->cx << 2) & 0x300;
	u->ncyl++;
	
	u->nhead  = (regs->dx >> 8) + 1;
	u->nsect  = (regs->cx & 63);
	
	u->size	  = u->ncyl * u->nhead * u->nsect;
	u->offset = 0;
	
	if (!u->size)
		goto nodisk;
	
	bd_chk_edd(u);
#if KVERBOSE
	printk("%i cyls, %i heads, %i sects (%li blocks, LBA %s)\n",
		u->ncyl, u->nhead, u->nsect, (long)u->size,
		u->use_lba ? "enabled" : "disabled");
#endif
	return 0;
nodisk:
#if KVERBOSE
	printk("no disk detected\n");
#endif
	return ENODEV;
}

static int bd_load_sectab(int d)
{
	struct part *part;
	char buf[512];
	int err;
	int i;
	
#if KVERBOSE
	printk("biosdisk.drv: bd_load_sectab(%i):\n", d);
#endif
	
	err = bd_read(d * SECTIONS, 0, buf);
	if (err)
		goto err;
	
	part = (void *)(buf + 0x1be);
	for (i = 0; i < 4; i++)
		if (part[i].size)
		{
			unit[d * SECTIONS + i + 1]	  = unit[d * SECTIONS];
			unit[d * SECTIONS + i + 1].offset = part[i].offset;
			unit[d * SECTIONS + i + 1].size	  = part[i].size;
			unit[d * SECTIONS + i + 1].os	  = part[i].type;
			
#if KVERBOSE
			printk(" partition %i: %i ... %i\n", i, part[i].offset, part[i].size + part[i].offset - 1);
#endif
			err = blk_install(&bdev[d * SECTIONS + i + 1]);
			if (err)
				perror("biosdisk.drv: bd_load_sectab: blk_install", err);
		}
#if KVERBOSE
	printk("biosdisk.drv: bd_load_sectab(%i): done\n", d);
#endif
	return 0;
err:
	perror("biosdisk.drv: bd_load_sectab", err);
	return err;
}

static int bd_open(int u)
{
	return 0;
}

static int bd_close(int u)
{
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

static int bd_read_edd(int u, blk_t blk, void *buf)
{
	struct real_regs *regs = get_bios_regs();
	struct edd_dap *dap = get_bios_buf();
	void *bbuf = dap + 1;
	
	dap->size    = sizeof *dap,
	dap->count   = 1,
	dap->buf_off = (uintptr_t)bbuf & 15;
	dap->buf_seg = (uintptr_t)bbuf >> 4;
	dap->blk     = blk;
	
	regs->intr = 0x13;
	regs->eax  = 0x4200;
	regs->edx  = 0x0080 | unit[u].unit;
	regs->esi  = (uintptr_t)dap & 15;
	regs->ds   = (uintptr_t)dap >> 4;
	v86_call();
	
	if (regs->eflags & 1)
		return EIO;
	
	memcpy(buf, bbuf, 512);
	return 0;
}

static int bd_write_edd(int u, blk_t blk, void *buf)
{
	struct real_regs *regs = get_bios_regs();
	struct edd_dap *dap = get_bios_buf();
	void *bbuf = dap + 1;
	
	memcpy(bbuf, buf, 512);
	
	dap->size    = sizeof *dap,
	dap->count   = 1,
	dap->buf_off = (uintptr_t)bbuf & 15;
	dap->buf_seg = (uintptr_t)bbuf >> 4;
	dap->blk     = blk;
	
	regs->intr = 0x13;
	regs->eax  = 0x4300;
	regs->edx  = 0x0080 | unit[u].unit;
	regs->esi  = (uintptr_t)dap & 15;
	regs->ds   = (uintptr_t)dap >> 4;
	v86_call();
	
	if (regs->eflags & 1)
		return EIO;
	
	return 0;
}

static int bd_read(int u, blk_t blk, void *buf)
{
	struct real_regs *regs;
	void *bbuf;
	unsigned c, h, s, t;
	int err = 0;
	int i;
	
	if (blk >= unit[u].size)
		return EIO;
	blk += unit[u].offset;
	
	if (unit[u].use_lba)
		return bd_read_edd(u, blk, buf);
	
	s = blk % unit[u].nsect;
	t = blk / unit[u].nsect;
	h = t   % unit[u].nhead;
	c = t   / unit[u].nhead;
	s++;
	
	regs = get_bios_regs();
	bbuf = get_bios_buf();
	
	regs->ax   = 0x0201;
	regs->cx   = (c << 8) | ((c >> 2) & 0xc0) | s;
	regs->dx   = 0x0080 | unit[u].unit | h << 8;
	regs->intr = 0x13;
	regs->es   = (intptr_t)bbuf >> 4;
	regs->bx   = (intptr_t)bbuf & 15;
	
	v86_call();
	
	if (regs->flags & 1)
		return EIO;
	
	memcpy(buf, bbuf, 512);
	return 0;
}

static int bd_write(int u, blk_t blk, const void *buf)
{
	struct real_regs *regs;
	void *bbuf;
	unsigned c, h, s, t;
	int err = 0;
	int i;
	
	if (blk >= unit[u].size)
		return EIO;
	blk += unit[u].offset;
	
	if (unit[u].use_lba)
		return bd_write_edd(u, blk, buf);
	
	s = blk % unit[u].nsect;
	t = blk / unit[u].nsect;
	h = t   % unit[u].nhead;
	c = t   / unit[u].nhead;
	s++;
	
	regs = get_bios_regs();
	bbuf = get_bios_buf();
	
	memcpy(bbuf, buf, 512);
	
	regs->ax   = 0x0301;
	regs->cx   = (c << 8) | ((c >> 2) & 0xc0) | s;
	regs->dx   = 0x0080 | unit[u].unit | h << 8;
	regs->intr = 0x13;
	regs->es   = (intptr_t)bbuf >> 4;
	regs->bx   = (intptr_t)bbuf & 15;
	
	v86_call();
	
	if (regs->flags & 1)
		return EIO;
	
	return 0;
}

static int bd_ioctl(int u, int cmd, void *buf)
{
	struct bio_info bi;
	
	switch (cmd)
	{
	case BIO_INFO:
		bi.type	  = unit[u].offset ? BIO_TYPE_HD_SECTION : BIO_TYPE_HD;
		bi.os	  = unit[u].os;
		bi.offset = unit[u].offset;
		bi.size	  = unit[u].size;
		bi.ncyl	  = unit[u].ncyl;
		bi.nhead  = unit[u].nhead;
		bi.nsect  = unit[u].nsect;
		return tucpy(buf, &bi, sizeof bi);
	default:
		return ENOTTY;
	}
}

static int bd_mknames(char *name)
{
	struct bdev *bdp;
	unsigned nsize;
	unsigned size;
	char *p;
	int i, n;
	int err;
	
	nsize = strlen(name) + 4;
	size  = nsize * UNITS;
	
	err = kmalloc(&p, size, "hdnames");
	if (err)
		perror("bd_mknames", err);
	
	bdp = bdev;
	for (n = 0; n < DISKS; n++)
		for (i = 0; i < SECTIONS; i++)
		{
			strcpy(p, name);
			p[nsize - 4] = ',';
			p[nsize - 3] = '0' + n;
			p[nsize - 2] = i ? 'a' + i - 1 : 0;
			p[nsize	- 1] = 0;
			bdp++->name = p;
			p += nsize;
		}
	
	return 0;
}

int mod_onload(unsigned md, char *pathname, struct device *dev, unsigned dev_size)
{
	int err;
	
	err = bd_mknames(dev->name);
	if (err)
		return err;
	bd_init();
	return 0;
}

int mod_onunload(unsigned md)
{
	return ENOSYS;
}
