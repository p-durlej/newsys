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
#include <sysload/console.h>

#include <string.h>
#include <stdint.h>

#define CON_BASE	0xb8000

int con_w = 80, con_h = 25;

static int con_x, con_y, con_xynu = 1;
static int con_attr = 0x0700;

static int con_logo;

static void con_scroll(void)
{
	uint16_t *p, *e;
	
	memmove((uint16_t *)CON_BASE, (uint16_t *)CON_BASE + con_w, con_w * (con_h - 1) * 2);
	con_gotoxy(con_x, con_y - 1);
	
	p = (uint16_t *)CON_BASE + (con_h - 1) * con_w;
	e = p + con_w;
	while (p < e)
		*p++ = con_attr | ' ';
}

void con_init(void)
{
	bcp.eax	 = 0x0003;
	bcp.intr = 0x10;
	bioscall();
	
	bcp.eax  = 0x0100;
	bcp.ecx  = 0x0d0f;
	bcp.intr = 0x10;
	bioscall();
}

void con_fini(void)
{
	con_attr = 0;
	con_clear();
}

void con_showlogo(void *data, int size, int width)
{
	char *sp = data;
	char *dp = (char *)0xa0000;
	
	con_logo = 1;
	
	bcp.eax	 = 0x0012;
	bcp.intr = 0x10;
	bioscall();
	
	while (size--)
		*dp++ = *sp++ ^ 255;
}

void con_hidelogo()
{
	if (con_logo)
	{
		con_logo = 0;
		con_init();
	}
}

void con_setattr(int bg, int fg, int attr)
{
	if (attr & INVERSE)
	{
		con_attr  = bg << 8;
		con_attr |= fg << 12;
	}
	else
	{
		con_attr  = bg << 12;
		con_attr |= fg << 8;
	}
	if (attr & BOLD)
		con_attr |= 0x0800;
}

void con_progress(int x, int y, int w, int val, int max)
{
	int i, s;
	
	s = w * val / max;
	con_gotoxy(x, y);
	for (i = 0; i < s; i++)
		con_putc(0xdb);
	for (; i < w; i++)
		con_putc(0xb0);
}

static void con_upxy(void)
{
	if (!con_xynu || con_logo)
		return;
	con_xynu = 0;
	bcp.eax	 = 0x0200;
	bcp.ebx	 = 0x0007;
	bcp.edx	 = (con_y << 8) | con_x;
	bcp.intr = 0x10;
	bioscall();
}

void con_gotoxy(int x, int y)
{
	con_xynu = 1;
	con_x	 = x;
	con_y	 = y;
}

void con_clear(void)
{
	uint16_t *p, *e;
	
	if (con_logo)
		return;
	
	p = (void *)CON_BASE;
	e = p + con_w * con_h;
	while (p < e)
		*p++ = con_attr | ' ';
}

void con_putxy(int x, int y, int c)
{
	uint16_t *p;
	
	if (con_logo)
		return;
	
	if (x < 0 || x >= con_w)
		return;
	if (y < 0 || y >= con_h)
		return;
	con_gotoxy(x, y);
	
	c &= 0xff;
	p  = (void *)CON_BASE;
	p += con_y * con_w;
	p += con_x;
	*p = c | con_attr;
}

void con_putc(unsigned char c)
{
	uint16_t *p;
	
	switch (c)
	{
	case '\n':
		if (con_y >= con_h)
			con_scroll();
		con_gotoxy(0, con_y + 1);
		break;
	case '\r':
		con_gotoxy(0, con_y);
		break;
	case '\b':
		if (con_x)
			con_gotoxy(con_x - 1, con_y);
		break;
	default:
		if (con_x >= con_w)
		{
			con_x = 0;
			con_y++;
		}
		while (con_y >= con_h)
			con_scroll();
		
		p  = (void *)0xb8000;
		p += con_x + con_y * con_w;
		*p = c | con_attr;
		con_gotoxy(con_x + 1, con_y);
	}
}

void con_update(void)
{
	con_upxy();
}

void con_frame(int x, int y, int w, int h)
{
	int i;
	
	for (i = 1; i < w - 1; i++)
	{
		con_putxy(x + i, y + h - 1, 0xc4);
		con_putxy(x + i, y,	    0xc4);
	}
	for (i = 1; i < h - 1; i++)
	{
		con_putxy(x,	     y + i, 0xb3);
		con_putxy(x + w - 1, y + i, 0xb3);
	}
	con_putxy(x + w - 1, y + h - 1, 0xd9);
	con_putxy(x + w - 1, y, 0xbf);
	con_putxy(x, y + h - 1, 0xc0);
	con_putxy(x, y, 0xda);
}

void con_rect(int x0, int y0, int w, int h)
{
	int x, y;
	
	for (y = y0; y < y0 + h; y++)
		for (x = x0; x < x0 + w; x++)
			con_putxy(x, y, ' ');
}

void con_putsxy(int x, int y, const char *s)
{
	con_gotoxy(x, y);
	con_puts(s);
}

void con_puts(const char *s)
{
	while (*s)
		con_putc(*s++);
}

void con_status(const char *s, const char *s1)
{
	con_setattr(0, 7, INVERSE);
	con_rect(0, con_h - 1, con_w, 1);
	con_putsxy(1, con_h - 1, s);
	con_puts(s1);
	con_gotoxy(con_w - 2, con_h - 1);
}

int con_kbhit(void)
{
	int r;
	
	con_upxy();
	
	bcp.eax	 = 0x0100;
	bcp.intr = 0x16;
	bioscall();
	return !(bcp.eflags & (1 << 6));
}

int con_getch(void)
{
	int sc;
	int ch;
	
	con_upxy();
	
	bcp.eax	 = 0x0000;
	bcp.intr = 0x16;
	bioscall();
	
	sc = (bcp.eax >> 8) & 255;
	ch =  bcp.eax & 255;
	
	switch (sc)
	{
	case 0x48:
		return CON_K_UP;
	case 0x50:
		return CON_K_DOWN;
	case 0x47:
		return CON_K_HOME;
	case 0x49:
		return CON_K_PGUP;
	case 0x51:
		return CON_K_PGDN;
	default:
		;
	}
	
	if (ch == '\r')
		return '\n';
	
	return ch;
}
