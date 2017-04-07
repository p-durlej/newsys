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

#include <kern/machine-i386-pc/bioscall.h>
#include <kern/wingui.h>
#include <kern/printk.h>
#include <kern/intr.h>
#include <kern/lib.h>
#include <kern/hw.h>
#include <dev/framebuf.h>
#include <overflow.h>
#include <devices.h>
#include <errno.h>

#define WITH_REFRESH	0
#define INVERT_CUSTOM	1

#define VGA_PORT	0x03c0

static int modeinfo(void *dd, int mode, struct win_modeinfo *buf);
static int setmode(void *dd, int mode, int refresh);

static void rgba2color_32(void *dd, int r, int g, int b, int a, win_color *c);
static void color2rgba_32(void *dd, int *r, int *g, int *b, int *a, win_color c);
static void invert_32(void *dd, win_color *c);

static int  newbuf_32(void *dd, struct win_pixbuf *pb);
static void freebuf_32(void *dd, struct win_pixbuf *pb);
static void setbuf_32(void *dd, struct win_pixbuf *pb);
static void copybuf_32(void *dd, struct win_pixbuf *pb, int x, int y);
static void combbuf_32(void *dd, struct win_pixbuf *pb, int x, int y);
static int  swapctl_32(void *dd, int ena);
static void swap_32(void *dd);

static void rgba2color_8(void *dd, int r, int g, int b, int a, win_color *c);
static void color2rgba_8(void *dd, int *r, int *g, int *b, int *a, win_color c);
static void invert_8(void *dd, win_color *c);

static void setcte_8(void *dd, win_color c, int r, int g, int b);

static void no_op();

#if WITH_REFRESH
struct crtc_data
{
	unsigned h_total;
	unsigned h_start;
	unsigned h_end;
	unsigned v_total;
	unsigned v_start;
	unsigned v_end;
};
#endif

static struct win_rgba color_table_8[256];

static unsigned bytes_per_line = 0;
static unsigned fbuf_size = 0;
static void *	fbuf = NULL;

#define fbuf_32		((volatile unsigned *)fbuf)
#define fbuf_8		((volatile unsigned char *)fbuf)

static unsigned vesa_ver = 0;

static int	 disp_installed = 0;

static struct win_desktop *desktop;

#define MODE_MAX	256

static int mode_count = 0;

static struct modeinfo
{
	unsigned bytes_per_line;
	unsigned xres;
	unsigned yres;
	unsigned nr;
	unsigned fbuf;
	unsigned bpp;
#if WITH_REFRESH
	unsigned max_clock;
#endif
} modetab[MODE_MAX];

static struct win_display disp;

static struct framebuf *mfb;
static struct framebuf *fb;

static int pixbuf_cnt;

static struct win_display disp =
{
	.flags		= 0,
	.width		= 0,
	.height		= 0,
	.ncolors	= 16777216,
	.user_cte	= 0,
	.user_cte_count	= 0,
	.mode		= 0,
	.transparent	= (win_color)-1,
	
	.modeinfo	= modeinfo,
	.setmode	= setmode,
	
	.setptr		= fb_setptr_32,
	.moveptr	= fb_moveptr_32,
	.showptr	= no_op,
	.hideptr	= no_op,
	
	.putpix		= fb_putpix_32,
	.getpix		= fb_getpix_32,
	.hline		= fb_hline_32,
	.vline		= fb_vline_32,
	.rect		= fb_rect_32,
	.copy		= fb_copy_32,
	
	.rgba2color	= rgba2color_32,
	.color2rgba	= color2rgba_32,
	.invert		= invert_32,
	
	.setcte		= NULL,
};

/* ---- 32-bit display support functions ----------------------------------- */

static int newbuf_32(void *dd, struct win_pixbuf *pb)
{
	struct framebuf *fb;
	uint32_t *buf;
	size_t bsz;
	int err;
	
	if (ov_mul_size(pb->width, pb->height))
		return EINVAL;
	bsz = pb->width * pb->height;
	if (ov_mul_size(bsz, 4))
		return EINVAL;
	bsz *= 4;
	
	err = kmalloc(&buf, bsz, "vbe: pixbuf");
	if (err)
		return err;
	memset(buf, 0, bsz);
	
	fb = fb_creat(NULL, buf, pb->width, pb->height);
	if (fb == NULL)
	{
		free(buf);
		return ENOMEM;
	}
	
	pb->data = fb;
	pixbuf_cnt++;
	return 0;
}

