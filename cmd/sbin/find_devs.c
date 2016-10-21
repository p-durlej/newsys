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

#include <machine/sysinstall.h>
#include <machine/utsname.h>
#include <machine/machdef.h>
#include <dev/pci.h>
#include <devices.h>
#include <wingui.h>
#include <sys/io.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <os386.h>
#include <paths.h>
#include <ctype.h>
#include <time.h>
#include <err.h>

static struct pci_dev *pci_devs;
static int pci_dev_cnt;
static int *pci_dev_active;

static struct device *devs;
static int dev_count;

static char *desktop	= "";
static char *ddb	= "/etc/devices";

static void load_devs(char *dbname)
{
	struct stat st;
	int fd;
	
	fd = open(dbname, O_RDONLY);
	if (fd < 0)
	{
		if (errno != ENOENT)
			warn("%s", dbname);
		return;
	}
	fstat(fd, &st);
	
	dev_count = st.st_size / sizeof(struct device);
	devs = malloc(st.st_size);
	if (!devs)
		err(errno, "malloc");
	if (read(fd, devs, st.st_size) < 0)
		err(errno, "%s", dbname);
	close(fd);
}

static void save_devs(char *dbname)
{
	int fd;
	
	fd = open(dbname, O_CREAT | O_TRUNC | O_WRONLY, 0600);
	if (fd < 0)
	{
		fprintf(stderr, "find_devs: %s: %m\n", dbname);
		exit(errno);
	}
	if (dev_count && write(fd, devs, dev_count * sizeof *devs) < 0)
	{
		fprintf(stderr, "find_devs: %s: %m\n", dbname);
		exit(errno);
	}
	close(fd);
}

static int load_dev(struct device *dev)
{
	int md;
	int fd;
	
	if (*dev->desktop_name && strcmp(dev->desktop_name, desktop))
		return 0;
	md = _mod_load(dev->driver, dev, sizeof *dev);
	if (md < 0)
	{
		int se = errno;
		
		warn("%s", dev->driver);
		errno = se;
		return -1;
	}
	
	fd = open(_PATH_V_DEVICES, O_APPEND | O_CREAT | O_WRONLY, 0600);
	if (fd < 0)
		return 0;
	dev->md = md;
	write(fd, dev, sizeof *dev);
	dev->md = 0;
	close(fd);
	
	sleep(1);
	return 0;
}

static int add_dev(struct device *dev)
{
	struct device dev2 = *dev;
	
	if (load_dev(&dev2))
		return -1;
	
	printf("find_devs: found new hardware: \"%s\" (\"%s\")\n", dev->name, dev->type);
	
	devs = realloc(devs, (dev_count + 1) * sizeof *devs);
	if (!devs)
	{
		perror("find_devs: realloc");
		exit(errno);
	}
	devs[dev_count++] = *dev;
	return 0;
}

static int is_mem_free(unsigned base, unsigned size)
{
	int i, n;
	
	for (i = 0; i < pci_dev_cnt; i++)
	{
		if (!pci_dev_active[i])
			continue;
		
		for (n = 0; n < 6; n++)
		{
			if (pci_devs[i].is_io[n])
				continue;
			if (base >= pci_devs[i].mem_base[n] + pci_devs[i].mem_size[n])
				continue;
			if (base + size <= pci_devs[i].mem_base[n])
				continue;
			return 0;
		}
	}
	
	for (i = 0; i < dev_count; i++)
		for (n = 0; n < DEV_MEM_COUNT; n++)
		{
			if (base >= devs[i].mem_base[n] + devs[i].mem_size[n])
				continue;
			if (base + size <= devs[i].mem_base[n])
				continue;
			return 0;
		}
	return 1;
}

static int is_port_free(unsigned base, unsigned size)
{
	int i, n;
	
	for (i = 0; i < pci_dev_cnt; i++)
	{
		if (!pci_dev_active[i])
			continue;
		
		for (n = 0; n < 6; n++)
		{
			if (!pci_devs[i].is_io[n])
				continue;
			if (base >= pci_devs[i].mem_base[n] + pci_devs[i].mem_size[n])
				continue;
			if (base + size <= pci_devs[i].mem_base[n])
				continue;
			return 0;
		}
	}
	
	for (i = 0; i < dev_count; i++)
		for (n = 0; n < DEV_IO_COUNT; n++)
		{
			if (base >= devs[i].io_base[n] + devs[i].io_size[n])
				continue;
			if (base + size <= devs[i].io_base[n])
				continue;
			return 0;
		}
	return 1;
}

