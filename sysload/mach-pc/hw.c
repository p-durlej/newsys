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

#include <sysload/i386-pc.h>
#include <sysload/kparam.h>
#include <sysload/main.h>
#include <sysload/hw.h>

#include <sys/io.h>

#define CASCADE_IRQ	2

static void a20_init(void)
{
	while (inb(0x64) & 2);
	outb(0x64, 0xd1);
	while (inb(0x64) & 2);
	outb(0x60, 0xff);
	while (inb(0x64) & 2);
}

void hw_init(void)
{
	a20_init();
}

void hw_fini(void)
{
	outb(0x20, 0x11);
	outb(0x21, 0xf0);
	outb(0x21, 1 << CASCADE_IRQ);
	outb(0x21, 0x01);
	
	outb(0xa0, 0x11);
	outb(0xa1, 0xf0);
	outb(0xa1, CASCADE_IRQ);
	outb(0xa1, 0x01);
	
	outb(0x21, 0xff);
	outb(0xa1, 0xff);
}

long clock_time(void)
{
	long t;
	
	bcp.intr = 0x1a;
	bcp.eax	 = 0x0000;
	bioscall();
	
	t  =  bcp.edx & 0xffff;
	t |= (bcp.ecx & 0xffff) << 16;
	return t;
}

int clock_hz(void)
{
	return 18;
}

void mach_fill_kparam(struct kparam *kp)
{
	kp->conv_mem_size = conv_mem_hbrk;
	kp->fb		  = vga_init();
}
