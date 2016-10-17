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

#include <kern/wingui.h>
#include <kern/lib.h>
#include <dev/framebuf.h>
#include <stdint.h>

static void fb_putpix_p_32(void *dd, int x, int y, win_color c);
static void fb_getpix_p_32(void *dd, int x, int y, win_color *c);

static void fb_putpix_p_8(void *dd, int x, int y, win_color c);
static void fb_getpix_p_8(void *dd, int x, int y, win_color *c);

struct framebuf *fb_creat(struct win_display *disp, void *buf, int w, int h)
{
	struct framebuf *fb;
	int err;
	
	err = kmalloc(&fb, sizeof *fb, "fb");
	if (err)
		return NULL;
	memset(fb, 0, sizeof *fb);
	fb->width  = w;
	fb->height = h;
	fb->fbuf   = buf;
	fb->disp   = disp;
	return fb;
}

void fb_reset(struct framebuf *fb)
{
}

void fb_free(struct framebuf *fb)
{
	free(fb);
}

void fb_setptr_32(void *dd, const win_color *shape, const unsigned *mask)
{
	struct framebuf *fb = dd;
	
	fb_hideptr_32(dd);
	memcpy(fb->ptr_shape, shape, sizeof fb->ptr_shape);
	memcpy(fb->ptr_mask,  mask,  sizeof fb->ptr_mask);
	fb_showptr_32(dd);
}

void fb_moveptr_32(void *dd, int x, int y)
{
	win_color oback[PTR_WIDTH * PTR_HEIGHT];
	struct framebuf *fb = dd;
	int x0, y0, x1, y1, x2, y2;
	int px, py;
	int i;
	
	if (fb->ptr_hide_count)
	{
		fb->ptr_x = x;
		fb->ptr_y = y;
	}
	
	memcpy(oback, fb->ptr_back, sizeof oback);
	x0 = fb->ptr_x;
	y0 = fb->ptr_y;
	
	fb->ptr_hide_count = 1;
	fb->ptr_x = x;
	fb->ptr_y = y;
	fb_showptr_32(dd);
	
	x1 = x0;
	y1 = y0;
	
	x2 = x0 + PTR_WIDTH;
	y2 = y0 + PTR_WIDTH;
	
	if (x1 < 0)		x1 = 0;
	if (y1 < 0)		y1 = 0;
	
	if (x2 > fb->width)	x2 = fb->width;
	if (y2 > fb->height)	y2 = fb->height;
	
	for (y = y1; y < y2; y++)
		for (x = x1; x < x2; x++)
		{
			i = (x - x0) + (y - y0) * PTR_WIDTH;
			
			if (fb->ptr_mask[i])
				fb_putpix_32(dd, x, y, oback[i]);
		}
}

void fb_showptr_32(void *dd)
{
	struct framebuf *fb = dd;
	int x0, x1, y0, y1;
	int xs, ys;
	int x, y;
	int i;
	
	if (!fb->ptr_hide_count)
		panic("fb_showptr_32: !fb->ptr_hide_count");
	
	fb->ptr_hide_count--;
	if (fb->ptr_hide_count)
		return;
	
	x0 = fb->ptr_x;
	x1 = fb->ptr_x + PTR_WIDTH;
	y0 = fb->ptr_y;
	y1 = fb->ptr_y + PTR_HEIGHT;
	
	if (x0 > fb->width)
		x0 = fb->width;
	if (x0 < 0)
		x0 = 0;
	
	if (x1 > fb->width)
		x1 = fb->width;
	if (x1 < 0)
		x1 = 0;
	
	if (y0 > fb->height)
		y0 = fb->height;
	if (y0 < 0)
		y0 = 0;
	
	if (y1 > fb->height)
		y1 = fb->height;
	if (y1 < 0)
		y1 = 0;
	
	xs = fb->ptr_x;
	ys = fb->ptr_y;
	
	for (y = y0; y < y1; y++)
		for (x = x0; x < x1; x++)
		{
			i = x - xs + (y - ys) * PTR_WIDTH;
			
			fb_getpix_p_32(dd, x, y, &fb->ptr_back[i]);
			if (fb->ptr_mask[i])
				fb_putpix_p_32(dd, x, y, fb->ptr_shape[i]);
		}
}

