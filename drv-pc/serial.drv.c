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

#include <kern/task_queue.h>
#include <kern/task.h>
#include <kern/printk.h>
#include <kern/event.h>
#include <kern/errno.h>
#include <kern/sched.h>
#include <kern/umem.h>
#include <kern/intr.h>
#include <kern/lib.h>
#include <kern/cio.h>
#include <kern/hw.h>
#include <devices.h>
#include <comm.h>

#define BUF_SIZE	16384

static void irq(int nr, int count);
static void set_speed(unsigned bps);
static int get_speed();

static int sio_open(struct cdev *cd);
static int sio_close(struct cdev *cd);
static int sio_read(struct cio_request *rq);
static int sio_write(struct cio_request *rq);
static int sio_ioctl(struct cdev *cd, int cmd, void *buf);

static char name[16] = "comm";
static struct cdev cdev =
{
	.name	 = name,
	.unit	 = 0,
	.no_seek = 1,
	.open	 = sio_open,
	.close	 = sio_close,
	.read	 = sio_read,
	.write	 = sio_write,
	.ioctl	 = sio_ioctl
};

static struct task_queue rx_queue;
static struct task_queue tx_queue;

static char *		out_buf;
static volatile char *	in_buf;
static volatile int	out_count;
static int		out_p0;
static volatile int	out_p1;
static volatile int	in_count;
static int		in_p0;
static volatile int	in_p1;
static int		port_open;

static unsigned io_base;
static unsigned irq_nr;

static void resume_tx(void)
{
	struct task *t;
	
	while (t = task_dequeue(&tx_queue), t)
		task_resume(t);
}

static void resume_rx(void)
{
	struct task *t;
	
	while (t = task_dequeue(&rx_queue), t)
		task_resume(t);
}

static int sio_open(struct cdev *cd)
{
	if (!port_open)
		cio_state(cd, W_OK);
	port_open++;
	return 0;
}

static int sio_close(struct cdev *cd)
{
	while (out_count) /* XXX */
		sched();
	port_open--;
	if (!port_open)
	{
		cio_state(cd, 0);
		in_count = 0;
		in_p0 = 0;
		in_p1 = 0;
	}
	return 0;
}

static int sio_read(struct cio_request *rq)
{
	char *buf;
	int left;
	int err;
	int ic;
	int s;
	
	s = intr_dis();
	while (!in_count && !rq->no_delay)
	{
		err = task_suspend(&rx_queue, WAIT_INTR);
		if (err)
		{
			intr_res(s);
			return err;
		}
		intr_dis(); /* XXX */
	}
	ic = in_count;
	intr_res(s);
	
	if (!ic)
		return EAGAIN;
	
	if (rq->count > ic)
		rq->count = ic;
	left = rq->count;
	buf  = rq->buf;
	while (left)
	{
		err = tucpy(buf, (void *)&in_buf[in_p0], 1);
		if (err)
		{
			if (left != rq->count)
				goto intr;
			goto err;
		}
		buf++;
		in_p0++;
		in_p0 %= BUF_SIZE;
		s = intr_dis();
		in_count--;
		intr_res(s);
		left--;
	}
intr:
	rq->count -= left;
	
	err = 0;
err:
	s = intr_dis();
	ic = in_count;
	intr_res(s);
	if (!ic)
		cio_clrstate(&cdev, R_OK);
	return err;
}

static void sio_startw(void)
{
	int s;
	
	s = intr_dis();
	if (out_count && inb(io_base + 5) & 0x20)
	{
		outb(io_base, out_buf[out_p1]);
		out_p1++;
		out_p1 %= BUF_SIZE;
		out_count--;
	}
	intr_res(s);
}

static int sio_write(struct cio_request *rq)
{
	char *buf;
	int left;
	int err = 0;
	char c;
	int s;
	
	s = intr_dis();
	if (rq->no_delay && rq->count > BUF_SIZE - out_count)
		rq->count = BUF_SIZE - out_count;
	intr_res(s);
	
	if (!rq->count)
		return EAGAIN;
	
	left = rq->count;
	buf  = rq->buf;
	while (left)
	{
		err = fucpy(&c, buf, 1); /* XXX */
		if (err)
			return err;
		
		s = intr_dis();
		while (out_count >= BUF_SIZE)
		{
			sio_startw();
			
			if (err = task_suspend(&tx_queue, WAIT_INTR), err)
			{
				intr_res(s);
				if (left != rq->count)
					err = 0;
				goto intr;
			}
			intr_dis(); /* XXX */
		}
		
		buf++;
		out_buf[out_p0] = c;
		out_p0++;
		out_p0 %= BUF_SIZE;
		out_count++;
		intr_res(s);
		left--;
	}
intr:
	rq->count -= left;
	
	sio_startw();
	
	if (out_count < BUF_SIZE)
		resume_tx();
	else
		cio_clrstate(&cdev, W_OK);
	return 0;
}

