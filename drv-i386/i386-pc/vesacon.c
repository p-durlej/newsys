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
#include <kern/clock.h>
#include <kern/start.h>
#include <kern/errno.h>
#include <kern/lib.h>
#include <dev/console.h>
#include <stdint.h>

#define WIDTH	640
#define HEIGHT	480
#define MODE	0xc101
#define PORT	0x3c0
#define FG	255
#define BG	0
#define FONTH	8
#define FONTW	8
#define LINESPC	2

static void gc_disable(struct console *con);
static void gc_reset(struct console *con);
static void gc_clear(struct console *con);
static int  gc_getxy(struct console *con, int x, int y);
static void gc_putxy(struct console *con, int x, int y, int c);
static void gc_putc(struct console *con, int c);
static void gc_status(struct console *con, int flags);
static void gc_gotoxy(struct console *con, int x, int y);
static void gc_puta(struct console *con, int x, int y, int c, int a);

struct console eccon =
{
	.width		= 80,
	.height		= 48,
	.flags		= CF_COLOR | CF_ADDR,
	
	.disable	= gc_disable,
	.reset		= gc_reset,
	.clear		= gc_clear,
	.putc		= gc_putc,
	.status		= gc_status,
	.gotoxy		= gc_gotoxy,
	.puta		= gc_puta,
};

extern char monofnt[];

static uint8_t *	gc_font = (void *)monofnt;
static uint8_t *	gc_buf;
static volatile int	gc_busy;
static volatile int	gc_cvis;
static int		gc_enabled;
static int		gc_x, gc_y;
static int		gc_anim = 0;

static uint8_t		gc_sbuf[WIDTH * FONTH];

static void gc_setdac(int i, int r, int g, int b)
{
	outb(PORT + 0x8, i);
	outb(PORT + 0x9, r);
	outb(PORT + 0x9, g);
	outb(PORT + 0x9, b);
}

static int gc_posval(struct console *con, int x, int y)
{
	if (x >= 0 && x < con->width && y >= 0 && y < con->height)
		return 1;
	return 0;
}

static void gc_disable(struct console *con)
{
	gc_enabled = 0;
	gc_anim = 0;
}

static void gc_reset(struct console *con)
{
	struct real_regs *r = get_bios_regs();
	int i;
	
	gc_busy++;
	
	r->intr	= 0x10;
	r->ax	= 0x4f02;
	r->bx	= 0xc101;
	v86_call();
	
	for (i = 0; i < 256; i++)
		gc_setdac(i, i >> 2, i >> 2, i >> 2);
	
	gc_enabled = 1;
	gc_clear(con);
	gc_busy--;
}

static void gc_icurs(struct console *con)
{
	uint8_t *p = gc_buf + gc_x * FONTW + gc_y * (FONTH + LINESPC) * WIDTH;
	int x, y;
	
	gc_busy++;
	
	if (gc_posval(con, gc_x, gc_y))
		for (y = FONTH; y < FONTH + LINESPC; y++)
			for (x = 0; x < FONTW; x++)
				p[x + y * WIDTH] ^= 255;
	gc_cvis = !gc_cvis;
	
	gc_busy--;
}

static void gc_hcurs(struct console *con)
{
	if (gc_cvis)
		gc_icurs(con);
}

static void gc_scurs(struct console *con)
{
	if (!gc_cvis)
		gc_icurs(con);
}

static void gc_clear(struct console *con)
{
	gc_busy++;
	memset(gc_buf, 0, WIDTH * HEIGHT);
	gc_x = gc_y = 0;
	gc_cvis = 0;
	gc_busy--;
}

static void gc_rchar(uint8_t *dp, int c)
{
	uint8_t *sp;
	uint8_t row;
	int px, py;
	
	sp = gc_font + (c & 127) * 8;
	for (py = 0; py < 8; py++)
	{
		row = *sp++;
		
		for (px = 0; px < 8; px++)
		{
			*dp++ = (row & 128) ? FG : BG;
			row <<= 1;
		}
		
		dp += WIDTH;
		dp -= 8;
	}
}