void fb_hideptr_32(void *dd)
{
	struct framebuf *fb = dd;
	int x0, x1, y0, y1;
	int xs, ys;
	int x, y;
	int i;
	
	fb->ptr_hide_count++;
	if (fb->ptr_hide_count > 1)
		return;
	
	x0 = fb->ptr_x;
	x1 = fb->ptr_x + PTR_WIDTH;
	y0 = fb->ptr_y;
	y1 = fb->ptr_y + PTR_HEIGHT;
	
	if (x0 > fb->width)
		x0 = fb->width;
	if (x0 < 0)
		x0 = 0;
	
	if (x1 > fb->width)
		x1 = fb->width;
	if (x1 < 0)
		x1 = 0;
	
	if (y0 > fb->height)
		y0 = fb->height;
	if (y0 < 0)
		y0 = 0;
	
	if (y1 > fb->height)
		y1 = fb->height;
	if (y1 < 0)
		y1 = 0;
	
	xs = fb->ptr_x;
	ys = fb->ptr_y;
	
	for (y = y0; y < y1; y++)
		for (x = x0; x < x1; x++)
		{
			i = x - xs + (y - ys) * PTR_WIDTH;
			
			if (fb->ptr_mask[i])
				fb_putpix_p_32(dd, x, y, fb->ptr_back[i]);
			i++;
		}
}

/* ---- 32-bit display support functions ----------------------------------- */

#ifdef __ARCH_I386__
static void cpy32fw(volatile uint32_t *dst, volatile uint32_t *src, unsigned len);
asm(
".text;				"
"cpy32fw:			"
"	cld;			"
"	push	%edi;		"
"	push	%esi;		"
"	push	%ecx;		"
"	mov	16(%esp), %edi;	"
"	mov	20(%esp), %esi;	"
"	mov	24(%esp), %ecx;	"
"	rep;			"
"	movsl;			"
"	pop	%ecx;		"
"	pop	%esi;		"
"	pop	%edi;		"
"	ret;			"
);

static void cpy32bk(volatile uint32_t *dst, volatile uint32_t *src, unsigned len);
asm(
".text;				"
"cpy32bk:			"
"	std;			"
"	push	%edi;		"
"	push	%esi;		"
"	push	%ecx;		"
"	mov	16(%esp), %edi;	"
"	mov	20(%esp), %esi;	"
"	mov	24(%esp), %ecx;	"
"	add	%ecx, %edi;	"
"	add	%ecx, %edi;	"
"	add	%ecx, %edi;	"
"	add	%ecx, %edi;	"
"	add	%ecx, %esi;	"
"	add	%ecx, %esi;	"
"	add	%ecx, %esi;	"
"	add	%ecx, %esi;	"
"	sub	$4, %edi;	"
"	sub	$4, %esi;	"
"	rep;			"
"	movsl;			"
"	pop	%ecx;		"
"	pop	%esi;		"
"	pop	%edi;		"
"	cld;			"
"	ret;			"
);

static void fill32(volatile uint32_t *p, unsigned c, unsigned len);
asm(
".text;				"
"fill32:			"
"	cld;			"
"	push	%edi;		"
"	push	%esi;		"
"	push	%ecx;		"
"	mov	16(%esp), %edi;	"
"	mov	20(%esp), %eax;	"
"	mov	24(%esp), %ecx;	"
"	rep;			"
"	stosl;			"
"	pop	%ecx;		"
"	pop	%esi;		"
"	pop	%edi;		"
"	ret;			"
);
#else
static void cpy32fw(volatile uint32_t *dst, volatile uint32_t *src, unsigned len)
{
	unsigned i;
	
	for (i = 0; i < len; i++)
		dst[i] = src[i];
}

static void cpy32bk(volatile uint32_t *dst, volatile uint32_t *src, unsigned len)
{
	int i;
	
	for (i = len - 1; i >= 0; i--)
		dst[i] = src[i];
}