static int is_dma_free(unsigned nr)
{
	int i, n;
	
	for (i = 0; i < dev_count; i++)
		for (n = 0; n < DEV_DMA_COUNT; n++)
			if (nr == devs[i].dma_nr[n])
				return 0;
	return 1;
}

static int is_irq_free(unsigned nr)
{
	int i, n;
	
	for (i = 0; i < pci_dev_cnt; i++)
	{
		if (!pci_dev_active[i])
			continue;
		
		if (pci_devs[i].irq == nr)
			return 0;
	}
	
	for (i = 0; i < dev_count; i++)
		for (n = 0; n < DEV_IRQ_COUNT; n++)
			if (nr == devs[i].irq_nr[n])
				return 0;
	return 1;
}

static int is_drv_listed(const char *pathname)
{
	int i;
	
	for (i = 0; i < dev_count; i++)
		if (!strcmp(devs[i].driver, pathname))
			return 1;
	return 0;
}

static int is_name_free(const char *name)
{
	int i;

	for (i = 0; i < dev_count; i++)
		if (!strcmp(devs[i].name, name))
			return 0;
	return 1;
}

static int is_pci_free(int bus, int dev, int func)
{
	int i;
	
	for (i = 0; i < dev_count; i++)
		if (devs[i].pci_bus  == bus &&
		    devs[i].pci_dev  == dev &&
		    devs[i].pci_func == func)
			return 0;
	return 1;
}

static struct pci_driver
{
	struct pci_driver *next;
	
	const char *pathname;
	const char *desc;
	const char *name;
	
	uint32_t class, class_mask;
	int vendor, product;
} *pci_drivers;

static int load_pci_driver_info(void)
{
	const char *vid, *pid, *class, *cmask;
	struct pci_driver *drv;
	char buf[4096];
	char *p;
	FILE *f;
	
	f = fopen("/lib/drv/pcidevs", "r");
	if (!f)
		return -1;
	
	while (fgets(buf, sizeof buf, f))
	{
		p = strchr(buf, '\n');
		if (p)
			*p = NULL;
		
		p = buf;
		while (isspace(*p))
			p++;
		
		if (!*p || *p == '#')
			continue;
		
		drv = calloc(sizeof *drv, 1);
		if (!drv)
		{
			warn("calloc");
			fclose(f);
			return -1;
		}
		
		drv->name	= strtok(p,	"\t ");
		vid		= strtok(NULL,	"\t ");
		pid		= strtok(NULL,	"\t ");
		class		= strtok(NULL,	"\t ");
		cmask		= strtok(NULL,	"\t ");
		drv->pathname	= strtok(NULL,	"\t");
		drv->desc	= strtok(NULL,	"");
		
		drv->vendor	= strtoul(vid,	 NULL, 0);
		drv->product	= strtoul(pid,	 NULL, 0);
		drv->class	= strtoul(class, NULL, 0);
		drv->class_mask	= strtoul(cmask, NULL, 0);
		
		if (!drv->desc)
		{
			free(drv);
			continue;
		}
		
		drv->name	= strdup(drv->name);
		drv->pathname	= strdup(drv->pathname);
		drv->desc	= strdup(drv->desc);
		
		drv->next = pci_drivers;
		pci_drivers = drv;
	}
	
	fclose(f);
	return 0;
}

