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

#include <kern/machine-i386-pc/bioscall.h>
#include <kern/printk.h>
#include <kern/wingui.h>
#include <kern/lib.h>
#include <kern/hw.h>
#include <devices.h>
#include <errno.h>

#define GRAY		0

#define MODE_COUNT	3

#define MAX_XRES	640
#define MAX_YRES	480

#define VGA_PORT	0x03c0
#define VGA_MEM		0xa0000

static int modeinfo(void *dd, int mode, struct win_modeinfo *buf);
static int setmode(void *dd, int mode, int refresh);

static void setptr(void *dd, const win_color *shape, const unsigned *mask);
static void moveptr(void *dd, int x, int y);
static void showptr(void *dd);
static void hideptr(void *dd);

static void p_putpix(void *dd, int x, int y, win_color c);
static void p_getpix(void *dd, int x, int y, win_color *c);
static void putpix(void *dd, int x, int y, win_color c);
static void getpix(void *dd, int x, int y, win_color *c);
static void hline(void *dd, int x, int y, int len, win_color c);
static void vline(void *dd, int x, int y, int len, win_color c);
static void rect(void *dd, int x, int y, int w, int h, win_color c);
static void copy(void *dd, int x0, int y0, int x1, int y1, int w, int h);

static void rgba2color(void *dd, int r, int g, int b, int a, win_color *c);
static void color2rgba(void *dd, int *r, int *g, int *b, int *a, win_color c);
static void invert(void *dd, win_color *c);

static volatile unsigned char *fbuf = (void *)VGA_MEM;
static unsigned char *vfbuf;

static int ptr_x;
static int ptr_y;
static win_color ptr_back[PTR_WIDTH * PTR_HEIGHT];
static win_color ptr_shape[PTR_WIDTH * PTR_HEIGHT];
static unsigned ptr_mask[PTR_WIDTH * PTR_HEIGHT];
static unsigned ptr_hide_count = 0;
static int disp_installed = 0;

static struct win_desktop *desktop;

static struct win_display disp =
{
	flags:		WIN_DF_SLOWCOPY,
	width:		0,
	height:		0,
	ncolors:	16,
	user_cte:	0,
	user_cte_count:	0,
	mode:		0,
	transparent:	(win_color)-1,
	
	modeinfo:	modeinfo,
	setmode:	setmode,
	
	setptr:		setptr,
	moveptr:	moveptr,
	showptr:	showptr,
	hideptr:	hideptr,
	
	putpix:		putpix,
	getpix:		getpix,
	hline:		hline,
	vline:		vline,
	rect:		rect,
	copy:		copy,
	
	rgba2color:	rgba2color,
	color2rgba:	color2rgba,
	invert:		invert,
	
	setcte:		NULL,
};

static struct
{
	int xres;
	int yres;
	int nr;
} modetab[MODE_COUNT] =
{
	{ 640, 480, 0x12 },
 	{ 640, 200, 0x0e },
	{ 320, 200, 0x0d }
};

static void setdac(int i, int r, int g, int b)
{
	outb(VGA_PORT + 0x8, i);
	outb(VGA_PORT + 0x9, r);
	outb(VGA_PORT + 0x9, g);
	outb(VGA_PORT + 0x9, b);
}

static void wrireg(int i, int v)
{
	inb(VGA_PORT + 0x1a);
	outb(VGA_PORT, i);
	outb(VGA_PORT, v);
}

static int vga_init(int md)
{
	int err;
	
	err = kmalloc(&vfbuf, MAX_XRES * MAX_YRES, "vga");
	if (err)
	{
		printk("vga.drv: cannot allocate virtual framebuffer\n");
		return ENOMEM;
	}
	memset(vfbuf, 0, MAX_XRES * MAX_YRES);
	
	setmode(NULL, 0, 0);
	err = win_display(md, &disp);
	if (err)
		return err;
	disp_installed = 1;
	return 0;
}