static void freebuf_32(void *dd, struct win_pixbuf *pb)
{
	struct framebuf *fb = pb->data;
	
	free(fb->fbuf);
	fb_free(fb);
	pixbuf_cnt--;
}

static void setbuf_32(void *dd, struct win_pixbuf *pb)
{
	if (pb == NULL)
		fb = mfb;
	else
		fb = pb->data;
	disp.data = fb;
}

static void copybuf_32_line(void *dd, struct win_pixbuf *pb, int dy, int sy, int x)
{
	struct framebuf *sfb = pb->data;
	const uint32_t *sp;
	uint32_t *dp;
	int off = 0;
	int len;
	
	if (x < 0)
	{
		off = -x;
		x   = 0;
	}
	
	dp  = fb->fbuf;
	dp += fb->width * dy + x;
	
	sp  = sfb->fbuf;
	sp += pb->width * sy + off;
	
	if (pb->width + x > fb->width)
		len = fb->width - x;
	else
		len = pb->width;
	
	if (off + len > pb->width)
		len = pb->width - off;
	if (len < 0)
		return;
	
	memcpy(dp, sp, len << 2);
}

static void copybuf_32(void *dd, struct win_pixbuf *pb, int x, int y)
{
	int yy, y0, y1;
	
	fb_hideptr_32(dd);
	
	y0 = y;
	y1 = y + pb->height;
	if (y0 < 0)
		y0  = 0;
	if (y1 > fb->height)
		y1 = fb->height;
	
	for (yy = y0; yy < y1; yy++)
	{
		copybuf_32_line(dd, pb, yy, yy - y, x);
		y0++;
	}
	
	fb_showptr_32(dd);
}

static void combpix_32(win_color *a, uint32_t v)
{
	uint8_t *dp = (void *)a, *sp = (void *)&v;
	uint32_t al = sp[3];
	
	if (al == 255)
	{
		*a = v;
		return;
	}
	
	dp[0] = (dp[0] * (255 - al) + sp[0] * al) / 255;
	dp[1] = (dp[1] * (255 - al) + sp[1] * al) / 255;
	dp[2] = (dp[2] * (255 - al) + sp[2] * al) / 255;
	dp[3] =  sp[3];
}

static void combbuf_32_line(void *dd, struct win_pixbuf *pb, int dy, int sy, int x)
{
	struct framebuf *sfb = pb->data;
	const uint32_t *sp;
	uint32_t *dp;
	int off = 0;
	int len;
	
	if (x < 0)
	{
		off = -x;
		x   = 0;
	}
	
	dp  = fb->fbuf;
	dp += fb->width * dy + x;
	
	sp  = sfb->fbuf;
	sp += pb->width * sy + off;
	
	if (pb->width + x > fb->width)
		len = fb->width - x;
	else
		len = pb->width;
	
	if (off + len > pb->width)
		len = pb->width - off;
	
	while (len-- > 0)
		combpix_32(dp++, *sp++);
}

static void combbuf_32(void *dd, struct win_pixbuf *pb, int x, int y)
{
	int yy, y0, y1;
	
	if (x > fb->width)
		return;
	
	fb_hideptr_32(dd);
	
	y0 = y;
	y1 = y + pb->height;
	if (y0 < 0)
		y0  = 0;
	if (y1 > fb->height)
		y1 = fb->height;
	
	for (yy = y0; yy < y1; yy++)
	{
		combbuf_32_line(dd, pb, yy, yy - y, x);
		y0++;
	}
	
	fb_showptr_32(dd);
}

static int swapctl_32(void *dd, int ena)
{
	return ENOSYS;
}

static void swap_32(void *dd)
{
}

static void rgba2color_32(void *dd, int r, int g, int b, int a, win_color *c)
{
	*c  = b;
	*c |= g << 8;
	*c |= r << 16;
	*c |= a << 24;
}

static void color2rgba_32(void *dd, int *r, int *g, int *b, int *a, win_color c)
{
	*b =  c		& 0xff;
	*g = (c >> 8)	& 0xff;
	*r = (c >> 16)	& 0xff;
	*a = (c >> 24)	& 0xff;
}

static void invert_32(void *dd, win_color *c)
{
	*c ^= 0x00ffffff;
}

/* ---- 8-bit display support functions ------------------------------------ */

static void setdac_8(void *dd, int i, int r, int g, int b)
{
	outb(VGA_PORT + 0x8, i);
	outb(VGA_PORT + 0x9, r);
	outb(VGA_PORT + 0x9, g);
	outb(VGA_PORT + 0x9, b);
}

