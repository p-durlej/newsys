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

#include <kern/console.h>
#include <kern/printk.h>
#include <kern/start.h>

#include <sys/types.h>

#define CON_CATTR	0xf0
#define CON_DATTR	0x1f

static int con_sattr = -1;
static int con_x = 0;
static int con_y = 0;

static unsigned		vga_columns;
static unsigned		vga_lines;
static unsigned char *	vga_buf;

static int		bootcon_disabled;

static void bootcon_gotoxy(struct console *con, int x, int y);

static void bootcon_show_cursor(void)
{
	unsigned i;
	
	i = con_x + con_y * vga_columns;
	if (i >= vga_lines * vga_columns)
		return;
	
	i <<= 1;
	i++;
	
	con_sattr = vga_buf[i];
	vga_buf[i] = CON_CATTR;
}

static void bootcon_hide_cursor(void)
{
	unsigned i;
	
	i = con_x + con_y * vga_columns;
	if (i >= vga_lines * vga_columns)
		return;
	
	i <<= 1;
	i++;
	
	vga_buf[i] = con_sattr;
}

static void bootcon_clear(struct console *con)
{
	int i;
	
	bootcon_hide_cursor();
	
	for (i = 0; i < vga_columns * vga_lines * 2; i += 2)
	{
		vga_buf[i    ] = ' ';
		vga_buf[i + 1] = CON_DATTR;
	}
	
	con_x = 0;
	con_y = 0;
	
	bootcon_show_cursor();
}

static void bootcon_scroll(void)
{
	int i;
	
	bootcon_hide_cursor();
	
	for (i = 0; i < vga_columns * (vga_lines - 1) * 2; i++)
		vga_buf[i] = vga_buf[i + vga_columns * 2];
	
	for (i = 0; i < vga_columns * 2; i += 2)
	{
		vga_buf[(vga_lines - 1) * vga_columns * 2 + i    ] = ' ';
		vga_buf[(vga_lines - 1) * vga_columns * 2 + i + 1] = CON_DATTR;
	}
	
	con_y--;
	
	bootcon_show_cursor();
}

static void bootcon_chrxy(int x, int y, char ch)
{
	bootcon_hide_cursor();
	vga_buf[(x + y * vga_columns) * 2] = ch;
	bootcon_show_cursor();
}

static void bootcon_putc(struct console *con, int ch)
{
	if (ch == '\r')
	{
		bootcon_gotoxy(con, 0, con_y);
		return;
	}
	
	if (ch == '\n')
	{
		bootcon_gotoxy(con, 0, con_y + 1);
		return;
	}
	
	if (ch == '\b')
	{
		if (con_x)
			bootcon_gotoxy(con, con_x - 1, con_y);
		return;
	}
	
	if (con_x >= vga_columns)
		bootcon_gotoxy(con, 0, con_y + 1);
	
	while (con_y >= vga_lines)
		bootcon_scroll();
	
	bootcon_chrxy(con_x, con_y, ch);
	bootcon_gotoxy(con, con_x + 1, con_y);
}

static void bootcon_disable(struct console *con)
{
	bootcon_disabled = 1;
}

static void bootcon_reset(struct console *con)
{
	bootcon_disabled = 0;
	
	bootcon_clear(con);
}

static void bootcon_status(struct console *con, int flags)
{
}

static void bootcon_gotoxy(struct console *con, int x, int y)
{
	bootcon_hide_cursor();
	
	con_x = x;
	con_y = y;
	
	bootcon_show_cursor();
}

static void bootcon_puta(struct console *con, int x, int y, int c, int a)
{
	unsigned i;
	
	bootcon_hide_cursor();
	
	i  = x + y * vga_columns;
	if (i >= vga_columns * vga_lines)
		return;
	i *= 2;
	
	vga_buf[i++] = c;
	vga_buf[i  ] = a;
	
	bootcon_show_cursor();
}

static struct console bootcon =
{
	.disable	= bootcon_disable,
	.reset		= bootcon_reset,
	.clear		= bootcon_clear,
	.putc		= bootcon_putc,
	.status		= bootcon_status,
	.gotoxy		= bootcon_gotoxy,
	.puta		= bootcon_puta,
};

void bootcon_init(void)
{
	struct kfb *kfb;
	
	for (kfb = kparam.fb; kfb != NULL; kfb = kfb->next)
		if (kfb->type == KFB_TEXT)
		{
			vga_columns	= bootcon.width	 = kfb->xres;
			vga_lines	= bootcon.height = kfb->yres;
			vga_buf		= (void *)(intptr_t)kfb->base;
			
			bootcon_clear(&bootcon);
			con_install(&bootcon);
			return;
		}
}