static void find_pci_devs(void)
{
	struct pci_dev *pdv;
	struct stat st;
	ssize_t cnt;
	int fd;
	int i, n;
	
	if (load_pci_driver_info())
		return;
	
	fd = open("/dev/pci", O_RDONLY);
	if (fd < 0)
	{
		if (errno != ENOENT)
			warn("/dev/pci");
		return;
	}
	
	if (fstat(fd, &st))
	{
		warn("/dev/pci");
		close(fd);
		return;
	}
	
	pci_devs = malloc(st.st_size);
	if (!pci_devs)
	{
		warn("/dev/pci");
		close(fd);
		return;
	}
	pci_dev_cnt = st.st_size / sizeof *pci_devs;
	
	pci_dev_active = calloc(sizeof *pci_dev_active, pci_dev_cnt);
	if (!pci_dev_active)
	{
		warn("/dev/pci");
		close(fd);
		return;
	}
	
	cnt = read(fd, pci_devs, st.st_size);
	if (cnt < 0)
	{
		warn("/dev/pci");
		close(fd);
		return;
	}
	if (cnt != st.st_size)
	{
		warnx("/dev/pci: Short read");
		close(fd);
		return;
	}
	
	for (i = 0; i < dev_count; i++)
	{
		if (devs[i].pci_bus < 0)
			continue;
		
		for (n = 0; n < pci_dev_cnt; n++)
		{
			if (devs[i].pci_bus != pci_devs[n].bus)
				continue;
			if (devs[i].pci_dev != pci_devs[n].dev)
				continue;
			if (devs[i].pci_func != pci_devs[n].func)
				continue;
			
			pci_dev_active[n] = 1;
			break;
		}
	}
	
	for (i = 0; i < pci_dev_cnt; i++)
	{
		struct pci_driver *drv;
		
		pdv = &pci_devs[i];
		
		if (!is_pci_free(pdv->bus, pdv->dev, pdv->func))
			continue;
		
		for (drv = pci_drivers; drv; drv = drv->next)
		{
			if (drv->product > 0 && drv->product != pdv->dev_id)
				continue;
			if (drv->vendor > 0 && drv->vendor != pdv->dev_vend)
				continue;
			
			if ((pdv->dev_class & drv->class_mask) != drv->class)
				continue;
			
			break;
		}
		
		if (drv)
		{
			struct device dev;
			int n;
			
			memset(&dev, 0, sizeof dev);
			strcpy(dev.driver, drv->pathname);
			strcpy(dev.desc, drv->desc);
			strcpy(dev.type, "machine");
			
			n = 0;
			do
				sprintf(dev.name, "%s%i", drv->name, n++);
			while (!is_name_free(dev.name));
			
			dev.pci_bus  = pdv->bus;
			dev.pci_dev  = pdv->dev;
			dev.pci_func = pdv->func;
			
			for (n = 0; n < DEV_DMA_COUNT; n++)
				dev.dma_nr[n] = -1U;
			for (n = 0; n < DEV_IRQ_COUNT; n++)
				dev.irq_nr[n] = -1U;
			
			pci_dev_active[i] = 1;
			add_dev(&dev);
		}
	}
	
	close(fd);
}

#if defined __MACH_I386_PC__ || defined __MACH_AMD64_PC__

static void find_system(void)
{
	struct device dev;
	int i;
	
	if (is_drv_listed(SYSINST_MACHINE_DRV))
		return;
	
	memset(&dev, 0, sizeof dev);
	strcpy(dev.driver, SYSINST_MACHINE_DRV);
	strcpy(dev.desc,   SYSINST_MACHINE_DESC);
	strcpy(dev.name,   UTSNAME);
	strcpy(dev.type,   "machine");
	
	dev.io_base[0]	= 0x20;
	dev.io_size[0]	= 0x02;
	dev.io_base[1]	= 0xa0;
	dev.io_size[1]	= 0x02;
	
	dev.io_base[2]	= 0x00;
	dev.io_size[2]	= 0x10;
	dev.io_base[3]	= 0xc0;
	dev.io_size[3]	= 0x20;
	
	dev.pci_bus = dev.pci_dev = dev.pci_func = -1;
	
	for (i = 0; i < DEV_DMA_COUNT; i++)
		dev.dma_nr[i] = -1U;
	for (i = 0; i < DEV_IRQ_COUNT; i++)
		dev.irq_nr[i] = -1U;
	
	add_dev(&dev);
}

static void find_cmos(void)
{
	struct device dev;
	int i;
	
	if (!is_port_free(0x70, 2))
		return;
	
	memset(&dev, 0, sizeof dev);
	strcpy(dev.driver, "/lib/drv/cmos.drv");
	strcpy(dev.desc,   "PC CMOS RAM");
	strcpy(dev.name,   "cmos");
	strcpy(dev.type,   "nvram");
	
	dev.io_base[0]	= 0x70;
	dev.io_size[0]	= 0x02;
	
	dev.pci_bus = dev.pci_dev = dev.pci_func = -1;
	
	for (i = 0; i < DEV_DMA_COUNT; i++)
		dev.dma_nr[i] = -1U;
	for (i = 0; i < DEV_IRQ_COUNT; i++)
		dev.irq_nr[i] = -1U;
	
	add_dev(&dev);
}