static void setcte_8(void *dd, win_color c, int r, int g, int b)
{
	if (c >= 256)
		return;
	
	color_table_8[c].r = r;
	color_table_8[c].g = g;
	color_table_8[c].b = b;
	
	setdac_8(NULL, c, r >> 2, g >> 2, b >> 2);
#if INVERT_CUSTOM
	if (c >= 216)
		setdac_8(NULL, 256 + 215 - c,
			 (255 - r) >> 2, (255 - g) >> 2, (255 - b) >> 2);
#endif
}

static void rgba2color_8(void *dd, int r, int g, int b, int a, win_color *c)
{
	*c = (r / 51) + (g / 51) * 6 + (b / 51) * 36;
}

static void color2rgba_8(void *dd, int *r, int *g, int *b, int *a, win_color c)
{
	if (c >= 256)
	{
		*r = 0;
		*g = 0;
		*b = 0;
	}
	else
	{
		*r = color_table_8[c].r;
		*g = color_table_8[c].g;
		*b = color_table_8[c].b;
	}
	*a = 255;
}

static void invert_8(void *dd, win_color *c)
{
	if (*c < 216)
		*c = 215 - *c;
#if INVERT_CUSTOM
	else
		*c = 256 + 215 - *c;
#endif
}

/* ---- common functions --------------------------------------------------- */

static int vga_trymode(struct modeinfo *m)
{
	unsigned char *bios_buf = get_bios_buf();
	struct real_regs *reg = get_bios_regs();
	
	reg->intr = 0x10;
	reg->ax = 0x4f01;
	reg->cx = m->nr;
	reg->es = (unsigned)bios_buf >> 4;
	reg->di = (unsigned)bios_buf & 0x000f;
	v86_call();
	if (reg->ax != 0x004f)
		return EINVAL;
	
	if ((bios_buf[0x00] & 0x81) != 0x81)
		return EINVAL;
	
	m->bytes_per_line  =	       bios_buf[0x10];
	m->bytes_per_line |= (unsigned)bios_buf[0x11] << 8;
	
	m->xres  = 	     bios_buf[0x12];
	m->xres |= (unsigned)bios_buf[0x13] << 8;
	m->yres  = 	     bios_buf[0x14];
	m->yres |= (unsigned)bios_buf[0x15] << 8;
#if WITH_REFRESH
	if (vesa_ver >= 0x0300)
	{
		m->max_clock  = 	  bios_buf[0x3e];
		m->max_clock |= (unsigned)bios_buf[0x3f] << 8;
		m->max_clock |= (unsigned)bios_buf[0x40] << 16;
		m->max_clock |= (unsigned)bios_buf[0x41] << 24;
	}
#endif
	
	if (bios_buf[0x19] != 32 && bios_buf[0x19] != 8)
		return EINVAL;
	m->bpp = bios_buf[0x19];
	
	m->fbuf  = 	     bios_buf[0x28];
	m->fbuf |= (unsigned)bios_buf[0x29] << 8;
	m->fbuf |= (unsigned)bios_buf[0x2a] << 16;
	m->fbuf |= (unsigned)bios_buf[0x2b] << 24;
	
	return 0;
}

static int vga_init(int md)
{
	unsigned char *bios_buf = get_bios_buf();
	struct real_regs *r = get_bios_regs();
	struct modeinfo *m = modetab;
	int err;
	int i;
	
	r->intr = 0x10;
	r->ax = 0x4f00;
	r->di = (unsigned)bios_buf & 15;
	r->es = (unsigned)bios_buf >> 4;
	v86_call();
	if (r->ax != 0x004f)
		return ENODEV;
	if (memcmp(bios_buf, "VESA", 4))
		return ENODEV;
	vesa_ver  = 	      bios_buf[4];
	vesa_ver |= (unsigned)bios_buf[5] << 8;
	if (vesa_ver < 0x0200)
		return ENODEV;
	
	disp.mode = -1;
	for (i = 0x4100; i < 0x4200 && mode_count < MODE_MAX; i++)
	{
		m->nr = i;
		err = vga_trymode(m);
		if (!err)
		{
			if (m->xres == 640 && m->yres == 480 && disp.mode < 0)
				disp.mode = m - modetab;
			m++;
		}
	}
	if (disp.mode < 0)
		disp.mode = 0;
	mode_count = m - modetab;
	if (!mode_count)
		return ENODEV;
	
	err = setmode(NULL, disp.mode, 0);
	if (err)
		return err;
	
	err = win_display(md, &disp);
	if (err)
		return err;
	
	disp_installed = 1;
	return 0;
}

