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
#include <kern/wingui.h>
#include <kern/start.h>
#include <kern/clock.h>
#include <kern/intr.h>
#include <kern/lib.h>

#include <sys/types.h>
#include <devices.h>
#include <errno.h>

static unsigned iobase;
static unsigned irq;

static struct win_desktop *desktop;

static unsigned char input[3];
static int input_count;
static int input_enable;
static time_t input_time;

static int button0;
static int button1;

static void packet(void)
{
	int x, y;
	
	input_count = 0;
	
	if (input[1] || input[2])
	{
		x = input[1] | (input[0] & 0x03) << 6;
		y = input[2] | (input[0] & 0x0c) << 4;
		
		if (x >= 128)
			x -= 256;
	
		if (y >= 128)
			y -= 256;
	
		win_ptrmove_rel(desktop, x, y);
	}
	
	if ((!!(input[0] & 0x20)) ^ button0)
	{
		button0 = !button0;
		
		if (button0)
			win_ptrdown(desktop, 0);
		else
			win_ptrup(desktop, 0);
	}
	
	if ((!!(input[0] & 0x10)) ^ button1)
	{
		button1 = !button1;
		
		if (button1)
			win_ptrdown(desktop, 1);
		else
			win_ptrup(desktop, 1);
	}
}

static void irqv(int nr, int count)
{
	unsigned iir = inb(iobase + 2);
	unsigned byte;
	
	while (!(iir & 1))
	{
		switch (iir & 7)
		{
		case 0: /* unwanted modem status interrupt */
			inb(iobase + 6);
			break;
		case 2: /* transmitter holding register empty interrupt */
			break;
		case 4: /* received data interrupt */
			byte = inb(iobase) & 0x7f;
			
			if (!input_enable)
				break;
			
			if (clock_time() > input_time + 2)
				input_count = 0;
			
			if (byte & 0x40)
				input_count = 0;
			
			input_time = clock_time();
			
			input[input_count++] = byte;
			if (input_count == 3)
				packet();
			
			break;
		case 6: /* receiver line status interrupt */
			inb(iobase + 5);
			break;
		}
		
		iir = inb(iobase + 2);
	}
}

static void init_sio()
{
	outb(iobase + 3, 0x82); /* lcr - set dlab */
	outb(iobase    , 0x60); /* divisor latch - 1200 bps */
	outb(iobase + 1, 0x00);
	outb(iobase + 3, 0x02); /* lcr - clear dlab*/
	
	outb(iobase + 1, 0x01); /* ier */
	outb(iobase + 2, 0x00); /* fcr */
	outb(iobase + 4, 0x0b); /* mcr */
	
	irqv(irq, 1);
}

int mod_onload(unsigned md, char *pathname, struct device *dev, unsigned dev_size)
{
	if (kparam.boot_flags & BOOT_VERBOSE)
		printk("mouse.drv: serial mouse driver\n");
	
	iobase = dev->io_base[0];
	irq    = dev->irq_nr[0];
	
	desktop = win_getdesktop();
	
	irq_set(irq, irqv);
	irq_ena(irq);
	
	init_sio();
	clock_delay(clock_hz());
	input_enable = 1;
	
	win_input(md);
	return 0;
}

int mod_onunload(unsigned md)
{
	win_uninstall_input(desktop, md);
	irq_dis(irq);
	return 0;
}
