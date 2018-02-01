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

#include <kern/printk.h>
#include <kern/config.h>
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

#define HD_RESET_TIMEOUT	2
#define HD_DETECT_TIMEOUT	2
#define HD_RESPONSE_TIMEOUT	10
#define HD_RETRIES		10

static void	hd_irqv();

static int	reset(void);
static int	detect(int d);
static int	load_sectab(int d);
static void	hd_shutdown(int type);

static int	open(int unit);
static int	close(int unit);
static int	hd_read(int unit, blk_t blk, void *buf);
static int	hd_write(int unit, blk_t blk, const void *buf);
static int	ioctl(int unit, int cmd, void *buf);

static int	wait_drq(int u);

static volatile int hd_irq;

static unsigned hd_iobase0;
static unsigned hd_iobase1;
static unsigned hd_bmbase;
static int	hd_bus_master;

#define DK_UNIT(i)	{ .refcnt = 0,		\
			  .name	  = NULL,	\
			  .unit	  = (i),	\
			  .open	  = open,	\
			  .close  = close,	\
			  .ioctl  = ioctl,	\
			  .read   = hd_read,	\
			  .write  = hd_write	}

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
	DK_UNIT(16),
	DK_UNIT(17),
	DK_UNIT(18),
	DK_UNIT(19),
};
#undef DK_UNIT

static struct unit
{
	unsigned iobase;
	
	int use_lba;
	int chan;
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

static int hd_init(void)
{
	int i, n;
	int err;
	
	for (i = 0; i < SECTIONS; i++)
		for (n = 0; n < DISKS; n++)
		{
			unit[i + n * SECTIONS].unit	= n & 1;
			unit[i + n * SECTIONS].size	= 0;
			unit[i + n * SECTIONS].os	= -1;
			
			if (n & 2)
			{
				unit[i + n * SECTIONS].iobase = hd_iobase1;
				unit[i + n * SECTIONS].chan   = 1;
			}
			else
			{
				unit[i + n * SECTIONS].iobase = hd_iobase0;
				unit[i + n * SECTIONS].chan   = 0;
			}
		}
	
	irq_set(14, hd_irqv);
	irq_set(15, hd_irqv);
	
	irq_ena(14);
	irq_ena(15);
	
	for (i = 0; i < DISKS; i++)
		if (!detect(i))
		{
			err = blk_install(&bdev[i * SECTIONS]);
			if (err)
			{
				perror("pciide: hd_init: blk_install", err);
				continue;
			}
			
			load_sectab(i);
		}
	
#if KVERBOSE
	printk("pciide: hd_init: ok\n");
#endif
	on_shutdown(hd_shutdown);
}

static void hd_shutdown(int type)
{
	long tmout;
	int i;
	
	if (type == SHUTDOWN_REBOOT)
		return;
	
	for (i = 0; i < UNITS; i += SECTIONS)
		if (unit[i].size)
		{
			unsigned iobase = unit[i].iobase;
			
#if KVERBOSE
			printk("pciide: hd_shutdown: waiting for %s to spin down\n", bdev[i].name);
#endif
			outb(iobase + 0x6, 0xc0 | (unit[i].unit << 4));
			outb(iobase + 0x7, 0xe6);
			tmout = clock_time() + HD_RESPONSE_TIMEOUT;
			while (tmout > clock_time() && !hd_irq);
			if (!hd_irq)
			{
				printk("pciide: hd_shutdown: disk not responding\n");
				continue;
			}
			if (inb(iobase + 0x6) & 1)
				printk("pciide: hd_shutdown: disk error\n");
		}
}

static void hd_irqv()
{
	hd_irq = 1;
}

static int reset1(unsigned iobase)
{
	long t;
	
	t = clock_time();
	outb(iobase + 0x0206, 0x04);
	outb(iobase + 0x0206, 0x00);
	while (t + HD_RESET_TIMEOUT > clock_time() && (inb(iobase + 0x7) & 0x80));
	if (inb(iobase + 0x7) & 0x80)
		return ENODEV;
	return 0;
}

static int reset(void)
{
	reset1(hd_iobase0);
	reset1(hd_iobase1);
}

static int detect(int d)
{
	struct unit *u = &unit[d * SECTIONS];
	unsigned short buf[256];
	unsigned iobase = u->iobase;
	long t;
	int i;
	
#if KVERBOSE
	printk("pciide: detect(%i): ", d);
#endif
	
	if (reset1(iobase))
	{
#if KVERBOSE
		printk("reset failed\n");
#endif
		return ENODEV;
	}
	
	hd_irq = 0;
	
	outb(iobase + 0x6, (u->unit << 4) | (u->use_lba << 6) | 0xa0);
	outb(iobase + 0x7, 0xec);
	
	t = clock_time();
	while (t + HD_DETECT_TIMEOUT > clock_time() && !hd_irq);
	
	if (!hd_irq || (inb(iobase + 0x7) & 0x01))
	{
#if KVERBOSE
		printk("no disk detected\n");
#endif
		return ENODEV;
	}
	
	inb(iobase + 0x7);
	
	for (i = 0; i < 256; i++)
		buf[i] = inw(iobase);
	
	u->iobase = iobase;
	u->ncyl	  = buf[1];
	u->nhead  = buf[3];
	u->nsect  = buf[6];
	u->size	  = u->ncyl * u->nhead * u->nsect;
	u->offset = 0;
	if (!u->size)
	{
#if KVERBOSE
		printk("no disk detected\n");
#endif
		return ENODEV;
	}
	
	if ((buf[49] & 256) && (buf[60] || buf[61]))
	{
		u->size = buf[60] + (buf[61] << 16);
		u->use_lba = 1;
		if (u->size > 0x10000000) /* 2**28 */
			u->size = 0x10000000;
	}
	else
		u->use_lba = 0;
	
#if KVERBOSE
	printk("%i cyls, %i heads, %i sects (%li blocks, LBA %s)\n",
		u->ncyl, u->nhead, u->nsect, (long)u->size,
		u->use_lba ? "enabled" : "disabled");
#endif
	
	reset();
	return 0;
}

static int load_sectab(int d)
{
	struct part *part;
	char buf[512];
	int err;
	int i;
	
#if KVERBOSE
	printk("pciide: load_sectab(%i):\n", d);
#endif
	
	err = hd_read(d * SECTIONS, 0, buf);
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
				perror("pciide: hd_init: blk_install", err);
		}
#if KVERBOSE
	printk("pciide: load_sectab(%i): done\n", d);
#endif
	return 0;
err:
	perror("pciide: load_sectab", err);
	return err;
}

