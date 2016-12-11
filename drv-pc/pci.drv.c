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

#include <sysload/kparam.h>
#include <sysload/flags.h>
#include <kern/limits.h>
#include <kern/printk.h>
#include <kern/errno.h>
#include <kern/start.h>
#include <kern/lib.h>
#include <kern/cio.h>
#include <kern/fs.h>
#include <dev/pci.h>
#include <sys/types.h>
#include <overflow.h>
#include <devices.h>

#define VERBOSE	KVERBOSE

static int pci_open(struct cdev *cd);
static int pci_close(struct cdev *cd);
static int pci_read(struct cio_request *rq);
static int pci_write(struct cio_request *rq);
static int pci_ioctl(struct cdev *cd, int cmd, void *buf);

static char pci_cdev_name[NAME_MAX + 1];

static struct cdev pci_cdev =
{
	.name	= pci_cdev_name,
	.open	= pci_open,
	.close	= pci_close,
	.read	= pci_read,
	.write	= pci_write,
	.ioctl	= pci_ioctl,
};

static struct pci_dev *	pci_dev;
static int		pci_dev_cnt;

static int pci_open(struct cdev *cd)
{
	cd->fso->size = pci_dev_cnt * sizeof *pci_dev;
	return 0;
}

static int pci_close(struct cdev *cd)
{
	return 0;
}

static int pci_read(struct cio_request *rq)
{
	uoff_t max = pci_dev_cnt * sizeof *pci_dev;
	
	if (ov_add_uoff(rq->offset, rq->count))
		return EINVAL;
	
	if (rq->offset > max)
	{
		rq->count = 0;
		return 0;
	}
	
	if (rq->offset + rq->count > max)
		rq->count = max - rq->offset;
	
	memcpy(rq->buf, (char *)pci_dev + rq->offset, rq->count);
	return 0;
}

static int pci_write(struct cio_request *rq)
{
	return EPERM;
}

static int pci_ioctl(struct cdev *cd, int cmd, void *buf)
{
	return ENOTTY;
}

unsigned pci_read_reg(int bus, int dev, int func, unsigned reg)
{
	outl(0xcf8, 0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | (reg << 2));
	return inl(0xcfc);
}

void pci_write_reg(int bus, int dev, int func, unsigned reg, unsigned data)
{
	outl(0xcf8, 0x80000000 | (bus << 16) | (dev << 11) | (func << 8) | (reg << 2));
	outl(0xcfc, data);
}

static void count_pci_devs(void)
{
	int bus, dev, func;
	uint32_t w0, w3;
	
	pci_dev_cnt = 0;
	for (bus = 0; bus < 256; bus++)
		for (dev = 0; dev < 32; dev++)
			for (func = 0; func < 8; func++)
			{
				w0 = pci_read_reg(bus, dev, func, 0);
				w3 = pci_read_reg(bus, dev, func, 3);
				
				if ((w0 & 0xffff) == 0xffff)
					continue;
				if (w3 & 0xff0000)
					continue;
				pci_dev_cnt++;
			}
}

static void fix_device(struct pci_dev *p)
{
	if ((p->dev_class & 0xffff00) == 0x010100) /* IDE Controller */
	{
		if (p->mem_base[0] < 2)
		{
			p->mem_base[0] = 0x1f0;
			p->mem_size[0] = 8;
			p->is_io[0]    = 1;
		}
		
		if (p->mem_base[1] < 2)
		{
			p->mem_base[1] = 0x3f6;
			p->mem_size[1] = 1;
			p->is_io[1]    = 1;
		}
		
		if (p->mem_base[2] < 2)
		{
			p->mem_base[2] = 0x170;
			p->mem_size[2] = 8;
			p->is_io[2]    = 1;
		}
		
		if (p->mem_base[3] < 2)
		{
			p->mem_base[3] = 0x376;
			p->mem_size[3] = 1;
			p->is_io[3]    = 1;
		}
		
		return;
	}
}

