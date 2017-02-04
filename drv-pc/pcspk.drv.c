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
#include <kern/lib.h>
#include <kern/cio.h>
#include <kern/hw.h>
#include <devices.h>

static int spk_open(struct cdev *cd);
static int spk_close(struct cdev *cd);
static int spk_read(struct cio_request *rq);
static int spk_write(struct cio_request *rq);
static int spk_ioctl(struct cdev *cd, int cmd, void *buf);

static int spk_iob0;
static int spk_iob1;

static struct cdev cdev =
{
	.name	 = "pcspk",
	.unit	 = 0,
	.no_seek = 0,
	.open	 = spk_open,
	.close	 = spk_close,
	.read	 = spk_read,
	.write	 = spk_write,
	.ioctl	 = spk_ioctl
};

static void spk_init(void)
{
	outb(spk_iob1, inb(spk_iob1) & 0xfc);
	outb(spk_iob0 + 1, 0xb6);
	outb(spk_iob0, 0);
	outb(spk_iob0, 0);
}

static int spkctl(int freq)
{
	int s;
	int t;
	
	if (freq <= 0)
		goto disable;
	
	t = 1193180 / freq;
	if (t > 0xffff)
		goto disable;
	
	s = intr_dis();
	outb(spk_iob0 + 1, 0xb6);
	outb(spk_iob0, t);
	outb(spk_iob0, t >> 8);
	outb(spk_iob1, inb(spk_iob1) | 3);
	intr_res(s);
	return 0;
disable:
	s = intr_dis();
	outb(spk_iob1, inb(spk_iob1) & 0xfc);
	intr_res(s);
	return 0;
}

static int spk_open(struct cdev *cd)
{
	return 0;
}

static int spk_close(struct cdev *cd)
{
	if (cd->refcnt <= 1)
		spkctl(0);
	return 0;
}

static int spk_read(struct cio_request *rq)
{
	return EIO;
}

static int spk_write(struct cio_request *rq)
{
	uint16_t freq;
	int err;
	
	if (rq->count != 2)
		return EIO;
	err = fucpy(&freq, rq->buf, sizeof freq);
	if (err)
		return err;
	return spkctl(freq);
}

static int spk_ioctl(struct cdev *cd, int cmd, void *buf)
{
	return ENOTTY;
}

int mod_onload(unsigned md, const char *pathname, const void *data, unsigned sz)
{
	const struct device *dev = data;
	
#if KVERBOSE
	printk("pcspk.drv: loadable speaker driver\n");
#endif
	spk_iob0 = dev->io_base[0];
	spk_iob1 = dev->io_base[1];
	spk_init();
	
	return cio_install(&cdev);
}

int mod_onunload(unsigned md)
{
	return EBUSY;
}
