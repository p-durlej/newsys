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

#include <kern/module.h>
#include <kern/printk.h>
#include <kern/config.h>
#include <kern/errno.h>
#include <kern/block.h>
#include <kern/hw.h>
#include <bioctl.h>
#include <dev/cmos.h>
#include <dev/fd.h>
#include <devices.h>

#define FD_MAX_RETRIES		3
#define FD_IO_TIMEOUT		(clock_hz() / 2)
#define FD_SEEK_TIMEOUT		(clock_hz() * 3)
#define FD_FORMAT_TIMEOUT	(clock_hz() * 3)
#define ERROR_PRINTK		KVERBOSE

#define FD_SPINUP_DELAY		(1 * clock_hz())
#define FD_MOTOR_TIMEOUT	(3 * clock_hz())

#define PHYS_UNITS		2
#define UNITS			2

#define IRQ			6

#define FDC_DOR		0x03f2
#define FDC_MSR		0x03f4
#define FDC_DSR		0x03f4
#define FDC_DATA	0x03f5
#define FDC_DIR		0x03f7
#define FDC_CONFIG	0x03f7

#define DATA_RATE_250	2
#define DATA_RATE_300	1
#define DATA_RATE_500	0
#define DATA_RATE_1000	3

struct floppy
{
	unsigned unit;
	unsigned c, h, s;
};

static void fd_irqv();

static int fd_reset(void);
static int fd_wait(unsigned timeout);
static int fd_out(unsigned byte);
static int fd_in(unsigned *byte);
static int fd_open(int unit);
static int fd_close(int unit);
static int fd_ioctl(int unit, int cmd, void *buf);
static int fd_read(int unit, blk_t blk, void *buf);
static int fd_write(int unit, blk_t blk, const void *buf);

static volatile int fd_motor_timeout;
static volatile int fd_irq;

static unsigned fdc_dor;
static int need_reset;

static int curr_cyl[PHYS_UNITS];

static void *fd_buf;

static struct bdev bdev[UNITS] =
{
	{ .name = "fd0", .unit = 0, .open = fd_open, .close = fd_close, .ioctl = fd_ioctl, .read = fd_read, .write = fd_write },
	{ .name = "fd1", .unit = 1, .open = fd_open, .close = fd_close, .ioctl = fd_ioctl, .read = fd_read, .write = fd_write },
};

static struct floppy floppy[UNITS] =
{
	{ 0, 80, 2, 18 },
	{ 1, 80, 2, 18 },
};

static int fd_instunit(int u)
{
	int err;
	
	err = blk_install(&bdev[u]);
	if (err)
		perror("fd.c: fd_init: blk_install", err);
	
	return err;
}

static int fd_init(void)
{
	uint8_t conf;
	int err;
	int i;
	
	err = cmos_read(0x10, &conf);
	if (err)
		return err;
	
#if KVERBOSE
	printk("fd_init: conf = %x\n", conf);
#endif
	
	if (!conf)
		return ENODEV;
	
	irq_set(IRQ, fd_irqv);
	irq_ena(IRQ);
	
	fd_reset();
	
	if ((conf & 0xf0) == 0x40)
		fd_instunit(0);
	
	if ((conf & 0x0f) == 0x04)
		fd_instunit(1);
	
	for (i = 0; i < PHYS_UNITS; i++)
		curr_cyl[i] = -1;
	
#if KVERBOSE
	printk("fd.c: fd_init: ok\n");
#endif
	return 0;
}

static void fd_clock(void)
{
	int s;
	
	if (fd_motor_timeout && !--fd_motor_timeout)
	{
		fdc_dor = 0x0c;
		outb(FDC_DOR, fdc_dor);
	}
}

static void fd_irqv()
{
	int s;
	
	fd_irq = 1;
}

static int fd_reset(void)
{
	unsigned st0, st1;
	int err;
	int i;
	
	for (i = 0; i < PHYS_UNITS; i++)
		curr_cyl[i] = -1;
	
	outb(FDC_DOR, 0);
	clock_delay(1);
	outb(FDC_DOR, fdc_dor | 0x0c);
	
	err = fd_wait(FD_IO_TIMEOUT);
	if (err)
		return err;
	
	for (i = 0; i < 4; i++)
	{
		if (fd_out(0x08))
			continue;
		if (fd_in(&st0))
			continue;
		if ((st0 & 0xc0) != 0x80)
			fd_in(&st1);
	}
	
	outb(FDC_CONFIG, DATA_RATE_500);
	
	fd_out(0x03); /* fix drive data command */
	fd_out(0x0f);
	fd_out(0xfe);
	
	need_reset = 0;
	return 0;
}