static void fill32(volatile uint32_t *p, unsigned c, unsigned len)
{
	unsigned i;
	
	for (i = 0; i < len; i++)
		p[i] = c;
}
#endif

static void fb_putpix_p_32(void *dd, int x, int y, win_color c)
{
	struct framebuf *fb = dd;
	uint32_t *fbuf_32 = fb->fbuf;
	
	if (x < 0 || x >= fb->width)
		return;
	if (y < 0 || y >= fb->height)
		return;
	
	fbuf_32[x + y * fb->width] = c;
}

static void fb_getpix_p_32(void *dd, int x, int y, win_color *c)
{
	struct framebuf *fb = dd;
	uint32_t *fbuf_32 = fb->fbuf;
	
	if (x < 0 || x >= fb->width)
		goto badpos;
	if (y < 0 || y >= fb->height)
		goto badpos;
	
	*c = fbuf_32[x + y * fb->width];
	return;
badpos:
	*c = 0;
}

void fb_putpix_32(void *dd, int x, int y, win_color c)
{
	struct framebuf *fb = dd;
	uint32_t *fbuf_32 = fb->fbuf;
	int i;
	
	if (!fb->ptr_hide_count &&
	    x >= fb->ptr_x && x < fb->ptr_x + PTR_WIDTH &&
	    y >= fb->ptr_y && y < fb->ptr_y + PTR_WIDTH)
	{
		i = (x - fb->ptr_x) + (y - fb->ptr_y) * PTR_WIDTH;
		
		if (fb->ptr_mask[i])
		{
			fb->ptr_back[i] = c;
			return;
		}
	}
	
	fbuf_32[x + y * fb->width] = c;
}

void fb_getpix_32(void *dd, int x, int y, win_color *c)
{
	struct framebuf *fb = dd;
	uint32_t *fbuf_32 = fb->fbuf;
	static int i;
	
	if (!fb->ptr_hide_count &&
	    x >= fb->ptr_x && x < fb->ptr_x + PTR_WIDTH &&
	    y >= fb->ptr_y && y < fb->ptr_y + PTR_WIDTH)
	{
		i = (x - fb->ptr_x) + (y - fb->ptr_y) * PTR_WIDTH;
		
		if (fb->ptr_mask[i])
		{
			*c = fb->ptr_back[i];
			return;
		}
	}
	
	*c = fbuf_32[x + y * fb->width];
}

void fb_hline_32(void *dd, int x, int y, int len, win_color c)
{
	struct framebuf *fb = dd;
	uint32_t *fbuf_32 = fb->fbuf;
	static int i;
	
	if (y >= fb->ptr_y && y < fb->ptr_y + PTR_HEIGHT)
		for (i = x; i < x + len; i++)
			fb_putpix_32(dd, i, y, c);
	else
		fill32(&fbuf_32[x + y * fb->width], c, len);
}

void fb_vline_32(void *dd, int x, int y, int len, win_color c)
{
	struct framebuf *fb = dd;
	static int i;
	
	for (i = y; i < y + len; i++)
		fb_putpix_32(dd, x, i, c);
}

void fb_rect_32(void *dd, int x0, int y0, int w, int h, win_color c)
{
	struct framebuf *fb = dd;
	static int y1;
	static int y;
	
	y1 = y0 + h;
	for (y = y0; y < y1; y++)
		fb_hline_32(dd, x0, y, w, c);
}

static void fb_copy_rev_32(void *dd, int x0, int y0, int x1, int y1, int w, int h)
{
	struct framebuf *fb = dd;
	uint32_t *fbuf_32 = fb->fbuf;
	static int y;
	
	for (y = h - 1; y >= 0; y--)
		cpy32bk(&fbuf_32[x0 + (y + y0) * fb->width],
			&fbuf_32[x1 + (y + y1) * fb->width],
			w);
}