static void find_pci(void)
{
	struct device dev;
	int i;
	
	if (!is_port_free(0xcf8, 8))
		return;
	
	if (!is_name_free("pci"))
		return;
	
	memset(&dev, 0, sizeof dev);
	
	strcpy(dev.driver, "/lib/drv/pci.drv");
	strcpy(dev.desc,   "PCI");
	strcpy(dev.name,   "pci");
	strcpy(dev.type,   "bus");
	
	dev.pci_bus = dev.pci_dev = dev.pci_func = -1;
	
	for (i = 0; i < DEV_DMA_COUNT; i++)
		dev.dma_nr[i] = -1U;
	for (i = 0; i < DEV_IRQ_COUNT; i++)
		dev.irq_nr[i] = -1U;
	
	dev.io_base[0] = 0xcf8;
	dev.io_size[0] = 8;
	
	add_dev(&dev);
}

static void find_vga(void)
{
	struct device dev;
	int i;
	
	if (!is_name_free("display"))
		return;
	
#ifdef __MACH_AMD64_PC__
	memset(&dev, 0, sizeof dev);
	
	strcpy(dev.desktop_name, "desktop0");
	strcpy(dev.driver, "/lib/drv/bootfb.drv");
	strcpy(dev.desc, "BOOTFB");
	strcpy(dev.name, "display");
	strcpy(dev.type, "display");
	
	dev.pci_bus = dev.pci_dev = dev.pci_func = -1;
	
	for (i = 0; i < DEV_DMA_COUNT; i++)
		dev.dma_nr[i] = -1U;
	for (i = 0; i < DEV_IRQ_COUNT; i++)
		dev.irq_nr[i] = -1U;
	
	if (!add_dev(&dev))
		return;
#endif
	
	if (!is_port_free(0x3c0, 0x20))
		return;
	
	if (!is_mem_free(0xa0000, 0x20000))
		return;
	
	memset(&dev, 0, sizeof dev);
	
	strcpy(dev.desktop_name, "desktop0");
	strcpy(dev.driver, "/lib/drv/vbe.drv");
	strcpy(dev.desc, "VBE 2+");
	strcpy(dev.name, "display");
	strcpy(dev.type, "display");
	
	dev.mem_base[0] = 0xa0000;
	dev.mem_size[0] = 0x20000;
	dev.io_base[0] = 0x3c0;
	dev.io_size[0] = 0x20;
	
	dev.pci_bus = dev.pci_dev = dev.pci_func = -1;
	
	for (i = 0; i < DEV_DMA_COUNT; i++)
		dev.dma_nr[i] = -1U;
	for (i = 0; i < DEV_IRQ_COUNT; i++)
		dev.irq_nr[i] = -1U;
	
	if (!add_dev(&dev))
		return;
	
	strcpy(dev.driver, "/lib/drv/vga.drv");
	strcpy(dev.desc, "VGA Compatible");
	add_dev(&dev);
}

static void find_pckbd(void)
{
	struct device dev;
	int i;

	if (!is_port_free(0x60, 0x10))
		return;
	if (!is_irq_free(1))
		return;
	if (!is_irq_free(12))
		return;

	memset(&dev, 0, sizeof dev);
	
	dev.pci_bus = dev.pci_dev = dev.pci_func = -1;
	
	for (i = 0; i < DEV_DMA_COUNT; i++)
		dev.dma_nr[i] = -1U;
	for (i = 0; i < DEV_IRQ_COUNT; i++)
		dev.irq_nr[i] = -1U;

	strcpy(dev.desktop_name, "desktop0");
	strcpy(dev.driver,	 "/lib/drv/pckbd.drv");
	strcpy(dev.desc,	 "PC Keyboard");
	strcpy(dev.name,	 "keyboard");
	strcpy(dev.type,	 "keyboard");

	dev.io_base[0] = 0x60;
	dev.io_size[0] = 0x10;
	dev.irq_nr[0]  = 1;
	dev.irq_nr[1]  = 12;

	add_dev(&dev);
}