static int setmode(void *dd, int mode, int refresh)
{
	struct real_regs *reg = get_bios_regs();
	int r, g, b, a;
	int i, n;
	
	if (mode < 0 || mode >= MODE_COUNT)
		return EINVAL;
	
	reg->ax = modetab[mode].nr;
	reg->intr = 0x10;
	v86_call();
	
	disp.width   = modetab[mode].xres;
	disp.height  = modetab[mode].yres;
	disp.ncolors = 16;
	disp.mode    = mode;
	
	for (i = 0; i < 16; i++) /* color index registers */
		wrireg(i, i);
	wrireg(0x11, 16);     /* border color */
	wrireg(0x10, 1);      /* enable graphics */
	outb(VGA_PORT, 0x20); /* unblank screen */
	
	for (n = 0; n < 16; n++)
		for (i = 0; i < 16; i++)
		{
			color2rgba(NULL, &r, &g, &b, &a, i);
			setdac(i + n * 16, r >> 2, g >> 2, b >> 2);
		}
	setdac(16, 0, 128, 255);
	
	outb(VGA_PORT + 0xe, 0x05);
	outb(VGA_PORT + 0xf, 0x02);
	
	return 0;
}

static int modeinfo(void *dd, int mode, struct win_modeinfo *buf)
{
	if (mode < 0 || mode >= MODE_COUNT)
		return EINVAL;
	
	buf->width		= modetab[mode].xres;
	buf->height		= modetab[mode].yres;
	buf->ncolors		= 16;
	buf->user_cte		= 0;
	buf->user_cte_count	= 0;
	return 0;
}

static void setptr(void *dd, const win_color *shape, const unsigned *mask)
{
	hideptr(NULL);
	memcpy(ptr_shape, shape, sizeof ptr_shape);
	memcpy(ptr_mask, mask, sizeof ptr_mask);
	showptr(NULL);
}

static void moveptr(void *dd, int x, int y)
{
	hideptr(NULL);
	ptr_x = x;
	ptr_y = y;
	showptr(NULL);
}

static void p_putpix(void *dd, int x, int y, win_color c)
{
	if (x < 0 || x >= disp.width)
		return;
	if (y < 0 || y >= disp.height)
		return;
	putpix(NULL, x, y, c);
}

static void p_getpix(void *dd, int x, int y, win_color *c)
{
	if (x < 0 || x >= disp.width)
		return;
	if (y < 0 || y >= disp.height)
		return;
	getpix(NULL, x, y, c);
}

static void showptr(void *dd)
{
	static int x, y;
	static int i;
	
	if (!ptr_hide_count)
		return;
	
	ptr_hide_count--;
	if (ptr_hide_count)
		return;
	
	for (i = 0, y = 0; y < PTR_HEIGHT; y++)
		for (x = 0; x < PTR_WIDTH; x++)
		{
			p_getpix(NULL, x + ptr_x, y + ptr_y, &ptr_back[i]);
			if (ptr_mask[i])
				p_putpix(NULL, x + ptr_x, y + ptr_y, ptr_shape[i]);
			i++;
		}
}

static void hideptr(void *dd)
{
	static int x, y;
	static int i;
	
	ptr_hide_count++;
	if (ptr_hide_count > 1)
		return;
	
	i = 0;
	for (y = 0; y < PTR_HEIGHT; y++)
		for (x = 0; x < PTR_WIDTH; x++)
		{
			if (ptr_mask[i])
				p_putpix(NULL, x + ptr_x, y + ptr_y, ptr_back[i]);
			i++;
		}
}

static void putpix(void *dd, int x, int y, win_color c)
{
	static unsigned shift;
	static unsigned latch;
	static unsigned i;
	
	i = x + y * disp.width;
	
	vfbuf[i] = c;
	
	shift = 7 - i % 8;
	i /= 8;
	
	outb(VGA_PORT + 0xe, 0x08);
	outb(VGA_PORT + 0xf, 1 << shift);
	
	latch = fbuf[i];
	fbuf[i] = c;
}

static void getpix(void *dd, int x, int y, win_color *c)
{
	static unsigned i;
	
	i = x + y * disp.width;
	
	*c = vfbuf[i];
}

static void hline_old(void *dd, int x, int y, int len, win_color c)
{
	int i;
	
	for (i = x; i < x + len; i++)
		putpix(NULL, i, y, c);
}