static int open(int u)
{
	return 0;
}

static int close(int u)
{
	return 0;
}

static int wait_drdy(int u)
{
	struct unit *up = &unit[u];
	long tmout;
	int st;
	
	outb(up->iobase + 6, (up->unit << 4) | (up->use_lba << 6) | 0xa0);
	tmout = clock_time() + HD_RESPONSE_TIMEOUT;
	do
		st = inb(up->iobase + 7);
	while (tmout > clock_time() && !(st & 65));
	if (st & 1)
	{
		printk("pciide: wait_drdy: device error, status = 0x%x\n", st);
		return EIO;
	}
	if (st & 64)
		return 0;
	printk("pciide: wait_drdy: device time out, status = 0x%x\n", st);
	return EIO;
}

static int wait_drq(int u)
{
	struct unit *up = &unit[u];
	long tmout;
	int st;
	
	outb(up->iobase + 6, (up->unit << 4) | (up->use_lba << 6) | 0xa0);
	tmout = clock_time() + HD_RESPONSE_TIMEOUT;
	do
		st = inb(up->iobase + 7);
	while (tmout > clock_time() && !(st & 9));
	if (st & 1)
	{
		printk("pciide: wait_drq: device error, status = 0x%x\n", st);
		return EIO;
	}
	if (st & 8)
		return 0;
	printk("pciide: wait_drq: device time out, status = 0x%x\n", st);
	return EIO;
}

static int hd_wait_irq(void)
{
	long tmout;
	int s;
	
	tmout = clock_time() + HD_RESPONSE_TIMEOUT;
	
	s = intr_dis();
	while (tmout > clock_time() && !hd_irq)
		asm volatile("sti; hlt; cli;");
	intr_res(s);
	
	if (hd_irq)
		return 0;
	
	printk("pciide: wait_irq: disk not responding\n");
	return EIO;
}