static unsigned fd_ticks(void)
{
	return clock_time() * clock_hz() + clock_ticks();
}

static int fd_wait(unsigned timeout)
{
	time_t tmout = fd_ticks() + timeout;
	int s;
	
	s = intr_dis();
	while (!fd_irq && fd_ticks() < tmout)
		asm volatile("sti; hlt; cli;");
	intr_res(s);
	
	if (!fd_irq)
	{
#if ERROR_PRINTK
		printk("fd_wait: timeout\n");
#endif
		need_reset = 1;
		return EIO;
	}
	return 0;
}

static int fd_out(unsigned byte)
{
	time_t tmout = fd_ticks() + FD_IO_TIMEOUT;
	
	while ((inb(FDC_MSR) & 0xc0) != 0x80 && fd_ticks() < tmout);
	if ((inb(FDC_MSR) & 0xc0) != 0x80)
	{
#if ERROR_PRINTK
		printk("fd_out: timeout\n");
#endif
		fd_motor_timeout = FD_MOTOR_TIMEOUT;
		need_reset = 1;
		return EIO;
	}
	
	outb(FDC_DATA, byte);
	return 0;
}

static int fd_in(unsigned *byte)
{
	time_t tmout = fd_ticks() + FD_IO_TIMEOUT;
	
	while ((inb(FDC_MSR) & 0xc0) != 0xc0 && fd_ticks() < tmout);
	if ((inb(FDC_MSR) & 0xc0) != 0xc0)
	{
#if ERROR_PRINTK
		printk("fd_in: timeout\n");
#endif
		need_reset = 1;
		return EIO;
	}
	
	*byte = inb(FDC_DATA);
	return 0;
}

static void fd_select(int u)
{
	unsigned motor = 0x10 << floppy[u].unit;
	
	fd_motor_timeout = 0;
	
	if (need_reset)
		fd_reset();
	
	if (fdc_dor & motor)
	{
		fdc_dor = motor | floppy[u].unit | 0x0c;
		outb(FDC_DOR, fdc_dor);
	}
	else
	{
		fdc_dor = motor | floppy[u].unit | 0x0c;
		outb(FDC_DOR, fdc_dor);
		clock_delay(FD_SPINUP_DELAY);
	}
}

static int fd_seek0(int u)
{
	unsigned st0;
	unsigned cyl;
	int err;
	
	fd_irq = 0;
	if ((err = fd_out(0x07)))		return err;
	if ((err = fd_out(floppy[u].unit)))	return err;
	if ((err = fd_wait(FD_SEEK_TIMEOUT)))
	{
#if ERROR_PRINTK
		printk("%s: fd_wait: error %i\n", __func__, err);
#endif
		return err;
	}
	
	fd_out(0x08);
	if ((err = fd_in(&st0)))		return err;
	if ((err = fd_in(&cyl)))		return err;
	
	if (st0 & 0xc0)
	{
#if ERROR_PRINTK
		printk("fd.c: fd_seek0: error, u = %i, st0 = 0x%x, cyl = %i\n", u, st0, cyl);
#endif
		curr_cyl[floppy[u].unit] = -1;
		return EIO;
	}
	curr_cyl[floppy[u].unit] = 0;
	return 0;
}

