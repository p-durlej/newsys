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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <vt100.h>

#define VST_CHARACTER	0
#define VST_ESC		1
#define VST_CSI		2
// #define VST_MODE	3

struct vt100 *vt100_creat(int w, int h)
{
	struct vt100 *vt;
	
	vt = malloc(sizeof *vt);
	if (!vt)
		return NULL;
	vt->line_dirty  = malloc(sizeof *vt->line_dirty * h);
	vt->cells	= malloc(sizeof *vt->cells * w * h);
	if (!vt->cells || !vt->line_dirty)
	{
		free(vt->line_dirty);
		free(vt->cells);
		free(vt);
		return NULL;
	}
	vt->w = w;
	vt->h = h;
	vt100_reset(vt);
	return vt;
}

void vt100_free(struct vt100 *vt)
{
	free(vt->line_dirty);
	free(vt->cells);
	free(vt);
}

void vt100_reset(struct vt100 *vt)
{
	struct vt100_cell *cl;
	int i;
	
	cl = vt->cells;
	for (i = 0; i < vt->w * vt->h; i++, cl++)
	{
		cl->attr = 0;
		cl->fg	 = 7;
		cl->bg	 = 0;
		cl->ch	 = ' ';
	}
	for (i = 0; i < vt->h; i++)
		vt->line_dirty[i] = 1;
	
	vt->scroll_bottom = vt->h;
	vt->scroll_top	  = 0;
	vt->st		  = VST_CHARACTER;
	vt->attr	  = 0;
	vt->fg		  = 7;
	vt->bg		  = 0;
	vt->x		  = 0;
	vt->y		  = 0;
}

static void vt100_scroll(struct vt100 *vt)
{
	struct vt100_cell *cl;
	int i;
	
	memmove(vt->cells + vt->scroll_top * vt->w,
		vt->cells + (vt->scroll_top + 1) * vt->w,
		(vt->scroll_bottom - vt->scroll_top - 1) * vt->w *
		sizeof *cl);
	
	cl = vt->cells + vt->w * (vt->scroll_bottom - 1);
	for (i = 0; i < vt->w; i++, cl++)
	{
		cl->attr = vt->attr;
		cl->bg	 = vt->bg;
		cl->fg	 = vt->fg;
		cl->ch	 = ' ';
	}
	for (i = 0; i < vt->h; i++)
		vt->line_dirty[i] = 1;
	vt->y--;
}

static void vt100_scroll_down(struct vt100 *vt, int y0, int y1, int cnt)
{
	struct vt100_cell *end;
	struct vt100_cell *p;
	int i;
	
	memmove(vt->cells + (y0 + cnt) * vt->w,
		vt->cells +  y0 * vt->w,
		(y1 - y0 - cnt) * vt->w * sizeof(struct vt100_cell));
	
	end = vt->cells +  y0 * vt->w;
	p   = vt->cells + (y0 + cnt) * vt->w;
	while (p < end)
	{
		p->attr = vt->attr;
		p->bg	= vt->bg;
		p->fg	= vt->fg;
		p->ch	= ' ';
		p++;
	}
	
	for (i = y0; i < y1; i++)
		vt->line_dirty[i] = 1;
}

static void vt100_do_putc(struct vt100 *vt, char ch)
{
	struct vt100_cell *cl;
	
	switch (ch)
	{
	case '\a':
		break;
	case '\r':
		vt->x = 0;
		break;
	case '\n':
		/* vt->x = 0; */
		vt->line_dirty[vt->y] = 1;
		vt->y++;
		while (vt->y >= vt->scroll_bottom /* vt->h */)
			vt100_scroll(vt);
		vt->line_dirty[vt->y] = 1;
		break;
	case '\b':
		if (vt->x)
			vt->x--;
		vt->line_dirty[vt->y] = 1;
		break;
	case '\t':
		vt->x &= ~7;
		vt->x += 8;
		vt->line_dirty[vt->y] = 1;
		break;
	default:
		if (ch == 0)
			ch = ' ';
		if ((unsigned char)ch < 32)
			break;
		
		if (vt->x >= vt->w)
		{
			vt->x = 0;
			vt->y++;
		}
		while (vt->y >= vt->scroll_bottom /* vt->h */)
			vt100_scroll(vt);
		
		cl = &vt->cells[vt->x + vt->y * vt->w];
		vt->line_dirty[vt->y] = 1;
		cl->attr = vt->attr;
		cl->fg	 = vt->fg;
		cl->bg	 = vt->bg;
		cl->ch	 = ch;
		vt->x++;
	}
}