static int hd_read(int u, blk_t blk, void *buf)
{
	unsigned iobase = unit[u].iobase;
	unsigned c, h, s, t;
	int err = 0;
	int i;
	
	hd_irq = 0;
	
	if (blk >= unit[u].size)
		return EIO;
	blk += unit[u].offset;
	
	if (!unit[u].use_lba)
	{
		s = blk % unit[u].nsect;
		t = blk / unit[u].nsect;
		h = t   % unit[u].nhead;
		c = t   / unit[u].nhead;
		s++;
	}
	
	err = wait_drdy(u);
	if (err)
		return err;
	
	outb(iobase + 0x2, 1);
	if (unit[u].use_lba)
	{
		outb(iobase + 0x3,  blk);
		outb(iobase + 0x4,  blk >> 8);
		outb(iobase + 0x5,  blk >> 16);
		outb(iobase + 0x6, (blk >> 24) | (unit[u].unit << 4) | 0xe0);
	}
	else
	{
		outb(iobase + 0x3, s);
		outb(iobase + 0x4, c);
		outb(iobase + 0x5, c >> 8);
		outb(iobase + 0x6, h | (unit[u].unit << 4) | 0xa0);
	}
	outb(iobase + 0x7, 0x20);
	
	err = hd_wait_irq();
	if (err)
		return err;
	
	if (inb(iobase + 0x7) & 0x01)
		return EIO;
	
	err = wait_drq(u);
	if (err)
		return err;
	
	insw(iobase, buf, 256);
	return 0;
}

static int hd_write(int u, blk_t blk, const void *buf)
{
	unsigned iobase = unit[u].iobase;
	unsigned c, h, s, t;
	int err = 0;
	int i;
	
	hd_irq = 0;
	
	if (blk >= unit[u].size)
		return EIO;
	blk += unit[u].offset;
	
	if (!unit[u].use_lba)
	{
		s = blk % unit[u].nsect;
		t = blk / unit[u].nsect;
		h = t % unit[u].nhead;
		c = t / unit[u].nhead;
		s++;
	}
	
	if (wait_drdy(u))
		return EIO;
	outb(iobase + 0x2, 1);
	if (unit[u].use_lba)
	{
		outb(iobase + 0x3, blk);
		outb(iobase + 0x4, blk >> 8);
		outb(iobase + 0x5, blk >> 16);
		outb(iobase + 0x6, (blk >> 24) | (unit[u].unit << 4) | 0xe0);
	}
	else
	{
		outb(iobase + 0x3, s);
		outb(iobase + 0x4, c);
		outb(iobase + 0x5, c >> 8);
		outb(iobase + 0x6, h | (unit[u].unit << 4) | 0xa0);
	}
	outb(iobase + 0x7, 0x30);
	
	err = wait_drq(u);
	if (err)
		return err;
	outsw(iobase, buf, 256);
	
	err = hd_wait_irq();
	if (err)
		return err;
	
	if (inb(iobase + 0x7) & 0x01)
		return EIO;
	return 0;
}


static int ioctl(int u, int cmd, void *buf)
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

static int hd_mknames(char *name)
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
		perror("hd_mknames", err);
	
	bdp = bdev;
	for (n = 0; n < DISKS; n++)
		for (i = 0; i < SECTIONS; i++)
		{
			strcpy(p, name);
			p[nsize - 4] = ',';
			p[nsize - 3] = '0' + n;
			p[nsize - 2] = i ? 'a' + i - 1 : 0;
			p[nsize - 1] = 0;
			bdp++->name = p;
			p += nsize;
		}
	
	return 0;
}

int mod_onload(unsigned md, char *pathname, struct device *dev, unsigned dev_size)
{
	int err;
	
	err = pci_configure(dev);
	if (err)
		return err;
	
	hd_iobase0 = dev->io_base[0];
	hd_iobase1 = dev->io_base[2];
	
	if (dev->io_size[4])
	{
		hd_bmbase = dev->io_base[4];
		hd_bus_master = 1;
	}
	
	err = hd_mknames(dev->name);
	if (err)
		return err;
	
	hd_init();
	return 0;
}

int mod_onunload(unsigned md)
{
	return EPERM;
}