void fb_copy_32(void *dd, int x0, int y0, int x1, int y1, int w, int h)
{
	struct framebuf *fb = dd;
	uint32_t *fbuf_32 = fb->fbuf;
	static int y;
	
	fb_hideptr_32(dd);
	if (y0 > y1 || (y0 == y1 && x0 > x1))
		fb_copy_rev_32(dd, x0, y0, x1, y1, w, h);
	else
	{
		for (y = 0; y < h; y++)
			cpy32fw(&fbuf_32[x0 + (y + y0) * fb->width],
				&fbuf_32[x1 + (y + y1) * fb->width],
				w);
	}
	fb_showptr_32(dd);
}

/* ---- 8-bit display support functions ------------------------------------ */

static void fb_putpix_p_8(void *dd, int x, int y, win_color c)
{
	struct framebuf *fb = dd;
	uint8_t *fbuf_8 = fb->fbuf;
	
	if (x < 0 || x >= fb->width)
		return;
	if (y < 0 || y >= fb->height)
		return;
	
	fbuf_8[x + y * fb->width] = c;
}

static void fb_getpix_p_8(void *dd, int x, int y, win_color *c)
{
	struct framebuf *fb = dd;
	uint8_t *fbuf_8 = fb->fbuf;
	
	if (x < 0 || x >= fb->width)
		goto badpos;
	if (y < 0 || y >= fb->height)
		goto badpos;
	
	*c = fbuf_8[x + y * fb->width];
	return;
badpos:
	*c = 0;
}

void fb_putpix_8(void *dd, int x, int y, win_color c)
{
	struct framebuf *fb = dd;
	uint8_t *fbuf_8 = fb->fbuf;
	static int i;
	
	if (x >= fb->ptr_x && x < fb->ptr_x + PTR_WIDTH &&
	    y >= fb->ptr_y && y < fb->ptr_y + PTR_WIDTH)
	{
		i = (x - fb->ptr_x) + (y - fb->ptr_y) * PTR_WIDTH;
		
		if (fb->ptr_mask[i])
		{
			fb->ptr_back[i] = c;
			return;
		}
	}
	
	fbuf_8[x + y * fb->width] = c;
}

void fb_getpix_8(void *dd, int x, int y, win_color *c)
{
	struct framebuf *fb = dd;
	uint8_t *fbuf_8 = fb->fbuf;
	static int i;
	
	if (x >= fb->ptr_x && x < fb->ptr_x + PTR_WIDTH &&
	    y >= fb->ptr_y && y < fb->ptr_y + PTR_WIDTH)
	{
		i = (x - fb->ptr_x) + (y - fb->ptr_y) * PTR_WIDTH;
		
		if (fb->ptr_mask[i])
		{
			*c = fb->ptr_back[i];
			return;
		}
	}
	
	*c = fbuf_8[x + y * fb->width];
}

void fb_hline_8(void *dd, int x, int y, int len, win_color c)
{
	struct framebuf *fb = dd;
	uint8_t *fbuf_8 = fb->fbuf;
	int i;
	
	if (y >= fb->ptr_y && y < fb->ptr_y + PTR_HEIGHT)
		for (i = x; i < x + len; i++)
			fb_putpix_8(dd, i, y, c);
	else
		memset((void *)&fbuf_8[x + y * fb->width], c, len);
}

void fb_vline_8(void *dd, int x, int y, int len, win_color c)
{
	struct framebuf *fb = dd;
	int i;
	
	for (i = y; i < y + len; i++)
		fb_putpix_8(dd, x, i, c);
}

void fb_rect_8(void *dd, int x0, int y0, int w, int h, win_color c)
{
	struct framebuf *fb = dd;
	int y1 = y0 + h;
	int y;
	
	for (y = y0; y < y1; y++)
		fb_hline_8(dd, x0, y, w, c);
}

static void fb_copy_rev_8(void *dd, int x0, int y0, int x1, int y1,
			  int w, int h)
{
	struct framebuf *fb = dd;
	uint8_t *fbuf_8 = fb->fbuf;
	int y;
	
	for (y = h - 1; y >= 0; y--)
		memmove((void *)&fbuf_8[x0 + (y + y0) * fb->width],
			(void *)&fbuf_8[x1 + (y + y1) * fb->width],
			w);
}