static int fd_seek(unsigned u, unsigned c, unsigned h)
{
	unsigned st0;
	unsigned cyl;
	int err;
	
	fd_irq = 0;
	if ((err = fd_out(0x0f)))		return err;
	if ((err = fd_out(floppy[u].unit)))	return err;
	if ((err = fd_out(c)))			return err;
	if ((err = fd_wait(FD_SEEK_TIMEOUT)))	return err;
	
	fd_out(0x08);
	if ((err = fd_in(&st0)))		return err;
	if ((err = fd_in(&cyl)))		return err;
	if (st0 & 0xc0)
	{
#if ERROR_PRINTK
		printk("fd.c: fd_seek: error, u = %i, c = %i, h = %i, "
			"st0 = 0x%x, cyl = %i\n", u, c, h, st0, cyl);
#endif
		curr_cyl[floppy[u].unit] = -1;
		return EIO;
	}
	if (cyl != c)
	{
#if ERROR_PRINTK
		printk("fd.c: fd_seek: fd controller failure, cyl = %i, c = %i\n", cyl, c);
#endif
		curr_cyl[floppy[u].unit] = -1;
		return EIO;
	}
	curr_cyl[floppy[u].unit] = c;
	return 0;
}

static void fd_setup_dma(void *buf, int wr)
{
	struct dma_setup ds;
	
	ds.addr	 = pg_getphys(buf);
	ds.count = 512;
	ds.mode	 = DMA_SINGLE;
	ds.dir	 = wr ? DMA_MEM_TO_IO : DMA_IO_TO_MEM;
	
	dma_setup(2, &ds);
	dma_ena(2);
}

static int fd_open(int u)
{
	return 0;
}

static int fd_close(int u)
{
	curr_cyl[floppy[u].unit] = -1;
	return 0;
}

static int fd_read(int u, blk_t blk, void *buf)
{
	int retries = 0;
	unsigned st0;
	unsigned st1;
	unsigned st2;
	unsigned c;
	unsigned h;
	unsigned s;
	unsigned t;
	unsigned junk;
	int err;
	
	s = blk % floppy[u].s;
	t = blk / floppy[u].s;
	h = t   % floppy[u].h;
	c = t   / floppy[u].h;
	
	if (c >= floppy[u].c)
	{
#if ERROR_PRINTK
		printk("fd.c: read: bad cylinder number: %i, blk %i\n", c, blk);
#endif
		return EIO;
	}
	
retry:
	if (++retries > FD_MAX_RETRIES)
		goto unlock;
	fd_select(u);
	
	if (curr_cyl[floppy[u].unit] < 0)
	{
		err = fd_seek0(u);
		if (err)
			goto retry;
	}
	
	if (curr_cyl[floppy[u].unit] != c)
	{
		err = fd_seek(u, c, h);
		if (err)
		{
			curr_cyl[floppy[u].unit] = -1;
			goto retry;
		}
	}
	
	fd_setup_dma(fd_buf, 0);
	
	fd_irq = 0;
	if ((err = fd_out(0xe6)))			goto retry; /* read sector command */
	if ((err = fd_out(floppy[u].unit | (h << 2))))	goto retry;
	if ((err = fd_out(c)))				goto retry;
	if ((err = fd_out(h)))				goto retry;
	if ((err = fd_out(s + 1)))			goto retry;
	if ((err = fd_out(2)))				goto retry;
	if ((err = fd_out(s + 1)))			goto retry;
	if ((err = fd_out(27)))				goto retry;
	if ((err = fd_out(0xff)))			goto retry;
	
	if ((err = fd_wait(FD_IO_TIMEOUT)))		goto retry;
	
	if ((err = fd_in(&st0)))			goto retry;
	if ((err = fd_in(&st1)))			goto retry;
	if ((err = fd_in(&st2)))			goto retry;
	if ((err = fd_in(&junk)))			goto retry;
	if ((err = fd_in(&junk)))			goto retry;
	if ((err = fd_in(&junk)))			goto retry;
	if ((err = fd_in(&junk)))			goto retry;
	
	if (st0 & 0xc0)
	{
#if ERROR_PRINTK
		printk("fd.c: read: i/o error\n");
		printk("            blk = %i\n", blk);
		printk("            c   = %i\n", c);
		printk("            h   = %i\n", h);
		printk("            s   = %i\n", s);
		printk("            st0 = 0x%x\n", st0);
		printk("            st1 = 0x%x\n", st1);
		printk("            st2 = 0x%x\n", st2);
#endif
		curr_cyl[floppy[u].unit] = -1;
		err = EIO;
		goto retry;
	}
	
	memcpy(buf, fd_buf, 512);
unlock:
	fd_motor_timeout = FD_MOTOR_TIMEOUT;
	return err;
}