static void config_pci_dev(struct pci_dev *p, int bus, int dev, int func)
{
	uint32_t w0, w2, w15;
	int irq;
	int i;
	
	w0  = pci_read_reg(bus, dev, func,  0);
	w2  = pci_read_reg(bus, dev, func,  2);
	w15 = pci_read_reg(bus, dev, func, 15);
	
	p->bus	     = bus;
	p->dev	     = dev;
	p->func	     = func;
	
	p->dev_class = w2 >> 8;
	p->dev_vend  = w0 & 0xffff;
	p->dev_rev   = w2 & 0xff;
	p->dev_id    = w0 >> 16;
	p->intr	     = (w15 >> 8) & 0x07;
	p->irq	     = w15 & 0xff;
	
#if VERBOSE
	printk("found a PCI dev, bus=%i, dev=%i, func=%i\n", bus, dev, func);
	printk("  vendor = 0x%x\n", (unsigned)p->dev_vend);
	printk("  id     = 0x%x\n", (unsigned)p->dev_id);
	printk("  class  = 0x%x\n", (unsigned)p->dev_class);
	printk("  rev    = 0x%x\n", (unsigned)p->dev_rev);
	printk("  IRQ    = %i\n", irq);
	printk("  intr   = %c\n", "-ABCD"[p->intr]);
#endif
	
	for (i = 0; i < 6; i++)
	{
		uint32_t bar = pci_read_reg(bus, dev, func, 4 + i);
		uint32_t base;
		uint32_t size;
		
		pci_write_reg(bus, dev, func, 4 + i, -1U);
		
		size = ~pci_read_reg(bus, dev, func, 4 + i);
		
		pci_write_reg(bus, dev, func, 4 + i, bar);
		
		if (bar & 1)
		{
			base  = bar & 0xfffffff8;
			size |= 7;
			size++;
		}
		else
		{
			base  = bar & 0xfffffff0;
			size |= 15;
			size++;
		}
		
		p->mem_base[i] = base;
		p->mem_size[i] = size;
		p->is_io[i]    = 0;
		
		if (bar & 1)
		{
			p->mem_base[i] &= 0xffff;
			p->mem_size[i] &= 0xffff;
			p->is_io[i]	= 1;
		}
	}
	
	fix_device(p);
	
#if VERBOSE
	for (i = 0; i < 6; i++)
	{
		uint32_t base = p->mem_base[i];
		uint32_t size = p->mem_size[i];
		
		if (!size)
			continue;
		
		if (p->is_io[i])
			printk("  I/O #%i: %x, size %x\n", i, base, size);
		else
			printk("  Mem #%i: %x, size %x\n", i, base, size);
	}
	printk("\n");
#endif
}

int pci_configure(struct device *dv)
{
	int i, n;
	
	for (i = 0; i < pci_dev_cnt; i++)
	{
		if (dv->pci_bus != pci_dev[i].bus)
			continue;
		if (dv->pci_dev != pci_dev[i].dev)
			continue;
		if (dv->pci_func != pci_dev[i].func)
			continue;
		
		for (n = 0; n < 6; n++)
		{
			if (pci_dev[i].is_io[n])
			{
				dv->io_base[n] = pci_dev[i].mem_base[n];
				dv->io_size[n] = pci_dev[i].mem_size[n];
			}
			else
			{
				dv->mem_base[n] = pci_dev[i].mem_base[n];
				dv->mem_size[n] = pci_dev[i].mem_size[n];
			}
		}
		
		return 0;
	}
	
	return ENODEV;
}

static void pci_scan(void)
{
	struct pci_dev *p;
	int bus, dev, func;
	uint32_t w0, w3;
	int err;
	
	if (kparam.boot_flags & BOOT_VERBOSE)
		printk("pci: configuring devices\n");
	count_pci_devs();
	
	if (!pci_dev_cnt)
	{
		if (kparam.boot_flags & BOOT_VERBOSE)
			printk("pci: no devices found\n");
		return;
	}
	if (kparam.boot_flags & BOOT_VERBOSE)
		printk("pci: found %i devices\n", pci_dev_cnt);
	
	err = kmalloc(&pci_dev, sizeof *pci_dev * pci_dev_cnt, "pci_dev");
	if (err)
	{
		perror("pci: kmalloc", err);
		return;
	}
	
	memset(pci_dev, 0, sizeof *pci_dev * pci_dev_cnt);
	p = pci_dev;
	
	for (bus = 0; bus < 256; bus++)
		for (dev = 0; dev < 32; dev++)
			for (func = 0; func < 8; func++)
			{
				w0 = pci_read_reg(bus, dev, func, 0);
				w3 = pci_read_reg(bus, dev, func, 3);
				
				if ((w0 & 0xffff) == 0xffff)
					continue;
				if (w3 & 0xff0000)
					continue;
				config_pci_dev(p++, bus, dev, func);
			}
}

int mod_onload(unsigned md, char *pathname, struct device *dev, unsigned dev_size)
{
	pci_scan();
	if (!pci_dev_cnt)
		return ENODEV;
	
	strcpy(pci_cdev_name, dev->name);
	cio_install(&pci_cdev);
	return 0;
}

int mod_onunload(unsigned md)
{
	return EBUSY;
}