static void hline(void *dd, int x, int y, int len, win_color c)
{
	int start;
	int end;
	int i;
	
	if (len < 8)
		hline_old(NULL, x, y, len, c);
	else
	{
		memset(&vfbuf[x + y * disp.width], c, len);
		
		start = ((x + 7) >> 3) + (y * disp.width >> 3);
		end = ((x + len) >> 3) + (y * disp.width >> 3);
		
		outb(VGA_PORT + 0xe, 0x08);
		outb(VGA_PORT + 0xf, 0xff);
		for (i = start; i < end; i++)
			fbuf[i] = c;
		
		for (i = x; i < ((x + 8) & ~7); i++)
			putpix(NULL, i, y, c);
		for (i = (x + len) & ~7; i < x + len; i++)
			putpix(NULL, i, y, c);
	}
}

static void vline(void *dd, int x, int y, int len, win_color c)
{
	int i;
	
	for (i = y; i < y + len; i++)
		putpix(NULL, x, i, c);
}

static void rect(void *dd, int x, int y, int w, int h, win_color c)
{
	int i;
	
	for (i = y; i < y + h; i++)
		hline(NULL, x, i, w, c);
}

static void copy_rev(int x0, int y0, int x1, int y1, int w, int h)
{
	static win_color c;
	static int x, y;
	
	for (y = h - 1; y >= 0; y--)
		for (x = w - 1; x >= 0; x--)
		{
			getpix(NULL, x + x1, y + y1, &c);
			putpix(NULL, x + x0, y + y0, c);
		}
}

static void copy(void *dd, int x0, int y0, int x1, int y1, int w, int h)
{
	static win_color c;
	static int x, y;
	
	if (y0 > y1 || (y0 == y1 && x0 > x1))
		copy_rev(x0, y0, x1, y1, w, h);
	else
	{
		for (y = 0; y < h; y++)
			for (x = 0; x < w; x++)
			{
				getpix(NULL, x + x1, y + y1, &c);
				putpix(NULL, x + x0, y + y0, c);
			}
	}
}

#if GRAY

static void rgba2color(void *dd, int r, int g, int b, int a, win_color *c)
{
	if (r < 0 || g < 0 || b < 0)
		panic("vga.drv: rgba2color: r < 0 || g < 0 || b < 0");
	
	*c = (r * 30 + g * 59 + b * 11) / 1700;
}

static void color2rgba(void *dd, int *r, int *g, int *b, int *a, win_color c)
{
	c &= 15;
	
	*r = c * 17;
	*g = c * 17;
	*b = c * 17;
	*a = 255;
}

#else

#define abs(v)		((v) < 0   ? -(v) : (v))
#define max(a, b)	((a) < (b) ?  (b) : (a))

static void rgba2color(void *dd, int r, int g, int b, int a, win_color *c)
{
	win_color cv = 0;
	int max;
	
	max = max(r, max(g, b));
	if (max < 32)
	{
		*c = 0;
		return;
	}
	
	if (r * 4 > max * 3)
		cv |= 1;
	if (g * 4 > max * 3)
		cv |= 2;
	if (b * 4 > max * 3)
		cv |= 4;
	
	if (max > 192)
		cv |= 8;
	else if (max >= 192 && cv == 7)
		cv = 8;
	*c = cv;
}

static void color2rgba(void *dd, int *r, int *g, int *b, int *a, win_color c)
{
	int br;
	
	if (c == 8)
	{
		*r = 192;
		*g = 192;
		*b = 192;
		*a = 255;
		return;
	}
	
	if (c & 0x08)
		br = 255;
	else
		br = 128;
	
	if (c & 0x01)
		*r = br;
	else
		*r = 0;
	
	if (c & 0x02)
		*g = br;
	else
		*g = 0;
	
	if (c & 0x04)
		*b = br;
	else
		*b = 0;
	
	*a = 255;
}

#endif

static void invert(void *dd, win_color *c)
{
	*c = 15 - *c;
}

int mod_onload(unsigned md, char *pathname, struct device *dev, int dev_size)
{
	desktop = win_getdesktop();
	return vga_init(md);
}

int mod_onunload(unsigned md)
{
	if (disp_installed)
		win_uninstall_display(desktop, &disp);
	free(vfbuf);
	return 0;
}