static void do_mode_cmd(struct vt100 *vt)
{
	int i;
	int a;
	
	for (i = 0; i < vt->argc; i++)
	{
		a = vt->args[i];
		
		if (a >= 30 && a <= 37)
			vt->fg = a - 30;
		else if (a >= 40 && a <= 47)
			vt->bg = a - 40;
		else
			switch (a)
			{
			case 0:
				vt->attr = 0;
				vt->bg	 = 0;
				vt->fg	 = 7;
				break;
			case 1:
				vt->attr |= VT100A_BOLD;
				break;
			case 4:
				vt->attr |= VT100A_UNDERLINE;
				break;
			case 5:
				vt->attr |= VT100A_BLINK;
				break;
			case 7:
				vt->attr |= VT100A_INVERT;
				break;
			case 22:
				vt->attr &= ~VT100A_BOLD;
				break;
			case 24:
				vt->attr &= ~VT100A_UNDERLINE;
				break;
			case 25:
				vt->attr &= ~VT100A_BLINK;
				break;
			case 27:
				vt->attr &= ~VT100A_INVERT;
				break;
			case 39:
				vt->fg = 7;
				break;
			case 49:
				vt->bg = 0;
				break;
			}
	}
	vt->st = VST_CHARACTER;
}

static void do_home_cmd(struct vt100 *vt)
{
	int x = 0;
	int y = 0;
	
	if (vt->argc > 0)
	{
		y = vt->args[0] - 1;
		if (y < 0)
			y = 0;
		if (y >= vt->h)
			y = vt->h - 1;
	}
	if (vt->argc > 1)
	{
		x = vt->args[1] - 1;
		if (x < 0)
			x = 0;
		if (x >= vt->w)
			x = vt->w - 1;
	}
	vt->line_dirty[vt->y] = 1;
	vt->st = VST_CHARACTER;
	vt->x = x;
	vt->y = y;
	vt->line_dirty[vt->y] = 1;
}

static void do_clr_cmd(struct vt100 *vt)
{
	struct vt100_cell *end;
	struct vt100_cell *cl;
	int i;
	
	end = vt->cells + vt->w * vt->h;
	cl  = vt->cells + vt->x + vt->y * vt->w;
	while (cl < end)
	{
		cl->attr = vt->attr;
		cl->bg = vt->bg;
		cl->fg = vt->fg;
		cl->ch = ' ';
		cl++;
	}
	for (i = vt->y; i < vt->h; i++)
		vt->line_dirty[i] = 1;
}

static void do_region_cmd(struct vt100 *vt)
{
	int b = vt->h;
	int t = 0;
	
	if (vt->argc > 0)
	{
		t = vt->args[0] - 1;
		if (t >= vt->h - 1)
			t = vt->h - 2;
	}
	if (vt->argc > 1)
	{
		b = vt->args[1] /* - 1 */;
		if (b > vt->h)
			b = vt->h;
		if (b <= t)
			b = t + 1;
	}
	vt->scroll_bottom = b;
	vt->scroll_top	  = t;
}