static int fd_write(int u, blk_t blk, const void *buf)
{
	int retries = 0;
	unsigned st0;
	unsigned st1;
	unsigned st2;
	unsigned c;
	unsigned h;
	unsigned s;
	unsigned t;
	unsigned junk;
	int err;
	
	memcpy(fd_buf, buf, 512);
	
	s = blk % floppy[u].s;
	t = blk / floppy[u].s;
	h = t	% floppy[u].h;
	c = t	/ floppy[u].h;
	
	if (c >= floppy[u].c)
	{
#if ERROR_PRINTK
		printk("fd.c: write: bad cylinder number: %i, blk %i\n", c, blk);
#endif
		return EIO;
	}
	
retry:
	if (++retries > FD_MAX_RETRIES)
		goto unlock;
	fd_select(u);
	
	if (curr_cyl[floppy[u].unit] < 0)
	{
		err = fd_seek0(u);
		if (err)
			goto retry;
	}
	
	if (curr_cyl[floppy[u].unit] != c)
	{
		err = fd_seek(u, c, h);
		if (err)
		{
			curr_cyl[floppy[u].unit] = -1;
			goto retry;
		}
	}
	
	fd_setup_dma(fd_buf, 1);
	
	fd_irq = 0;
	if ((err = fd_out(0x45)))			goto retry; /* write sector command */
	if ((err = fd_out(floppy[u].unit | (h << 2))))	goto retry;
	if ((err = fd_out(c)))				goto retry;
	if ((err = fd_out(h)))				goto retry;
	if ((err = fd_out(s + 1)))			goto retry;
	if ((err = fd_out(2)))				goto retry;
	if ((err = fd_out(s + 1)))			goto retry;
	if ((err = fd_out(27)))				goto retry;
	if ((err = fd_out(0xff)))			goto retry;
	
	if ((err = fd_wait(FD_IO_TIMEOUT)))		goto retry;
	
	if ((err = fd_in(&st0)))			goto retry;
	if ((err = fd_in(&st1)))			goto retry;
	if ((err = fd_in(&st2)))			goto retry;
	if ((err = fd_in(&junk)))			goto retry;
	if ((err = fd_in(&junk)))			goto retry;
	if ((err = fd_in(&junk)))			goto retry;
	if ((err = fd_in(&junk)))			goto retry;
	
	if (st0 & 0xc0)
	{
#if ERROR_PRINTK
		printk("fd.c: write: i/o error\n");
		printk("             blk = %i\n", blk);
		printk("             c   = %i\n", c);
		printk("             h   = %i\n", h);
		printk("             s   = %i\n", s);
		printk("             st0 = 0x%x\n", st0);
		printk("             st1 = 0x%x\n", st1);
		printk("             st2 = 0x%x\n", st2);
#endif
		curr_cyl[floppy[u].unit] = -1;
		err = EIO;
		goto retry;
	}
	
	fd_motor_timeout = FD_MOTOR_TIMEOUT;
unlock:
	return err;
}