static void find_pcspk(void)
{
	struct device dev;
	int i;
	
	if (is_drv_listed("/lib/drv/pcspk.drv"))
		return;
	
	memset(&dev, 0, sizeof dev);
	
	dev.pci_bus = dev.pci_dev = dev.pci_func = -1;
	
	for (i = 0; i < DEV_DMA_COUNT; i++)
		dev.dma_nr[i] = -1U;
	for (i = 0; i < DEV_IRQ_COUNT; i++)
		dev.irq_nr[i] = -1U;
	
	strcpy(dev.driver,	"/lib/drv/pcspk.drv");
	strcpy(dev.desc,	"PC Speaker");
	strcpy(dev.name,	"pcspk");
	strcpy(dev.type,	"audio");
	
	dev.io_base[0] = 0x42;
	dev.io_size[0] = 2;
	dev.io_base[1] = 0x61;
	dev.io_size[1] = 1;
	
	add_dev(&dev);
}

static void try_mouse(struct device *dev)
{
	unsigned saved_div;
	unsigned saved_ier;
	unsigned saved_lcr;
	unsigned saved_mcr;
	time_t t;
	
	saved_lcr = inb(dev->io_base[0] + 3);
	saved_mcr = inb(dev->io_base[0] + 4);
	outb(dev->io_base[0] + 3, saved_lcr & 0x7f);
	saved_ier = inb(dev->io_base[0] + 1);
	outb(dev->io_base[0] + 3, saved_lcr | 0x80);
	saved_div = inb(dev->io_base[0]);
	saved_div += inb(dev->io_base[0] + 1) << 8;
	
	outb(dev->io_base[0] + 3, 0x82);
	outb(dev->io_base[0]	, 0x60);
	outb(dev->io_base[0] + 1, 0x00);
	outb(dev->io_base[0] + 3, 0x02);
	outb(dev->io_base[0] + 1, 0x00);
	outb(dev->io_base[0] + 2, 0x00);
	outb(dev->io_base[0] + 4, 0x08);
	usleep(100000);
	inb(dev->io_base[0]);
	
	time(&t);
	outb(dev->io_base[0] + 4, 0x0b);
	while (time(NULL) <= t + 1)
	{
		if (!(inb(dev->io_base[0] + 5) & 1))
			continue;
		
		if (inb(dev->io_base[0]) != 'M')
			continue;
		
		printf("find_devs: found serial mouse\n");
		
		strcpy(dev->desktop_name, "desktop0");
		strcpy(dev->driver,	  "/lib/drv/mouse.drv");
		strcpy(dev->desc,	  "Serial Mouse");
		strcpy(dev->name,	  "mouse");
		strcpy(dev->type,	  "mouse");
	}
	
	outb(dev->io_base[0] + 3, saved_lcr | 0x80);
	outb(dev->io_base[0]	, saved_div);
	outb(dev->io_base[0] + 1, saved_div >> 8);
	outb(dev->io_base[0] + 3, saved_lcr | 0x7f);
	outb(dev->io_base[0] + 1, saved_ier);
	outb(dev->io_base[0] + 3, saved_lcr);
	outb(dev->io_base[0] + 4, saved_mcr);
}

static void try_serial(char *name, unsigned iobase, unsigned irq)
{
	struct device dev;
	int i;
	
	if (!is_port_free(iobase, 8))
		return;
	if (!is_irq_free(irq))
		return;
	
	outb(iobase + 3, 0x80);
	outb(iobase + 0, 0xac);
	outb(iobase + 1, 0xe0);
	if (inb(iobase + 0) != 0xac)
		return;
	if (inb(iobase + 1) != 0xe0)
		return;
	
	memset(&dev, 0, sizeof dev);
	
	dev.pci_bus = dev.pci_dev = dev.pci_func = -1;
	
	for (i = 0; i < DEV_DMA_COUNT; i++)
		dev.dma_nr[i] = -1U;
	for (i = 0; i < DEV_IRQ_COUNT; i++)
		dev.irq_nr[i] = -1U;
	
	strcpy(dev.driver,	"/lib/drv/serial.drv");
	strcpy(dev.desc,	"Serial Port");
	strcpy(dev.name,	name);
	strcpy(dev.type,	"serial");
	dev.io_base[0] = iobase;
	dev.io_size[0] = 8;
	dev.irq_nr[0]  = irq;
	
	try_mouse(&dev);
	add_dev(&dev);
}

