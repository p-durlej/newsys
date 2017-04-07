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

#ifndef _DEV_FRAMEBUF_H
#define _DEV_FRAMEBUF_H

struct framebuf
{
	struct win_display *	disp;
	void *			fbuf;
	int			width, height;
	int			vwidth;
	
	unsigned		ptr_mask[PTR_WIDTH * PTR_HEIGHT];
	win_color		ptr_shape[PTR_WIDTH * PTR_HEIGHT];
	win_color		ptr_back[PTR_WIDTH * PTR_HEIGHT];
	int			ptr_x, ptr_y;
	int			ptr_hide_count;
};

struct framebuf *fb_creat5(struct win_display *disp, void *buf, int w, int h, int vw);
struct framebuf *fb_creat(struct win_display *disp, void *buf, int w, int h);
void fb_reset(struct framebuf *fb);
void fb_free(struct framebuf *fb);

void fb_setptr_32(void *dd, const win_color *shape, const unsigned *mask);
void fb_moveptr_32(void *dd, int x, int y);
void fb_showptr_32(void *dd);
void fb_hideptr_32(void *dd);

void fb_putpix_32(void *dd, int x, int y, win_color c);
void fb_getpix_32(void *dd, int x, int y, win_color *c);
void fb_hline_32(void *dd, int x, int y, int len, win_color c);
void fb_vline_32(void *dd, int x, int y, int len, win_color c);
void fb_rect_32(void *dd, int x, int y, int w, int h, win_color c);
void fb_copy_32(void *dd, int x0, int y0, int x1, int y1, int w, int h);

void fb_bmp_hline_32(void *dd, int x, int y, int w, const uint8_t *data, int off, win_color bg, win_color fg);

void fb_putpix_8(void *dd, int x, int y, win_color c);
void fb_getpix_8(void *dd, int x, int y, win_color *c);
void fb_hline_8(void *dd, int x, int y, int len, win_color c);
void fb_vline_8(void *dd, int x, int y, int len, win_color c);
void fb_rect_8(void *dd, int x, int y, int w, int h, win_color c);
void fb_copy_8(void *dd, int x0, int y0, int x1, int y1, int w, int h);

void fb_setptr_8(void *dd, const win_color *shape, const unsigned *mask);
void fb_moveptr_8(void *dd, int x, int y);
void fb_showptr_8(void *dd);
void fb_hideptr_8(void *dd);

void fb_bmp_hline_8(void *dd, int x, int y, int w, const uint8_t *data, int off, win_color bg, win_color fg);

#endif
