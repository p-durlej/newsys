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

#include <kern/machine/machine.h>
#include <machine/machdef.h>
#include <sys/io.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <os386.h>
#include <paths.h>
#include <time.h>

#define IOBASE		0x70

#if defined __MACH_I386_PC__ || defined __MACH_AMD64_PC__

static unsigned bcdint(unsigned v)
{
	return (v >> 4) * 10 + (v & 0x0f);
}

static unsigned intbcd(unsigned v)
{
	return (v % 10) + ((v / 10) << 4);
}

static void rtc_read(void)
{
	struct tm tm;
	time_t t;
	
	if (_iopl(3))
	{
		perror("iopl");
		exit(errno);
	}
	
	do
	{
		outb(IOBASE, 0x00);	tm.tm_sec  = bcdint(inb(IOBASE + 1));
		outb(IOBASE, 0x02);	tm.tm_min  = bcdint(inb(IOBASE + 1));
		outb(IOBASE, 0x04);	tm.tm_hour = bcdint(inb(IOBASE + 1));
		outb(IOBASE, 0x07);	tm.tm_mday = bcdint(inb(IOBASE + 1));
		outb(IOBASE, 0x08);	tm.tm_mon  = bcdint(inb(IOBASE + 1)) - 1;
		outb(IOBASE, 0x09);	tm.tm_year = 100 + bcdint(inb(IOBASE + 1));
		outb(IOBASE, 0x00);
	}
	while (tm.tm_sec != bcdint(inb(IOBASE + 1)));
	
	t = mktime(&tm);
	if (stime(&t))
	{
		perror("stime");
		exit(errno);
	}
	
	if (_boot_flags() & BOOT_VERBOSE)
		fprintf(stderr, "hwclock: %s", ctime(&t));
}

static void rtc_write(void)
{
	struct tm *tm;
	time_t t;
	
	time(&t);
	tm = localtime(&t);
	
	if (_iopl(3))
	{
		perror("iopl");
		exit(errno);
	}
	
	outb(IOBASE,	 0x00);
	outb(IOBASE + 1, 0x00);
	
	outb(IOBASE, 0x09);	outb(IOBASE + 1, intbcd(tm->tm_year - 100));
	outb(IOBASE, 0x08);	outb(IOBASE + 1, intbcd(tm->tm_mon + 1));
	outb(IOBASE, 0x07);	outb(IOBASE + 1, intbcd(tm->tm_mday));
	outb(IOBASE, 0x06);	outb(IOBASE + 1, intbcd(tm->tm_wday + 1));
	outb(IOBASE, 0x04);	outb(IOBASE + 1, intbcd(tm->tm_hour));
	outb(IOBASE, 0x02);	outb(IOBASE + 1, intbcd(tm->tm_min));
	outb(IOBASE, 0x00);	outb(IOBASE + 1, intbcd(tm->tm_sec));
}

#else
#error unknown machine type
#endif

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "usage: hwclock r|w\n");
		return 255;
	}
	
	switch (*argv[1])
	{
	case 'r':
		rtc_read();
		break;
	case 'w':
		rtc_write();
		break;
	default:
		fprintf(stderr, "usage: hwclock r|w\n");
		return 255;
	}
	return 0;
}
