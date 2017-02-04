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

#include <sysload/kparam.h>
#include <sysload/flags.h>
#include <kern/start.h>
#include <kern/block.h>
#include <kern/lib.h>
#include <dev/rd.h>
#include <overflow.h>
#include <bioctl.h>
#include <errno.h>

#define RD_MAX	8

static struct bdev rd_bdevs[RD_MAX];
static char rd_names[RD_MAX][4];

static struct rd_unit
{
	char *	image;
	blk_t	size;
} rd_units[RD_MAX];

static int rd_open(int unit);
static int rd_close(int unit);
static int rd_read(int unit, blk_t blk, void *buf);
static int rd_write(int unit, blk_t blk, const void *buf);
static int rd_ioctl(int unit, int cmd, void *buf);

static struct bdev rd_bdev =
{
	refcnt:		0,
	name:		"rdX",
	unit:		0,
	open:		rd_open,
	close:		rd_close,
	ioctl:		rd_ioctl,
	read:		rd_read,
	write:		rd_write,
};

static int rd_open(int unit)
{
	return 0;
}

static int rd_close(int unit)
{
	return 0;
}

static int rd_ioctl(int unit, int cmd, void *buf)
{
	struct rd_unit *u = &rd_units[unit];
	struct bio_info bi;
	size_t sz;
	blk_t bc;
	char *p;
	int err;
	
	switch (cmd)
	{
	case BIO_INFO:
		memset(&bi, 0, sizeof bi);
		bi.type = BIO_TYPE_SSD;
		bi.os	= BIO_OS_SELF;
		bi.size	= u->size;
		return tucpy(buf, &bi, sizeof bi);
	case RDIOCNEW:
		err = fucpy(&bc, buf, sizeof bc);
		if (err)
			return err;
		
		if (ov_mul_size(sz, bc))
			return EINVAL;
		sz = bc * BLK_SIZE;
		
		free(u->image);
		u->size = 0;
		if (!bc)
			return 0;
		
		err = kmalloc(&p, sz, "rd");
		if (err)
			return err;
		memset(p, 0, sz);
		
		u->image = p;
		u->size  = bc;
		return 0;
	default:
		;
	}
	return ENOTTY;
}

static int rd_read(int unit, blk_t blk, void *buf)
{
	struct rd_unit *u = &rd_units[unit];
	
	if (blk < 0 || blk >= u->size)
		return EINVAL;
	
	memcpy(buf, u->image + BLK_SIZE * blk, BLK_SIZE);
	return 0;
}

static int rd_write(int unit, blk_t blk, const void *buf)
{
	struct rd_unit *u = &rd_units[unit];
	
	if (blk < 0 || blk >= u->size)
		return EINVAL;
	
	memcpy(u->image + BLK_SIZE * blk, buf, BLK_SIZE);
	return 0;
}

int mod_onload(unsigned md, const char *pathname, const void *arg, unsigned arg_size)
{
	int err;
	int i;
	
	if (kparam.boot_flags & BOOT_VERBOSE)
		printk("rd.drv: loadable ramdisk driver\n");
	
	for (i = 0; i < RD_MAX; i++)
	{
		rd_bdevs[i]	    = rd_bdev;
		rd_bdevs[i].name    = rd_names[i];
		rd_bdevs[i].unit    = i;
		
		strcpy(rd_names[i], "rdX");
		rd_names[i][2] = '0' + i;
		
		err = blk_install(&rd_bdevs[i]);
		if (err)
		{
			while (i-- > 0)
				blk_uinstall(&rd_bdevs[i]);
			return err;
		}
	}
	return 0;
}

int mod_onunload(unsigned md)
{
	int i;
	
	for (i = 0; i < RD_MAX; i++)
	{
		blk_uinstall(&rd_bdevs[i]);
		free(rd_units[i].image);
	}
	return 0;
}