static int sio_ioctl(struct cdev *cd, int cmd, void *buf)
{
	struct comm_config conf;
	struct comm_stat st;
	int speed;
	int err;
	
	if (!port_open)
		panic("serial.drv: sio_ioctl: !port_open");
	
	switch (cmd)
	{
	case COMMIO_CHECK:
		return 0;
	case COMMIO_SETSPEED:
		err = fucpy(&speed, buf, sizeof speed);
		if (err)
			return err;
		if (speed < 2 || speed > 115200)
			return EINVAL;
		set_speed(speed);
		return 0;
	case COMMIO_GETSPEED:
		speed = get_speed();
		return tucpy(buf, &speed, sizeof speed);
	case COMMIO_SETCONF:
		err = fucpy(&conf, buf, sizeof conf);
		if (err)
			return err;
		if (conf.speed < 2 || conf.speed > 115200)
			return EINVAL;
		set_speed(conf.speed);
		return 0;
	case COMMIO_GETCONF:
		memset(&conf, 0, sizeof conf);
		conf.speed = get_speed();
		conf.word_len = 8;
		conf.stop = 1;
		conf.parity = 'n';
		return tucpy(buf, &conf, sizeof conf);
	case COMMIO_STAT:
		memset(&st, 0, sizeof st);
		st.obu = out_count;
		st.ibu = in_count;
		return tucpy(buf, &st, sizeof st);
	}
	return ENOTTY;
}

static void irq(int nr, int count)
{
	static volatile busy;
	unsigned iir;
	
	if (busy)
		panic("irq: busy");
	busy = 1;
	
	while (iir = inb(io_base + 2), !(iir & 1))
	{
		
		switch (iir & 7)
		{
		case 0: /* modem status */
			inb(io_base + 6);
			break;
		case 2: /* transmitter empty */
			/* if (!port_open)
				break; */
			if (out_count)
			{
				outb(io_base, out_buf[out_p1]);
				out_count--;
				out_p1++;
				out_p1 %= BUF_SIZE;
			}
			cio_setstate(&cdev, W_OK);
			resume_tx();
			break;
		case 4: /* received data */
			if (in_count < BUF_SIZE && port_open)
			{
				in_buf[in_p1] = inb(io_base);
				in_count++;
				in_p1++;
				in_p1 %= BUF_SIZE;
			}
			else
				inb(io_base);
			cio_setstate(&cdev, R_OK);
			resume_rx();
			break;
		case 6: /* line status */
			inb(io_base + 5);
			break;
		}
	}
	busy = 0;
}

static void set_speed(unsigned bps)
{
	int s;
	
	s = intr_dis();
	outb(io_base + 3, inb(io_base + 3) | 128);
	outb(io_base    ,  115200 / bps);
	outb(io_base + 1, (115200 / bps) >> 8);
	outb(io_base + 3, inb(io_base + 3) & 127);
	intr_res(s);
}

static int get_speed(void)
{
	int speed;
	int s;
	
	s = intr_dis();
	outb(io_base + 3, inb(io_base + 3) | 128);
	speed = inb(io_base) + (inb(io_base + 1) << 8);
	outb(io_base + 3, inb(io_base + 3) & 127);
	intr_res(s);
	
	return 115200 / speed;
}

int mod_onload(unsigned md, char *pathname, struct device *dev, unsigned dev_size)
{
	int err;
	
	err = kmalloc(&out_buf, BUF_SIZE, dev->name);
	if (err)
		return err;
	
	err = kmalloc(&in_buf, BUF_SIZE, dev->name);
	if (err)
	{
		free(out_buf);
		return err;
	}
	
	task_qinit(&rx_queue, "srx");
	task_qinit(&tx_queue, "stx");
	
	io_base = dev->io_base[0];
	irq_nr = dev->irq_nr[0];
	
	outb(io_base + 3, 3);
	outb(io_base + 1, 3);
	outb(io_base + 2, 0);
	outb(io_base + 4, 11);
	set_speed(19200);
	irq(irq_nr, 0);
	
	irq_set(irq_nr, irq);
	irq_ena(irq_nr);
	
	strcpy(name, dev->name);
	cio_install(&cdev);
	return 0;
}

int mod_onunload(unsigned md)
{
	return EBUSY;
}