void fb_copy_8(void *dd, int x0, int y0, int x1, int y1, int w, int h)
{
	struct framebuf *fb = dd;
	uint8_t *fbuf_8 = fb->fbuf;
	int y;
	
	fb_hideptr_8(dd);
	if (y0 > y1 || (y0 == y1 && x0 > x1))
		fb_copy_rev_8(dd, x0, y0, x1, y1, w, h);
	else
	{
		for (y = 0; y < h; y++)
			memmove((void *)&fbuf_8[x0 + (y + y0) * fb->width],
				(void *)&fbuf_8[x1 + (y + y1) * fb->width],
				w);
	}
	fb_showptr_8(dd);
}

void fb_setptr_8(void *dd, const win_color *shape, const unsigned *mask)
{
	struct framebuf *fb = dd;
	
	fb_hideptr_8(dd);
	memcpy(fb->ptr_shape, shape, sizeof fb->ptr_shape);
	memcpy(fb->ptr_mask,  mask,  sizeof fb->ptr_mask);
	fb_showptr_8(dd);
}

void fb_moveptr_8(void *dd, int x, int y)
{
	win_color oback[PTR_WIDTH * PTR_HEIGHT];
	struct framebuf *fb = dd;
	int x0, y0, x1, y1, x2, y2;
	int px, py;
	int i;
	
	if (fb->ptr_hide_count)
	{
		fb->ptr_x = x;
		fb->ptr_y = y;
	}
	
	memcpy(oback, fb->ptr_back, sizeof oback);
	x0 = fb->ptr_x;
	y0 = fb->ptr_y;
	
	fb->ptr_hide_count = 1;
	fb->ptr_x = x;
	fb->ptr_y = y;
	fb_showptr_8(dd);
	
	x1 = x0;
	y1 = y0;
	
	x2 = x0 + PTR_WIDTH;
	y2 = y0 + PTR_WIDTH;
	
	if (x1 < 0)		x1 = 0;
	if (y1 < 0)		y1 = 0;
	
	if (x2 > fb->width)	x2 = fb->width;
	if (y2 > fb->height)	y2 = fb->height;
	
	for (y = y1; y < y2; y++)
		for (x = x1; x < x2; x++)
		{
			i = (x - x0) + (y - y0) * PTR_WIDTH;
			
			if (fb->ptr_mask[i])
				fb_putpix_8(dd, x, y, oback[i]);
		}
}

void fb_showptr_8(void *dd)
{
	struct framebuf *fb = dd;
	int x, y;
	int i;
	
	if (!fb->ptr_hide_count)
		return;
	
	fb->ptr_hide_count--;
	if (fb->ptr_hide_count)
		return;
	
	if (!fb->ptr_hide_count)
	{
		i = 0;
		for (y = 0; y < PTR_HEIGHT; y++)
			for (x = 0; x < PTR_WIDTH; x++)
			{
				fb_getpix_p_8(dd, x + fb->ptr_x, y + fb->ptr_y, &fb->ptr_back[i]);
				if (fb->ptr_mask[i])
					fb_putpix_p_8(dd, x + fb->ptr_x, y + fb->ptr_y, fb->ptr_shape[i]);
				i++;
			}
	}
}

void fb_hideptr_8(void *dd)
{
	struct framebuf *fb = dd;
	int x, y;
	int i;
	
	fb->ptr_hide_count++;
	if (fb->ptr_hide_count > 1)
		return;
	
	i = 0;
	for (y = 0; y < PTR_HEIGHT; y++)
		for (x = 0; x < PTR_WIDTH; x++)
		{
			if (fb->ptr_mask[i])
				fb_putpix_p_8(dd, x + fb->ptr_x, y + fb->ptr_y, fb->ptr_back[i]);
			i++;
		}
}

int mod_onload(unsigned md, const char *pathname, void *data, unsigned sz)
{
	return 0;
}

int mod_onunload(unsigned md)
{
	return 0;
}