static void do_erase_line_cmd(struct vt100 *vt)
{
	int mode = 0;
	int i;
	
	switch (mode)
	{
	case 0:
		for (i = vt->x; i < vt->w; i++)
		{
			vt->cells[vt->y * vt->w + i].ch = ' ';
			/* vt->cells[vt->y * vt->w + i].attr = vt->attr;
			vt->cells[vt->y * vt->w + i].bg = vt->bg;
			vt->cells[vt->y * vt->w + i].fg = vt->fg; */
		}
		break;
	case 1:
		for (i = 0; i < vt->x; i++)
		{
			vt->cells[vt->y * vt->w + i].ch = ' ';
			/* vt->cells[vt->y * vt->w + i].attr = vt->attr;
			vt->cells[vt->y * vt->w + i].bg = vt->bg;
			vt->cells[vt->y * vt->w + i].fg = vt->fg; */
		}
		break;
	case 2:
		for (i = 0; i < vt->w; i++)
		{
			vt->cells[vt->y * vt->w + i].ch = ' ';
			/* vt->cells[vt->y * vt->w + i].attr = vt->attr;
			vt->cells[vt->y * vt->w + i].bg = vt->bg;
			vt->cells[vt->y * vt->w + i].fg = vt->fg; */
		}
		break;
	default:
		;
	}
	vt->line_dirty[vt->y] = 1;
}

static void do_move_up_cmd(struct vt100 *vt)
{
	int n = 1;
	
	if (vt->argc > 0)
		n = vt->args[0];
	if (n < 1)
		return;
	
	vt->line_dirty[vt->y] = 1;
	if (n > vt->y)
		vt->y = 0;
	else
		vt->y -= n;
	vt->line_dirty[vt->y] = 1;
}

static void do_move_down_cmd(struct vt100 *vt)
{
	int n = 1;
	
	if (vt->argc > 0)
		n = vt->args[0];
	if (n < 1)
		return;
	
	vt->line_dirty[vt->y] = 1;
	if (vt->y + n >= vt->h)
		vt->y = vt->h - 1;
	else
		vt->y += n;
	vt->line_dirty[vt->y] = 1;
}

static void do_move_right_cmd(struct vt100 *vt)
{
	int n = 1;
	
	if (vt->argc > 0)
		n = vt->args[0];
	
	vt->x += n;
	if (vt->x >= vt->w)
		vt->x = vt->w - 1;
	vt->line_dirty[vt->y] = 1;	
}

static void do_move_left_cmd(struct vt100 *vt)
{
	int n = 1;
	
	if (vt->argc > 0)
		n = vt->args[0];
	
	vt->x -= n;
	if (vt->x < 0)
		vt->x = 0;
	vt->line_dirty[vt->y] = 1;	
}

static void do_nop_cmd(struct vt100 *vt)
{
	/* nop */
}

static void do_set_cmd(struct vt100 *vt)
{
}

static void do_reset_cmd(struct vt100 *vt)
{
}

static void do_insert_cmd(struct vt100 *vt)
{
	struct vt100_cell *end;
	struct vt100_cell *p;
	int cnt = 1;
	int i;
	
	if (vt->y < vt->scroll_top)
		return;
	if (vt->y >= vt->scroll_bottom)
		return;
	
	if (vt->argc > 0)
		cnt = vt->args[0];
	
	if (vt->y + cnt > vt->scroll_bottom)
	{
		end = vt->cells + vt->scroll_bottom * vt->w;
		p   = vt->cells + vt->y * vt->w;
		while (p < end)
		{
			p->attr = vt->attr;
			p->bg	= vt->bg;
			p->fg	= vt->fg;
			p->ch	= ' ';
			p++;
		}
	}
	else
	{
		memmove(vt->cells + (vt->y + cnt) * vt->w,
			vt->cells +  vt->y * vt->w,
			(vt->scroll_bottom - vt->y - cnt) * vt->w *
			sizeof(struct vt100_cell));
		
		end = vt->cells + (vt->y + cnt) * vt->w;
		p   = vt->cells + vt->y * vt->w;
		while (p < end)
		{
			p->attr = vt->attr;
			p->bg	= vt->bg;
			p->fg	= vt->fg;
			p->ch	= ' ';
			p++;
		}
	}
	for (i = vt->y; i < vt->scroll_bottom; i++)
		vt->line_dirty[i] = 1;
	vt->x = 0;
}