#if WITH_REFRESH
static void make_crtc(struct crtc_data *cd, int xres, int yres)
{
	cd->h_total = (xres * 130) / 100;
	cd->h_start = (xres * 104) / 100;
	cd->h_end   = (xres * 112) / 100;
	cd->v_total = (xres * 110) / 100;
	cd->v_start =  xres + 1;
	cd->v_end   =  xres + 4;
}
#endif

static int setmode_8(struct modeinfo *m)
{
	struct real_regs *reg = get_bios_regs();
	int r, g, b;
	int err;
	int i;
	
	fbuf_size = m->xres * m->yres;
	err = phys_map(&fbuf, m->fbuf, fbuf_size, READ_CACHE);
	if (err)
		return err;
	
	if (mfb)
		fb_free(mfb);
	
	fb = mfb = fb_creat5(&disp, fbuf, m->xres, m->yres, m->bytes_per_line);
	
	reg->ax	  = 0x4f02;
	reg->bx	  = m->nr;
	reg->intr = 0x10;
	v86_call();
	if (reg->ax != 0x004f)
		return ENODEV;
	
	i = 0;
	for (b = 0; b < 6; b++)
		for (g = 0; g < 6; g++)
			for (r = 0; r < 6; r++, i++)
				setcte_8(NULL, i, r * 51, g * 51, b * 51);
	memset(fbuf, 0, m->xres * m->yres);
	
	disp.width	= m->xres;
	disp.height	= m->yres;
	disp.ncolors	= 256;
	disp.user_cte	= 216;
#if INVERT_CUSTOM
	disp.user_cte_count = 20;
#else
	disp.user_cte_count = 40;
#endif
	disp.mode	    = m - modetab;
	disp.transparent    = (win_color)-1;
	disp.data	    = mfb;
	
	disp.setptr	= fb_setptr_8;
	disp.moveptr	= fb_moveptr_8;
	
	disp.putpix	= fb_putpix_8;
	disp.getpix	= fb_getpix_8;
	disp.hline	= fb_hline_8;
	disp.vline	= fb_vline_8;
	disp.rect	= fb_rect_8;
	disp.copy	= fb_copy_8;
	disp.rgba2color	= rgba2color_8;
	disp.color2rgba	= color2rgba_8;
	disp.invert	= invert_8;
	disp.setcte	= setcte_8;
	disp.newbuf	= NULL;
	disp.freebuf	= NULL;
	disp.setbuf	= NULL;
	disp.copybuf	= NULL;
	disp.combbuf	= NULL;
	disp.swapctl	= NULL;
	disp.swap	= NULL;
	return 0;
}

