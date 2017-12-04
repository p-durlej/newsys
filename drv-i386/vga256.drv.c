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

#include <kern/machine/bioscall.h>
#include <kern/wingui.h>
#include <kern/lib.h>
#include <dev/framebuf.h>
#include <devices.h>
#include <errno.h>

#define VGA_XRES	320
#define VGA_YRES	200

#define VGA_MEM		0xa0000
#define VGA_PORT	0x03c0

static int modeinfo(void *dd, int mode, struct win_modeinfo *buf);
static int setmode(void *dd, int mode, int refresh);

static void rgba2color(void *dd, int r, int g, int b, int a, win_color *c);
static void color2rgba(void *dd, int *r, int *g, int *b, int *a, win_color c);
static void invert(void *dd, win_color *c);

static void setcte(void *dd, win_color c, int r, int g, int b);

static void no_op(void *dd);

static int disp_installed = 0;

static struct win_rgba color_table[256];

static struct win_desktop *desktop;
static struct framebuf *fb;

static struct win_display disp =
{
	width:		VGA_XRES,
	height:		VGA_YRES,
	ncolors:	256,
	user_cte:	216,
	user_cte_count:	20,
	mode:		0,
	transparent:	(win_color)-1,
	
	modeinfo:	modeinfo,
	setmode:	setmode,
	
	setptr:		fb_setptr_8,
	moveptr:	fb_moveptr_8,
	showptr:	no_op,
	hideptr:	no_op,
	
	putpix:		fb_putpix_8,
	getpix:		fb_getpix_8,
	hline:		fb_hline_8,
	vline:		fb_vline_8,
	rect:		fb_rect_8,
	copy:		fb_copy_8,
	
	rgba2color:	rgba2color,
	color2rgba:	color2rgba,
	invert:		invert,
	
	setcte:		setcte,
};

static void setdac(int i, int r, int g, int b)
{
	outb(VGA_PORT + 0x8, i);
	outb(VGA_PORT + 0x9, r);
	outb(VGA_PORT + 0x9, g);
	outb(VGA_PORT + 0x9, b);
}

static int vga_init(int md)
{
	struct real_regs *reg = get_bios_regs();
	int r, g, b;
	int err;
	int i;
	
	reg->ax = 0x0013;
	reg->intr = 0x10;
	v86_call();
	
	i = 0;
	for (b = 0; b < 6; b++)
		for (g = 0; g < 6; g++)
			for (r = 0; r < 6; r++, i++)
				setcte(NULL, i, r * 51, g * 51, b * 51);
	
	disp.data = fb = fb_creat(&disp, (void *)VGA_MEM, VGA_XRES, VGA_YRES);
	
	err = win_display(md, &disp);
	if (err)
		return err;
	
	disp_installed = 1;
	return 0;
}

static int setmode(void *dd, int mode, int refresh)
{
	if (mode)
		return EINVAL;
	return 0;
}

static int modeinfo(void *dd, int mode, struct win_modeinfo *buf)
{
	if (mode)
		return EINVAL;
	
	buf->width		= VGA_XRES;
	buf->height		= VGA_YRES;
	buf->ncolors		= 256;
	buf->user_cte		= 216;
	buf->user_cte_count	= 20;
	return 0;
}

static void rgba2color(void *dd, int r, int g, int b, int a, win_color *c)
{
	*c  = (r / 51) + (g / 51) * 6 + (b / 51) * 36;
}

static void color2rgba(void *dd, int *r, int *g, int *b, int *a, win_color c)
{
	c &= 255;
	
	*r = color_table[c].r;
	*g = color_table[c].g;
	*b = color_table[c].b;
	*a = 255;
}

static void invert(void *dd, win_color *c)
{
	if (*c < 64)
	{
		*c = 63 - *c;
		return;
	}
}

static void no_op(void *dd)
{
}

static void setcte(void *dd, win_color c, int r, int g, int b)
{
	c &= 255;
	
	color_table[c].r = r;
	color_table[c].g = g;
	color_table[c].b = b;
	
	setdac(c, r >> 2, g >> 2, b >> 2);
}

int mod_onload(unsigned md, char *pathname, struct device *dev, unsigned dev_size)
{
	desktop = win_getdesktop();
	return vga_init(md);
}

int mod_onunload(unsigned md)
{
	if (disp_installed)
		win_uninstall_display(desktop, &disp);
	return 0;
}