static void do_remove_cmd(struct vt100 *vt)
{
	struct vt100_cell *end;
	struct vt100_cell *p;
	int cnt = 1;
	int i;
	
	if (vt->y < vt->scroll_top)
		return;
	if (vt->y >= vt->scroll_bottom)
		return;
	
	if (vt->argc > 0)
		cnt = vt->args[0];
	
	if (vt->y + cnt > vt->scroll_bottom)
	{
		end = vt->cells + vt->scroll_bottom * vt->w;
		p   = vt->cells + vt->y * vt->w;
		while (p < end)
		{
			p->attr = vt->attr;
			p->bg	= vt->bg;
			p->fg	= vt->fg;
			p->ch	= ' ';
			p++;
		}
	}
	else
	{
		memmove(vt->cells +  vt->y * vt->w,
			vt->cells + (vt->y + cnt) * vt->w,
			(vt->scroll_bottom - vt->y - cnt) * vt->w *
			sizeof(struct vt100_cell));
		
		/* end = vt->cells +  vt->scroll_bottom * vt->w;
		p   = vt->cells + (vt->scroll_bottom - cnt) * vt->w;
		while (p < end)
		{
			p->attr = vt->attr;
			p->bg	= vt->bg;
			p->fg	= vt->fg;
			p->ch	= ' ';
			p++;
		} */
	}
	for (i = vt->y; i < vt->scroll_bottom; i++)
		vt->line_dirty[i] = 1;
	vt->x = 0;
}

static void do_insert_chr_cmd(struct vt100 *vt)
{
	int cnt = 1;
	int i;
	
	if (vt->argc > 0)
		cnt = vt->args[0];
	
	if (!cnt)
		cnt = 1;
	if (vt->x + cnt > vt->w)
		cnt = vt->w - vt->x;
	
	memmove(vt->cells + vt->y * vt->w + vt->x + cnt,
		vt->cells + vt->y * vt->w + vt->x,
		(vt->w - vt->x - cnt) * sizeof(struct vt100_cell));
	for (i = vt->x; i < vt->x + cnt; i++)
	{
		vt->cells[vt->y * vt->w + i].attr = 0;
		vt->cells[vt->y * vt->w + i].bg	  = 0;
		vt->cells[vt->y * vt->w + i].fg	  = 0;
		vt->cells[vt->y * vt->w + i].ch	  = ' ';
	}
}

static void do_delete_chr_cmd(struct vt100 *vt)
{
	int cnt = 1;
	int i;
	
	if (vt->argc > 0)
		cnt = vt->args[0];
	
	if (vt->x + cnt > vt->w)
		cnt = vt->w - vt->x;
	
	memmove(vt->cells + vt->y * vt->w + vt->x,
		vt->cells + vt->y * vt->w + vt->x + cnt,
		(vt->w - vt->x - cnt) * sizeof(struct vt100_cell));
	for (i = vt->w - cnt; i < vt->w; i++)
	{
		vt->cells[vt->y * vt->w + i].attr = 0;
		vt->cells[vt->y * vt->w + i].bg	  = 0;
		vt->cells[vt->y * vt->w + i].fg	  = 0;
		vt->cells[vt->y * vt->w + i].ch	  = ' ';
	}
}

static void do_test_cmd(struct vt100 *vt)
{
	int i;
	
	for (i = 0; i < vt->argc; i++)
		printf("[%i] = %i\n", i, vt->args[i]);
	sleep(5);
}

