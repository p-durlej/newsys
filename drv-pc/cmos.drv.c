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

#include <kern/errno.h>
#include <kern/cio.h>
#include <kern/lib.h>
#include <dev/cmos.h>
#include <devices.h>

static int cd_open(struct cdev *cd);
static int cd_close(struct cdev *cd);
static int cd_read(struct cio_request *rq);
static int cd_write(struct cio_request *rq);
static int cd_ioctl(struct cdev *cd, int cmd, void *buf);

static uint8_t  cmos_image[128];
static unsigned cmos_iobase;

static struct cdev cmos_cdev =
{
	.name	= "cmos",
	.open	= cd_open,
	.close	= cd_close,
	.read	= cd_read,
	.write	= cd_write,
	.ioctl	= cd_ioctl,
};

static int cdev_installed;

static int cd_open(struct cdev *cd)
{
	return 0;
}

static int cd_close(struct cdev *cd)
{
	return 0;
}

static int cd_read(struct cio_request *rq)
{
	if (rq->offset >= sizeof cmos_image)
	{
		rq->count = 0;
		return 0;
	}
	
	if (rq->offset + rq->count >= sizeof cmos_image)
		rq->count = sizeof cmos_image - rq->offset;
	
	memcpy(rq->buf, cmos_image + rq->offset, rq->count);
	return 0;
}

static int cd_write(struct cio_request *rq)
{
	return EPERM;
}

static int cd_ioctl(struct cdev *cd, int cmd, void *buf)
{
	return ENOTTY;
}

int cmos_read_uncached(unsigned addr, uint8_t *valp)
{
	if (addr >= 128)
		return EINVAL;
	
	outb(cmos_iobase, addr);
	*valp = inb(cmos_iobase + 1);
	return 0;
}

int cmos_read(unsigned addr, uint8_t *valp)
{
	if (addr >= 128)
		return EINVAL;
	
	*valp = cmos_image[addr];
	return 0;
}

int cmos_write(unsigned addr, uint8_t val)
{
	if (addr >= 128)
		return EINVAL;
	
	cmos_image[addr] = val;
	
	outb(cmos_iobase,     addr);
	outb(cmos_iobase + 1, val);
}

int mod_onload(unsigned md, char *pathname, struct device *dev, unsigned dev_size)
{
	unsigned i;
	int err;
	
	cmos_iobase = dev->io_base[0];
	
	for (i = 0; i < sizeof cmos_image; i++)
		cmos_read_uncached(i, &cmos_image[i]);
	
	err = cio_install(&cmos_cdev);
	if (err)
		return err;
	
	cdev_installed = 1;
	return 0;
}

int mod_onunload(unsigned md)
{
	if (!cdev_installed)
		return 0;
	
	return cio_uinstall(&cmos_cdev);
}
