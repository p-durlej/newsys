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
#include <kern/wingui.h>
#include <kern/printk.h>
#include <kern/start.h>
#include <kern/lib.h>
#include <kern/hw.h>

#include <dev/framebuf.h>
#include <overflow.h>
#include <devices.h>
#include <errno.h>

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

static struct win_rgba color_table_8[256];

static unsigned bytes_per_line	= 0;
static unsigned fbuf_size	= 0;
static unsigned io_base		= 0x3c0;
static void *	fbuf		= NULL;

#define fbuf_32		((volatile unsigned *)fbuf)
#define fbuf_8		((volatile unsigned char *)fbuf)

static int	disp_installed = 0;

static struct win_desktop *desktop;

#define MODE_MAX	256

static struct win_modeinfo *modes;
static int mode_cnt;

static struct win_modeinfo wmi;
static struct win_display disp;

static struct framebuf mfb =
{
	.disp = &disp,
};

static struct framebuf *fb = &mfb;

static int pixbuf_cnt;

static struct win_display disp =
{
	.flags		= WIN_DF_BOOTFB,
	.width		= 0,
	.height		= 0,
	.ncolors	= 16777216,
	.user_cte	= 0,
	.user_cte_count	= 0,
	.mode		= 0,
	.transparent	= (win_color)-1,
	.data		= &mfb,
	
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
		fb = &mfb;
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
	outb(io_base + 0x8, i);
	outb(io_base + 0x9, r);
	outb(io_base + 0x9, g);
	outb(io_base + 0x9, b);
}

static void setcte_8(void *dd, win_color c, int r, int g, int b)
{
	if (c >= 256)
		return;
	
	color_table_8[c].r = r;
	color_table_8[c].g = g;
	color_table_8[c].b = b;
	
	setdac_8(NULL, c, r >> 2, g >> 2, b >> 2);
	if (c >= 216)
		setdac_8(NULL, 256 + 215 - c, (255 - r) >> 2, (255 - g) >> 2, (255 - b) >> 2);
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
	else
		*c = 256 + 215 - *c;
}

static int setup_8(struct kfb *kfb)
{
	int r, g, b;
	int err;
	int i;
	
	wmi.ncolors		= 256;
	wmi.user_cte		= 216;
	wmi.user_cte_count	= 20;
	wmi.width		= kfb->xres;
	wmi.height		= kfb->yres;
	
	bytes_per_line = kfb->bytes_per_line;
	
	fbuf_size = kfb->xres * kfb->yres;
	err = phys_map(&fbuf, kfb->base, fbuf_size, READ_CACHE);
	if (err)
		return err;
	
	mfb.vwidth = kfb->bytes_per_line;
	mfb.width  = kfb->xres;
	mfb.height = kfb->yres;
	mfb.fbuf   = fbuf;
	
	i = 0;
	for (b = 0; b < 6; b++)
		for (g = 0; g < 6; g++)
			for (r = 0; r < 6; r++, i++)
				setcte_8(NULL, i, r * 51, g * 51, b * 51);
	memset(fbuf, 0, kfb->xres * kfb->yres);
	
	disp.width	    = kfb->xres;
	disp.height	    = kfb->yres;
	disp.ncolors	    = 256;
	disp.user_cte	    = 216;
	disp.user_cte_count = 20;
	disp.transparent    = (win_color)-1;
	
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

static int setup_32(struct kfb *kfb)
{
	int err;
	
	wmi.ncolors	   = 16777216;
	wmi.user_cte	   = 0;
	wmi.user_cte_count = 0;
	wmi.width	   = kfb->xres;
	wmi.height	   = kfb->yres;
	
	bytes_per_line = kfb->bytes_per_line;
	
	fbuf_size = kfb->xres * kfb->yres * 4;
	err = phys_map(&fbuf, kfb->base, fbuf_size, READ_CACHE);
	if (err)
		return err;
	
	mfb.vwidth = kfb->bytes_per_line / 4; // XXX
	mfb.width  = kfb->xres;
	mfb.height = kfb->yres;
	mfb.fbuf   = fbuf;
	
	// memset(fbuf, 0, kfb->bytes_per_line * kfb->yres);
	
	disp.width	    = kfb->xres;
	disp.height	    = kfb->yres;
	disp.ncolors	    = 16777216;
	disp.user_cte	    = 0;
	disp.user_cte_count = 0;
	disp.transparent    = 0;
	
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

/* ---- common functions --------------------------------------------------- */

static int bootfb_init(int md)
{
	struct kfb *kfb;
	int i, n;
	int err;
	
	for (kfb = kparam.fb; kfb != NULL; kfb = kfb->next)
	{
		if (kfb->type != KFB_GRAPH)
			continue;
		
		if (kfb->bpp != 32 && kfb->bpp != 8)
			continue;
		
		break;
	}
	if (!kfb)
		return ENODEV;
	
	if (kfb->mode_cnt)
	{
		err = kmalloc(&modes, sizeof *modes * kfb->mode_cnt, "bootfb");
		if (err)
			return err;
		
		disp.mode = -1;
		mode_cnt  = kfb->mode_cnt;
		for (i = n = 0; i < mode_cnt; i++)
		{
			if (kfb->modes[i].type != KFB_GRAPH)
				continue;
			
			switch (kfb->modes[i].bpp)
			{
			case 8:
				modes[n].ncolors	= 256;
				modes[n].user_cte	= 216;
				modes[n].user_cte_count	= 20;
				break;
			case 32:
				modes[n].ncolors	= 16777216;
				modes[n].user_cte	= 0;
				modes[n].user_cte_count	= 0;
				break;
			default:
				continue;
			}
			
			modes[n].width  = kfb->modes[i].xres;
			modes[n].height = kfb->modes[i].yres;
			
			if (kfb->xres == kfb->modes[i].xres &&
			    kfb->yres == kfb->modes[i].yres &&
			    kfb->bpp  == kfb->modes[i].bpp)
				disp.mode = n;
			
			n++;
		}
		
		if (disp.mode < 0)
			panic("bootfb.drv: current mode not found in the list");
	}
	
	switch (kfb->bpp)
	{
	case 8:
		err = setup_8(kfb);
		break;
	case 32:
		err = setup_32(kfb);
		break;
	default:
		return EINVAL;
	}
	if (err)
		return err;
	
	err = win_display(md, &disp);
	if (err)
		return err;
	
	disp_installed = 1;
	return 0;
}

static int setmode(void *dd, int mode, int refresh)
{
	if (modes && mode < mode_cnt)
		return EBUSY;
	if (mode)
		return EINVAL;
	return 0;
}

static int modeinfo(void *dd, int mode, struct win_modeinfo *buf)
{
	if (modes && mode < mode_cnt)
	{
		*buf = modes[mode];
		return 0;
	}
	
	if (mode)
		return EINVAL;
	*buf = wmi;
	return 0;
}

static void no_op()
{
}

int mod_onload(unsigned md, char *pathname, struct device *dev, unsigned dev_size)
{
	desktop = win_getdesktop();
	return bootfb_init(md);
}

int mod_onunload(unsigned md)
{
	if (disp_installed)
		win_uninstall_display(desktop, &disp);
	if (fbuf)
		phys_unmap(fbuf, fbuf_size);
	return 0;
}