static struct CSI_cmd
{
	void (*proc)(struct vt100 *vt);
	char code;
} CSI_cmd[] =
{
	{ do_mode_cmd,		'm' },
	{ do_home_cmd,		'H' },
	{ do_clr_cmd,		'J' },
	{ do_region_cmd,	'r' },
	{ do_erase_line_cmd,	'K' },
	{ do_move_up_cmd,	'A' },
	{ do_move_down_cmd,	'B' },
	{ do_move_right_cmd,	'C' },
	{ do_move_left_cmd,	'D' },
	{ do_set_cmd,		'h' },
	{ do_reset_cmd,		'l' },
	{ do_insert_cmd,	'L' },
	{ do_remove_cmd,	'M' },
	{ do_insert_chr_cmd,	'@' },
	{ do_delete_chr_cmd,	'P' }
};

static void do_esm_M(struct vt100 *vt)
{
	if (vt->y < vt->scroll_top)
		return;
	
	if (vt->y > vt->scroll_top)
	{
		vt->line_dirty[vt->y] = 1;
		vt->y--;
		vt->line_dirty[vt->y] = 1;
	}
	else
		vt100_scroll_down(vt, vt->scroll_top, vt->scroll_bottom, 1);
}

static struct
{
	void (*proc)(struct vt100 *vt);
	char code;
} esm_cmd[] =
{
	{ do_esm_M,		'M' },
};

#define CSI_CNT		(sizeof(CSI_cmd) / sizeof(*CSI_cmd))
#define ESM_CNT		(sizeof(esm_cmd) / sizeof(*esm_cmd))

void vt100_putc(struct vt100 *vt, char ch)
{
	int i;
	
	switch (vt->st)
	{
	case VST_CHARACTER:
		switch (ch)
		{
		case '\e':
			vt->st = VST_ESC;
			break;
		case (char)('[' + 0x40):
			vt->st = VST_CSI;
			vt->args[0] = 0;
			vt->argc = 0;
			break;
		case (char)('M' + 0x40):
			do_esm_M(vt);
			break;
		default:
			vt100_do_putc(vt, ch);
		}
		break;
	case VST_ESC:
		switch (ch)
		{
		case 'D':
		case 'E':
		case 'H':
		case 'M':
		case 'N':
		case 'O':
		case 'P':
		case '[':
		case '\\':
			vt->st = VST_CHARACTER;
			vt100_putc(vt, ch + 0x40);
			break;
		default:
			for (i = 0; i < ESM_CNT; i++)
				if (ch == esm_cmd[i].code)
				{
					/* if (vt->argc < sizeof(vt->args) / sizeof(*vt->args) && vt->args[vt->argc])
						vt->argc++; */
					esm_cmd[i].proc(vt);
					break;
				}
				/* if (i == ESM_CNT)
				{
					printf("unknown ESM command: '%c'\n", ch);
					exit(1);
				} */
				vt->st = VST_CHARACTER;
			}
			break;
		case VST_CSI:
			if (ch >= '0' && ch <= '9')
			{
				if (vt->argc <= sizeof vt->args / sizeof *vt->args && vt->args[vt->argc - 1] < 1000)
				{
					if (!vt->argc)
						vt->argc = 1;
					vt->args[vt->argc - 1] *= 10;
					vt->args[vt->argc - 1] += ch - '0';
				}
			}
			else if (ch == ';')
			{
				if (vt->argc <= sizeof vt->args / sizeof *vt->args)
				{
					vt->argc++;
					vt->args[vt->argc - 1] = 0;
				}
			}
			else
			{
				for (i = 0; i < CSI_CNT; i++)
					if (ch == CSI_cmd[i].code)
					{
						CSI_cmd[i].proc(vt);
						break;
					}
				/* if (i == CSI_CNT)
				{
					printf("unknown CSI command: '%c'\n", ch);
					exit(1);
				} */
				vt->st = VST_CHARACTER;
			}
			break;
		default:
			abort();
	}
}

void vt100_puts(struct vt100 *vt, const char *s)
{
	while (*s)
		vt100_putc(vt, *s++);
}