static void find_serial(void)
{
	try_serial("com0", 0x3f8, 4);
	try_serial("com1", 0x2f8, 3);
	try_serial("com2", 0x3e8, 4);
	try_serial("com3", 0x2e8, 3);
}

static int try_floppy(unsigned port, unsigned irq, unsigned dma)
{
	int i;
	
	if (!is_port_free(port, 6) || !is_port_free(port + 7, 1))
		return 0;
	if (!is_irq_free(irq))
		return 0;
	if (!is_dma_free(dma))
		return 0;
	
	outb(port + 2, 0x00);
	usleep(100000);
	outb(port + 2, 0x04);
	
	for (i = 0; i < 3; i++)
	{
		if ((inb(port + 4) & 0xc0) == 0x80)
			return 1;
		usleep(100000);
	}
	return 0;
}

static int try_ide(unsigned port0, unsigned port1, unsigned irq)
{
	char *p;
	
	if (!is_port_free(port0, 8) || !is_port_free(port1, 1))
		return 0;
	if (!is_irq_free(irq))
		return 0;
	
	for (p = "HDCPOKE"; *p; p++)
	{
		outb(port0 + 2, *p);
		if (inb(port0 + 2) != *p)
			return 0;
	}
	return 1;
}

static void find_disk(void)
{
	struct device d;
	int i;
	
	memset(&d, 0, sizeof d);
	
	d.pci_bus = d.pci_dev = d.pci_func = -1;
	
	for (i = 0; i < DEV_IRQ_COUNT; i++)
		d.irq_nr[i] = -1U;
	for (i = 0; i < DEV_DMA_COUNT; i++)
		d.dma_nr[i] = -1U;
	strcpy(d.type, "disk");
	
	if (try_ide(0x1f0, 0x3f6, 14))
	{
		d.io_base[0] = 0x1f0;
		d.io_size[0] = 8;
		d.io_base[1] = 0x3f6;
		d.io_size[1] = 1;
		d.irq_nr[0]  = 14;
		strcpy(d.driver, "/lib/drv/hd.drv");
		strcpy(d.desc,   "Primary IDE Channel");
		strcpy(d.name,	 "ide0");
		add_dev(&d);
	}
	
	if (try_ide(0x170, 0x376, 15))
	{
		d.io_base[0] = 0x170;
		d.io_size[0] = 8;
		d.io_base[1] = 0x376;
		d.io_size[1] = 1;
		d.irq_nr[0]  = 15;
		strcpy(d.driver, "/lib/drv/hd.drv");
		strcpy(d.desc,	 "Secondary IDE Channel");
		strcpy(d.name,	 "ide1");
		add_dev(&d);
	}
	
	if (try_floppy(0x3f0, 6, 2))
	{
		d.io_base[0] = 0x3f0;
		d.io_size[0] = 6;
		d.io_base[1] = 0x3f7;
		d.io_size[1] = 1;
		d.irq_nr[0]  = 6;
		d.dma_nr[0]  = 2;
		strcpy(d.driver, "/lib/drv/fd.drv");
		strcpy(d.desc,	 "PC Floppy");
		strcpy(d.name,	 "fd0");
		add_dev(&d);
	}
}

static void find_devs(void)
{
	find_system();
	find_cmos();
	find_pci();
	find_pci_devs();
	find_vga();
	find_pckbd();
	find_disk();
	find_serial();
	find_pcspk();
}

#else
#error unsupported machine type
#endif

int main(int argc, char **argv)
{
	int pci = 0;
	int c;
	
	while (c = getopt(argc, argv, "d:"), c > 0)
		switch (c)
		{
		case 'd':
			desktop = optarg;
			break;
		case 'p':
			pci = 1;
			break;
		default:
			return 1;
		}
	
	argc -= optind;
	argv += optind;
	
	if (argc)
		ddb = argv[0];
	
	if (geteuid())
		errx(255, "you are not root");
	_iopl(3);
	
	if (win_attach())
		err(255, NULL);
	
	load_devs(ddb);
	if (pci)
		find_pci_devs();
	else
		find_devs();
	save_devs(ddb);
	return 0;
}
