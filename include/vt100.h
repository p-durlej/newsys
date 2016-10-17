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

#ifndef VT100_H
#define VT100_H

#define VT100A_BOLD		1
#define VT100A_INVERT		2
#define VT100A_UNDERLINE	4
#define VT100A_BLINK		8

struct vt100_cell
{
	char bg, fg;
	char ch;
	int  attr;
};

struct vt100
{
	struct vt100_cell *	cells;
	int *			line_dirty;
	int			w, h;
	int			x, y;
	char			bg, fg;
	int			attr;
	
	int			scroll_bottom;
	int			scroll_top;
	
	int			args[8];
	int			argc;
	int			st;
};

struct vt100 *vt100_creat(int w, int h);
void vt100_free(struct vt100 *vt);

void vt100_reset(struct vt100 *vt);

void vt100_putc(struct vt100 *vt, char ch);
void vt100_puts(struct vt100 *vt, const char *s);

#endif