static int fd_fmttrk(int u, struct fmttrack *fmt)
{
	int st0, st1, st2;
	int junk;
	int err;
	char *p;
	int i;
	
	if (fmt->cyl  < 0 || fmt->cyl  >= floppy[u].c)
		return EINVAL;
	if (fmt->head < 0 || fmt->head >= floppy[u].h)
		return EINVAL;
	
	fd_select(u);
	
	if (curr_cyl[floppy[u].unit] < 0)
	{
		err = fd_seek0(u);
		if (err)
			goto unlock;
	}
	
	if (curr_cyl[floppy[u].unit] != fmt->cyl)
	{
		err = fd_seek(u, fmt->cyl, fmt->head);
		if (err)
			goto unlock;
	}
	
	for (p = fd_buf, i = 1; i <= 18; i++)
	{
		*p++ = fmt->cyl;
		*p++ = fmt->head;
		*p++ = i;
		*p++ = 2; /* sector size: 512 bytes */
	}
	
	fd_setup_dma(fd_buf, 1);
	
	fd_irq = 0;
	if ((err = fd_out(0x4d)))				goto unlock; /* format sector command */
	if ((err = fd_out(floppy[u].unit | (fmt->head << 2))))	goto unlock;
	if ((err = fd_out(2)))					goto unlock; /* sector size: 512 bytes */
	if ((err = fd_out(18)))					goto unlock; /* number of sectors */
	if ((err = fd_out(0x54)))				goto unlock; /* GAP3 */
	if ((err = fd_out(0xcc)))				goto unlock; /* fill byte */
	
	if ((err = fd_wait(FD_FORMAT_TIMEOUT)))			goto unlock;
	
	if ((err = fd_in(&st0)))				goto unlock;
	if ((err = fd_in(&st1)))				goto unlock;
	if ((err = fd_in(&st2)))				goto unlock;
	if ((err = fd_in(&junk)))				goto unlock;
	if ((err = fd_in(&junk)))				goto unlock;
	if ((err = fd_in(&junk)))				goto unlock;
	if ((err = fd_in(&junk)))				goto unlock;
	
	if (st0 & 0xc0)
	{
		curr_cyl[floppy[u].unit] = -1;
#if ERROR_PRINTK
		printk("fd.drv: fd_fmttrk: error\n");
		printk("                   c   = %i\n", fmt->cyl);
		printk("                   h   = %i\n", fmt->head);
		printk("                   st0 = 0x%x\n", st0);
		printk("                   st1 = 0x%x\n", st1);
		printk("                   st2 = 0x%x\n", st2);
#endif
		err = EIO;
		goto unlock;
	}

unlock:
	fd_motor_timeout = FD_MOTOR_TIMEOUT;
	return err;
}

static int fd_ioctl(int unit, int cmd, void *buf)
{
	struct fmttrack fmt;
	struct bio_info bi;
	int parm;
	int err;
	
	switch (cmd)
	{
	case BIO_INFO:
		memset(&bi, 0, sizeof bi);
		
		bi.type	  = BIO_TYPE_FD;
		bi.os	  = 0;
		bi.offset = 0;
		bi.ncyl	  = floppy[unit].c;
		bi.nhead  = floppy[unit].h;
		bi.nsect  = floppy[unit].s;
		bi.size	  = bi.ncyl * bi.nhead * bi.nsect;
		return tucpy(buf, &bi, sizeof bi);
	case FDIOCFMTTRK:
		err = fucpy(&fmt, buf, sizeof fmt);
		if (err)
			return err;
		
		return fd_fmttrk(unit, &fmt);
	case FDIOCSEEK:
		err = fucpy(&parm, buf, sizeof parm);
		if (err)
			return err;
		
		fd_select(unit);
		
		if (curr_cyl[floppy[unit].unit] < 0)
		{
			err = fd_seek0(unit);
			if (err)
			{
				fd_motor_timeout = FD_MOTOR_TIMEOUT;
				return err;
			}
		}
		
		err = fd_seek(unit, parm, 0);
		fd_motor_timeout = FD_MOTOR_TIMEOUT;
		return err;
	case FDIOCSEEK0:
		fd_select(unit);
		
		err = fd_seek0(unit);
		fd_motor_timeout = FD_MOTOR_TIMEOUT;
		return err;
	case FDIOCRESET:
		return fd_reset();
	case FDIOCGCCYL:
		return tucpy(buf, &curr_cyl[floppy[unit].unit], sizeof curr_cyl[floppy[unit].unit]);
	default:
		return ENOTTY;
	}
}

int mod_onload(unsigned md, const char *pathname, const void *data, unsigned data_size)
{
	const struct device *dev = data;
	char *n1, *n2;
	int len;
	int err;
	
	len = strlen(dev->name);
	n1  = malloc(len + 3);
	n2  = malloc(len + 3);
	if (n1 == NULL || n2 == NULL)
	{
		free(n1);
		free(n2);
		return ENOMEM;
	}
	
	strcpy(n1, dev->name);
	strcat(n1, ",0");
	
	strcpy(n2, dev->name);
	strcat(n2, ",1");
	
	bdev[0].name = n1;
	bdev[1].name = n2;
	
	fd_buf = dma_malloc(512); /* XXX */
	fd_init(); /* XXX */
	clock_ihand(fd_clock); /* XXX */
	return 0;
}

int mod_onunload(unsigned md)
{
	return EBUSY;
}
