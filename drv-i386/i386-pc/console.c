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
#include <sysload/flags.h>

#include <kern/machine/bioscall.h>
#include <kern/console.h>
#include <kern/start.h>
#include <kern/clock.h>
#include <dev/console.h>
#include <stdint.h>

#define VC_CATTR	0xf000
#define VC_ATTR		0x1f00

static void vc_disable(struct console *con);
static void vc_reset(struct console *con);
static void vc_clear(struct console *con);
static int  vc_getxy(struct console *con, int x, int y);
static void vc_putxy(struct console *con, int x, int y, int c);
static void vc_putc(struct console *con, int c);
static void vc_status(struct console *con, int flags);
static void vc_gotoxy(struct console *con, int x, int y);
static void vc_puta(struct console *con, int x, int y, int c, int a);

struct console con =
{
	.width		= 80,
	.height		= 50,
	.flags		= CF_COLOR | CF_ADDR,
	
	.disable	= vc_disable,
	.reset		= vc_reset,
	.clear		= vc_clear,
	.putc		= vc_putc,
	.status		= vc_status,
	.gotoxy		= vc_gotoxy,
	.puta		= vc_puta,
};

static uint16_t *	vc_buf	= (void *)0xb8000;
static volatile int	vc_csen;
static volatile int	vc_busy;
static volatile int	vc_csav;
static int		vc_enabled;
static int		vc_x, vc_y;

static int vc_posval(struct console *con, int x, int y)
{
	if (x >= 0 && x < con->width && y >= 0 && y < con->height)
		return 1;
	return 0;
}

static void vc_disable(struct console *con)
{
	vc_enabled = 0;
}

static void vc_reset(struct console *con)
{
	struct real_regs *r = get_bios_regs();
	
	vc_busy++;
	
	r->intr	= 0x10;
	r->ax	= 0x0003;
	v86_call();
	
	if (con->height > 25)
	{
		r->ax	= 0x1112;
		r->bx	= 0x0000;
		v86_call();
	}
	
	r->ax	= 0x0100;
	r->cx	= 0x2000;
	v86_call();
	
	vc_enabled = 1;
	vc_clear(con);
	vc_busy--;
}

static void vc_clear(struct console *con)
{
	int i;
	
	vc_busy++;
	for (i = 1; i < con->width * con->height; i++)
		vc_buf[i] = VC_ATTR | ' ';
	vc_buf[0] = VC_CATTR | ' ';
	vc_x = vc_y = 0;
	vc_busy--;
}

static int vc_getxy(struct console *con, int x, int y)
{
	int s, c;
	
	if (!vc_posval(con, x, y))
		return;
	
	s = intr_dis();
	if (x == vc_x && y == vc_y && vc_csen)
		c = vc_csav;
	else
		c = vc_buf[x + y * con->width];
	intr_res(s);
	
	return c;
}

static void vc_putxy(struct console *con, int x, int y, int c)
{
	int s;
	
	if (!vc_posval(con, x, y))
		return;
	
	s = intr_dis();
	if (x == vc_x && y == vc_y && vc_csen)
	{
		vc_csav = c;
		c &= 255;
		c |= VC_CATTR;
	}
	vc_buf[x + y * con->width] = c;
	intr_res(s);
}

static void vc_scroll(struct console *con)
{
	int w, h;
	int x, y;
	
	w = con->width;
	h = con->height;
	
	for (y = 0; y < h - 1; y++)
		for (x = 0; x < w; x++)
			vc_putxy(con, x, y, vc_getxy(con, x, y + 1));
	
	for (x = 0; x < w; x++)
		vc_putxy(con, x, h - 1, ' ' | VC_ATTR);
	
	vc_gotoxy(con, vc_x, vc_y - 1);
}

static void vc_clock(void)
{
	static int div;
	
	uint16_t v;
	
	if (++div >= clock_hz() / 2)
	{
		vc_csen = !vc_csen;
		if (vc_enabled)
		{
			if (vc_csen && vc_posval(&con, vc_x, vc_y))
			{
				vc_csav = v = vc_buf[vc_x + vc_y * con.width];
				v &= 255;
				v |= VC_CATTR;
				vc_buf[vc_x + vc_y * con.width] = v;
			}
			else
				vc_buf[vc_x + vc_y * con.width] = vc_csav;
		}
		div = 0;
	}
}

static void vc_putc(struct console *con, int c)
{
	vc_busy++;
	switch (c)
	{
	case '\n':
		vc_gotoxy(con, 0, vc_y + 1);
		if (vc_y >= con->height)
			vc_scroll(con);
		break;
	case '\b':
		if (vc_x > 0)
			vc_gotoxy(con, vc_x - 1, vc_y);
		break;
	default:
		if (vc_x >= con->width)
			vc_gotoxy(con, 0, vc_y + 1);
		if (vc_y >= con->height)
			vc_scroll(con);
		
		vc_putxy(con, vc_x, vc_y, VC_ATTR | c);
		vc_gotoxy(con, vc_x + 1, vc_y);
	}
	vc_busy--;
}

static void vc_status(struct console *con, int flags)
{
}

static void vc_gotoxy(struct console *con, int x, int y)
{
	uint16_t v;
	
	if (vc_posval(con, vc_x, vc_y) && vc_csen)
	{
		vc_buf[vc_x + vc_y * con->width] = vc_csav;
	}
	
	vc_x = x;
	vc_y = y;
	
	if (vc_csen && vc_posval(con, vc_x, vc_y))
	{
		v  = vc_buf[vc_x + vc_y * con->width];
		vc_csav = v;
		v &= 0x00ff;
		v |= VC_CATTR;
		vc_buf[vc_x + vc_y * con->width] = v;
	}
}

static int vc_conva(int a)
{
	int va = 0;
	
	va  = (a &  1) << 10;
	va |= (a &  2) <<  8;
	va |= (a &  4) <<  6;
	va |= (a & 16) << 10;
	va |= (a & 32) <<  8;
	va |= (a & 64) <<  6;
	return va;
}

static void vc_puta(struct console *con, int x, int y, int c, int a)
{
	vc_putxy(con, x, y, (c & 255) | vc_conva(a));
}

void con_init(void)
{
	con_install(&con);
	clock_ihand(vc_clock);
	
	if (kparam.boot_flags & BOOT_VERBOSE)
		printk("console: vga text\n");
}
