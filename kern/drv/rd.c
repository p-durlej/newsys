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

#include <kern/printk.h>
#include <kern/start.h>
#include <kern/block.h>
#include <kern/lib.h>
#include <errno.h>

static int rd_open(int unit);
static int rd_close(int unit);
static int rd_ioctl(int unit, int cmd, void *buf);
static int rd_read(int unit, blk_t blk, void *buf);
static int rd_write(int unit, blk_t blk, const void *buf);

static struct bdev rd_bdev =
{
	refcnt:		0,
	name:		"ramdisk",
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
	return ENOTTY;
}

static int rd_read(int unit, blk_t blk, void *buf)
{
	if (blk < 0 || blk >= kparam.rd_blocks)
		return EINVAL;
	
	memcpy(buf, (char *)(intptr_t)kparam.rd_base + BLK_SIZE * blk, BLK_SIZE);
	return 0;
}

static int rd_write(int unit, blk_t blk, const void *buf)
{
	if (blk < 0 || blk >= kparam.rd_blocks)
		return EINVAL;
	
	memcpy((char *)(intptr_t)kparam.rd_base + BLK_SIZE * blk, buf, BLK_SIZE);
	return 0;
}

void rd_boot(void)
{
	int err;
	
	if (kparam.rd_blocks)
	{
		err = blk_install(&rd_bdev);
		if (err)
		{
			perror("rd_boot: blk_install", err);
			panic("rd_boot: blk_install failed");
		}
	}
}