static int setmode(void *dd, int mode, int refresh)
{
	struct real_regs *r = get_bios_regs();
	struct modeinfo *m;
	int err;
	
	if (pixbuf_cnt)
		return EBUSY;
	
	if (mode < 0 || mode >= mode_count)
		return EINVAL;
	m = &modetab[mode];
	
	if (fbuf)
	{
		phys_unmap(fbuf, fbuf_size);
		fbuf = NULL;
	}
	
	if (m->bpp == 8)
		return setmode_8(m);
	
#if WITH_REFRESH
	if (vesa_ver < 0x0300 || !refresh)
	{
#endif
		r->ax = 0x4f02;
		r->bx = modetab[mode].nr;
		r->intr = 0x10;
		v86_call();
		if (r->ax != 0x004f)
			return EINVAL;
#if WITH_REFRESH
	}
	else
	{
		unsigned char *bios_buf = get_bios_buf();
		struct crtc_data cd;
		unsigned clock;
		
		make_crtc(&cd, m->xres, m->yres);
		clock = refresh * (cd.h_total * cd.v_total) / 100;
		
		printk("vbe.drv: cd.h_total = %i\n", cd.h_total);
		printk("vbe.drv: cd.h_start = %i\n", cd.h_start);
		printk("vbe.drv: cd.h_end   = %i\n", cd.h_end);
		printk("vbe.drv: cd.v_total = %i\n", cd.v_total);
		printk("vbe.drv: cd.v_start = %i\n", cd.v_start);
		printk("vbe.drv: cd.v_end   = %i\n", cd.v_end);
		printk("vbe.drv: refresh    = %i (in units of .01 Hz)\n", refresh);
		printk("vbe.drv: clock      = %i Hz\n", clock);
		
		r->intr = 0x10;
		r->eax = 0x4f0b;
		r->ebx = 0x0000;
		r->ecx = clock;
		r->edx = m->nr | (1 << 11);
		v86_call();
		if (r->ax == 0x004f && r->ecx)
		{
			clock = r->ecx;
			refresh = clock * 100 / (cd.h_total * cd.v_total);
			
			printk("vbe.drv: refresh    = %i (in units of .01 Hz)\n", refresh);
			printk("vbe.drv: clock      = %i Hz\n", clock);
		}
		else
			printk("vbe.drv: r->ax = %x\n", r->ax);
		
		bios_buf[ 0] = cd.h_total;
		bios_buf[ 1] = cd.h_total >> 8;
		bios_buf[ 2] = cd.h_start;
		bios_buf[ 3] = cd.h_start >> 8;
		bios_buf[ 4] = cd.h_end;
		bios_buf[ 5] = cd.h_end   >> 8;
		bios_buf[ 6] = cd.v_total;
		bios_buf[ 7] = cd.v_total >> 8;
		bios_buf[ 8] = cd.v_start;
		bios_buf[ 9] = cd.v_start >> 8;
		bios_buf[10] = cd.v_end;
		bios_buf[11] = cd.v_end   >> 8;
		bios_buf[12] = 0;
		bios_buf[13] = clock;
		bios_buf[14] = clock >> 8;
		bios_buf[15] = clock >> 16;
		bios_buf[16] = clock >> 24;
		bios_buf[17] = refresh;
		bios_buf[18] = refresh >> 8;
		
		r->es = (unsigned)bios_buf >> 4;
		r->di = (unsigned)bios_buf & 15;
		r->ax = 0x4f02;
		r->bx = modetab[mode].nr | (1 << 11);
		v86_call();
		if (r->ax != 0x004f)
			return EINVAL;
	}
#endif
	bytes_per_line = modetab[mode].bytes_per_line;
	
	fbuf_size = modetab[mode].xres * modetab[mode].yres * 4;
	err = phys_map(&fbuf, modetab[mode].fbuf, fbuf_size, READ_CACHE);
	if (err)
		return err;
	
	if (mfb)
		fb_free(mfb);
	
	fb = mfb = fb_creat5(&disp, fbuf, m->xres, m->yres, m->bytes_per_line / 4);
	
	memset(fbuf, 0, sizeof *fbuf * m->xres * m->yres);
	
	disp.width	    = m->xres;
	disp.height	    = m->yres;
	disp.ncolors	    = 16777216;
	disp.user_cte	    = 0;
	disp.user_cte_count = 0;
	disp.mode	    = mode;
	disp.transparent    = 0;
	disp.data	    = mfb;
	
	disp.setptr	= fb_setptr_32;
	disp.moveptr	= fb_moveptr_32;
	
	disp.putpix	= fb_putpix_32;
	disp.getpix	= fb_getpix_32;
	disp.hline	= fb_hline_32;
	disp.vline	= fb_vline_32;
	disp.rect	= fb_rect_32;
	disp.copy	= fb_copy_32;
	disp.rgba2color	= rgba2color_32;
	disp.color2rgba	= color2rgba_32;
	disp.invert	= invert_32;
	disp.setcte	= no_op;
	disp.newbuf	= newbuf_32;
	disp.freebuf	= freebuf_32;
	disp.setbuf	= setbuf_32;
	disp.copybuf	= copybuf_32;
	disp.combbuf	= combbuf_32;
	disp.swapctl	= swapctl_32;
	disp.swap	= swap_32;
	return 0;
}

static int modeinfo(void *dd, int mode, struct win_modeinfo *buf)
{
	struct modeinfo *m;
	
	if (mode < 0 || mode >= mode_count)
		return EINVAL;
	m = &modetab[mode];
	
	if (m->bpp == 32)
	{
		buf->ncolors		= 16777216;
		buf->user_cte		= 0;
		buf->user_cte_count	= 0;
	}
	else
	{
		buf->ncolors		= 256;
		buf->user_cte		= 216;
#if INVERT_CUSTOM
		buf->user_cte_count	= 20;
#else
		buf->user_cte_count	= 40;
#endif
	}
	buf->width  = modetab[mode].xres;
	buf->height = modetab[mode].yres;
	return 0;
}

static void no_op()
{
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
	if (fbuf)
		phys_unmap(fbuf, fbuf_size);
	return 0;
}