static void gc_putxy(struct console *con, int x, int y, int c)
{
	uint8_t *dp;
	int s;
	
	if (!gc_posval(con, x, y))
		return;
	
	if (gc_anim)
		dp = gc_sbuf + x * FONTW;
	else
		dp = gc_buf + x * FONTW + y * (FONTH + LINESPC) * WIDTH;
	
	s = intr_dis();
	
	if (x == gc_x && y == gc_y)
		gc_hcurs(con);
	
	gc_rchar(dp, c);
	
	intr_res(s);
}

static void gc_bll(int y, int r)
{
	uint8_t *dp = gc_buf  + (y + gc_y * FONTH) * WIDTH;
	uint8_t *sp = gc_sbuf +  y * WIDTH;
	int x, i, x0, x1;
	int acc;
	
	for (x = 0; x < WIDTH; x++)
	{
		x0 = x - r;
		x1 = x + r;
		
		if (x0 < 0)
			x0 = 0;
		if (x1 >= WIDTH)
			x1 = WIDTH - 1;
		
		for (acc = 0, i = x0; i <= x1; i++)
			acc += sp[i];
		acc /= 2 * r + 1;
		
		*dp++ = acc;
	}
}

static void gc_blur(int r)
{
	int y;
	
	for (y = 0; y < FONTH; y++)
		gc_bll(y, r);
}

static void gc_anewlin(struct console *con)
{
	int hz = clock_hz();
	int t0 = clock_ticks();
	int du = hz / 4;
	int t, r;
	
	if (!gc_anim)
		return;
	
	do
	{
		t = clock_ticks() - t0;
		if (t < 0)
			t += hz;
		
		r = (du - t) * 80 / hz;
		if (r < 0)
			r = 0;
		
		gc_blur(r);
	} while (du > t);
	
	memset(gc_sbuf, BG, sizeof gc_sbuf);
}

static void gc_scroll(struct console *con)
{
	int len = WIDTH * (HEIGHT - FONTH - LINESPC);
	
	memmove(gc_buf, gc_buf + WIDTH * FONTH, len);
	memset(gc_buf + len, BG, WIDTH * FONTH);
	
	if (gc_y > 0)
		gc_y--;
}

static void gc_clock(void)
{
	static int div;
	
	uint16_t v;
	
	if (++div >= clock_hz() / 2)
	{
		if (gc_enabled)
			gc_icurs(&eccon);
		div = 0;
	}
}

static void gc_putc(struct console *con, int c)
{
	gc_busy++;
	switch (c)
	{
	case '\n':
		gc_anewlin(con);
		gc_gotoxy(con, 0, gc_y + 1);
		if (gc_y >= con->height)
			gc_scroll(con);
		break;
	case '\b':
		if (gc_x > 0)
			gc_gotoxy(con, gc_x - 1, gc_y);
		break;
	default:
		if (gc_x >= con->width)
			gc_gotoxy(con, 0, gc_y + 1);
		if (gc_y >= con->height)
			gc_scroll(con);
		gc_putxy(con, gc_x, gc_y, c);
		gc_gotoxy(con, gc_x + 1, gc_y);
	}
	gc_busy--;
}

static void gc_status(struct console *con, int flags)
{
}

static void gc_gotoxy(struct console *con, int x, int y)
{
	gc_hcurs(con);
	
	gc_x = x;
	gc_y = y;
	
	gc_scurs(con);
}

static int gc_conva(int a)
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

static void gc_puta(struct console *con, int x, int y, int c, int a)
{
	gc_putxy(con, x, y, (c & 255) | gc_conva(a));
}

int gc_init(void)
{
	struct real_regs *r = get_bios_regs();
	uint8_t *buf = get_bios_buf();
	int err;
	
	r->intr	= 0x10;
	r->ax	= 0x4f01;
	r->cx	= 0xc101;
	r->es	= (uint32_t)buf >> 4;
	r->di	= (uint32_t)buf & 15;
	v86_call();
	
	if (r->ax != 0x004f)
	{
		printk("console: vesa call failed\n");
		return ENODEV;
	}
	
	printk("console: lfb at 0x%x\n", *(uint32_t *)(buf + 0x28));
	
	err = phys_map((void **)&gc_buf, *(uint32_t *)(buf + 0x28), WIDTH * HEIGHT, 0);
	if (err)
		return err;
	
	con_install(&eccon);
	clock_ihand(gc_clock);
	
	if (kparam.boot_flags & BOOT_VERBOSE)
		printk("console: vesa\n");
	return 0;
}
