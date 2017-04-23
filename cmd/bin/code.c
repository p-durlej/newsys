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

#include <priv/copyright.h>
#include <wingui_cgadget.h>
#include <wingui_msgbox.h>
#include <wingui_color.h>
#include <wingui_menu.h>
#include <wingui_form.h>
#include <wingui_bell.h>
#include <wingui_dlg.h>
#include <wingui_buf.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <mkcanon.h>
#include <newtask.h>
#include <confdb.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <paths.h>
#include <err.h>

#define DEFAULT_FONT	WIN_FONT_MONO_N
#define COMPILER	"/usr/bin/tcc"

#define min(a, b)	((a) < (b) ? (a) : (b))
#define max(a, b)	((a) > (b) ? (a) : (b))

typedef char echar;

static struct
{
	int run_mode;
	int font;
} config;

struct buffer
{
	char pathname[PATH_MAX];
	struct cell *text;
	int length;
	int dirty;
	int nsel_start;
	int sel_start;
	int sel_end;
	int cur_index;
	int top_y;
	int scroll_x;
};

struct cell
{
	echar ch;
};

static struct form *main_form;
static struct menu *main_menu;
static struct gadget *editor;

static struct buffer buffers[12];
static struct buffer errbuf;
static struct buffer *prev_buf;
static struct buffer *cur_buf = &buffers[0];

static void editor_redraw_line(struct gadget *g, int i, int clip);
static void editor_redraw(struct gadget *g, int wd);
static void editor_inschr(struct gadget *g, unsigned ch);
static void editor_delchr(struct gadget *g);
static void editor_adjpos(struct gadget *g);
static void editor_key_down(struct gadget *g, unsigned ch, unsigned shift);
static struct gadget *editor_creat(struct form *f, int x, int y, int w, int h);

static void run_click(struct menu_item *mi);

static void help(void)
{
	const char msg[] =
		"Additional keyboard shortcuts:\n\n"
		" F1: This help\n"
		" F2: Save\n"
		" F3: Mark\n"
		" F5: Copy\n"
		" F6: Move\n"
		" F7: Delete\n\n"
		" ESC: Close the compiler output view";
	
	msgbox(main_form, "Help", msg);
}

static int new_buffer(struct buffer *buf)
{
	memset(buf, 0, sizeof *buf);
	buf->nsel_start = -1;
	return 0;
}

static void free_buffer(struct buffer *buf)
{
	free(buf->text);
	
	memset(buf, 0, sizeof *buf);
	buf->nsel_start = -1;
}

static void clear_buffer(struct buffer *buf)
{
	free(buf->text);
	
	memset(buf, 0, sizeof *buf);
	buf->nsel_start = -1;
}

static int save_buffer(struct buffer *buf, char *pathname)
{
	struct cell *cl;
	FILE *f;
	int i;
	
	f = fopen(pathname, "w");
	if (!f)
		return -1;
	
	cl = buf->text;
	for (i = 0; i < buf->length; i++, cl++)
		if (fputc(cl->ch, f) == EOF)
		{
			fclose(f);
			return -1;
		}
	
	if (fclose(f))
		return -1;
	return 0;
}

static int fsave_buffer_sel(struct buffer *buf, FILE *f)
{
	struct cell *cl;
	int i;
	
	cl = buf->text + buf->sel_start;
	for (i = buf->sel_start; i < buf->sel_end; i++, cl++)
		if (fputc(cl->ch, f) == EOF)
		{
			fclose(f);
			return -1;
		}
	return 0;
}

static int expand_tabs(struct buffer *buf)
{
	struct cell *ncl;
	struct cell *end;
	struct cell *dcl;
	struct cell *cl;
	size_t ntab = 0;
	size_t nsz;
	int pos;
	
	end = buf->text + buf->length;
	for (cl = buf->text; cl < end; cl++)
		if (cl->ch == '\t')
			ntab++;
	
	nsz = buf->length + ntab * 7;
	ncl = calloc(nsz, sizeof *ncl);
	if (!ncl)
		return -1;
	
	pos = 0;
	for (cl = buf->text, dcl = ncl; cl < end; cl++)
		switch (cl->ch)
		{
		case '\t':
			do
			{
				dcl++->ch = ' ';
				pos++;
			} while (pos & 7);
			break;
		case '\n':
			*dcl++ = *cl;
			pos = 0;
			break;
		default:
			*dcl++ = *cl;
			pos++;
		}
	
	free(buf->text);
	buf->length = dcl - ncl;
	buf->text   = ncl;
	return 0;
}

static int load_buffer(struct buffer *buf, char *pathname)
{
	struct cell *end;
	struct cell *cl;
	struct stat st;
	FILE *f;
	int ch;
	int se;
	
	if (strlen(pathname) >= sizeof buf->pathname)
	{
		errno = ENAMETOOLONG;
		return -1;
	}
	
	f = fopen(pathname, "r");
	if (!f)
		return -1;
	fstat(fileno(f), &st);
	
	clear_buffer(buf);
	buf->length = st.st_size;
	buf->text   = malloc(st.st_size * sizeof *buf->text);
	if (!buf->text)
	{
		clear_buffer(buf);
		return -1;
	}
	
	cl  = buf->text;
	end = cl + st.st_size;
	while (cl < end)
	{
		ch = fgetc(f);
		if (ch == EOF)
			break;
		cl++->ch = ch;
	}
	if (feof(f))
	{
		clear_buffer(buf);
		errno = EINVAL;
		fclose(f);
		return -1;
	}
	if (ferror(f))
	{
		se = errno;
		clear_buffer(buf);
		errno = se;
		fclose(f);
		return -1;
	}
	
	if (fclose(f))
	{
		se = errno;
		clear_buffer(buf);
		errno = se;
		return -1;
	}
	
	if (expand_tabs(buf))
	{
		se = errno;
		clear_buffer(buf);
		errno = se;
		return -1;
	}
	
	strcpy(buf->pathname, pathname);
	return 0;
}

static int paste_buffer(struct buffer *dst, struct buffer *src)
{
	struct cell *cl;
	
	cl = realloc(dst->text, (dst->length + src->length) * sizeof *cl);
	if (!cl)
		return -1;
	dst->text = cl;
	
	memmove(cl + dst->cur_index + src->length,
		cl + dst->cur_index,
		(dst->length - dst->cur_index) * sizeof *cl);
	memcpy(cl + dst->cur_index, src->text, src->length * sizeof *cl);
	dst->length += src->length;
	dst->dirty = 1;
	if (dst->sel_start > dst->cur_index)
	    dst->sel_start += src->length;
	if (dst->sel_end > dst->cur_index)
	    dst->sel_end += src->length;
	return 0;
}

static int delete_selection(struct buffer *buf)
{
	struct cell *cl;
	
	memmove(buf->text + buf->sel_start,
		buf->text + buf->sel_end,
		(buf->length - buf->sel_end) * sizeof *buf->text);
	buf->length -= buf->sel_end - buf->sel_start;
	
	cl = realloc(buf->text, buf->length * sizeof *cl);
	if (cl)
		buf->text = cl;
	buf->cur_index = buf->sel_start;
	buf->sel_start = 0;
	buf->sel_end   = 0;
	return 0;
}

static int sub_buffer(struct buffer *dst, struct buffer *src)
{
	new_buffer(dst);
	
	dst->length = src->sel_end - src->sel_start;
	dst->text   = malloc(dst->length * sizeof *dst->text);
	if (!dst->text)
		return -1;
	memcpy(dst->text,
	       src->text + src->sel_start,
	       dst->length * sizeof *dst->text);
	return 0;
}

static struct cell *lineptr(struct buffer *buf, int y)
{
	struct cell *p;
	int i;
	
	for (i = 0, p = buf->text; y && i < buf->length; p++, i++)
		if (p->ch == '\n')
			y--;
	if (y)
		return NULL;
	return p;
}

static void ptrxy(struct buffer *buf, struct cell *c, int *xp, int *yp)
{
	struct cell *p;
	int x = 0;
	int y = 0;
	
	for (p = buf->text; p < c; p++)
		if (p->ch == '\n')
		{
			x = 0;
			y++;
		}
		else
			x++;
	*xp = x;
	*yp = y;
}

static int ptrline(struct buffer *buf, struct cell *c)
{
	int x, y;
	
	ptrxy(buf, c, &x, &y);
	return y;
}

static int asksave(struct buffer *buf)
{
	char msg[PATH_MAX + 64];
	
	if (!buf->dirty)
		return 0;
	
	if (*buf->pathname)
		sprintf(msg, "The %s file has been modified.\n\n", buf->pathname);
	else
		strcpy(msg, "This file has not been saved.\n\n");
	
	strcat(msg, "Do you want to save the changes?");
	
	cur_buf = buf;
	gadget_redraw(editor);
	
	if (msgbox_ask(main_form, "Code", msg) != MSGBOX_NO)
	{
		if (!*buf->pathname && !dlg_save(main_form, "Save", buf->pathname, sizeof buf->pathname))
			return -1;
		
		if (save_buffer(buf, buf->pathname))
		{
			msgbox_perror(main_form, "Code", "Could not save the buffer", errno);
			return -1;
		}
	}
	return 0;
}

static void new_click(struct menu_item *mi)
{
	if (asksave(cur_buf))
		return;
	clear_buffer(cur_buf);
	gadget_redraw(editor);
}

static void open_click(struct menu_item *mi)
{
	if (asksave(cur_buf))
		return;
	if (!dlg_open(main_form, "Open", cur_buf->pathname, sizeof cur_buf->pathname))
		return;
	if (load_buffer(cur_buf, cur_buf->pathname))
		msgbox(main_form, "Code", "Cannot load the buffer.");
	gadget_redraw(editor);
}

static void save_as_click(struct menu_item *mi)
{
	if (!dlg_save(main_form, "Save As", cur_buf->pathname, sizeof cur_buf->pathname))
		return;
	if (save_buffer(cur_buf, cur_buf->pathname))
		msgbox(main_form, "Code", "Could not save the buffer.");
	cur_buf->dirty = 0;
}

static void save_click(struct menu_item *mi)
{
	if (!*cur_buf->pathname)
	{
		save_as_click(mi);
		return;
	}
	if (save_buffer(cur_buf, cur_buf->pathname))
		msgbox(main_form, "Code", "Could not save the buffer.");
	cur_buf->dirty = 0;
}

static void close_click(struct menu_item *mi)
{
	if (asksave(cur_buf))
		return;
	exit(0);
}

static void copy_click(struct menu_item *mi)
{
	FILE *f;
	int fd;
	
	if (cur_buf->sel_start >= cur_buf->sel_end)
		return;
	
	fd = wbu_open(O_WRONLY | O_CREAT | O_TRUNC);
	if (fd < 0)
	{
		msgbox_perror(main_form, "Code", "Cannot open the shared buffer", errno);
		return;
	}
	
	f = fdopen(fd, "w");
	if (!f)
	{
		msgbox_perror(main_form, "Code", "Cannot open the shared buffer", errno);
		close(fd);
		return;
	}
	
	if (fsave_buffer_sel(cur_buf, f))
	{
		msgbox_perror(main_form, "Code", "Cannot copy", errno);
		fclose(f);
		return;
	}
	
	if (fclose(f))
		msgbox_perror(main_form, "Code", "Error closing the shared buffer", errno);
}

static void cut_click(struct menu_item *mi)
{
	FILE *f;
	int fd;
	
	if (cur_buf->sel_start >= cur_buf->sel_end)
		return;
	
	fd = wbu_open(O_WRONLY | O_CREAT | O_TRUNC);
	if (fd < 0)
	{
		msgbox_perror(main_form, "Code", "Cannot open the shared buffer", errno);
		return;
	}
	
	f = fdopen(fd, "w");
	if (!f)
	{
		msgbox_perror(main_form, "Code", "Cannot open the shared buffer", errno);
		close(fd);
		return;
	}
	
	if (fsave_buffer_sel(cur_buf, f))
	{
		msgbox_perror(main_form, "Code", "Cannot cut", errno);
		fclose(f);
		return;
	}
	
	if (fclose(f))
	{
		msgbox_perror(main_form, "Code", "Error closing the shared buffer", errno);
		return;
	}
	
	if (delete_selection(cur_buf))
		msgbox_perror(main_form, "Code", "Cannot delete", errno);
}

static void paste_click(struct menu_item *mi)
{
	char path[PATH_MAX];
	char *home;
	struct buffer buf;
	
	home = getenv("HOME");
	if (!home)
	{
		msgbox(main_form, "Code", "HOME is not set.");
		return;
	}
	if (snprintf(path, sizeof path, "%s/.clipbrd", home) >= sizeof path)
	{
		msgbox(main_form, "Code", "Path too long.");
		return;
	}
	
	if (new_buffer(&buf))
	{
		msgbox_perror(main_form, "Code", "Cannot create the buffer", errno);
		return;
	}
	if (load_buffer(&buf, path))
	{
		msgbox_perror(main_form, "Code", "Cannot load the buffer", errno);
		return;
	}
	if (paste_buffer(cur_buf, &buf))
	{
		msgbox_perror(main_form, "Code", "Cannot paste the buffer", errno);
		free_buffer(&buf);
		gadget_redraw(editor);
		return;
	}
	free_buffer(&buf);
	gadget_redraw(editor);
	return;
}

static void delete_click(struct menu_item *mi)
{
	if (cur_buf->sel_start >= cur_buf->sel_end)
		return;
	
	if (delete_selection(cur_buf))
		msgbox_perror(main_form, "Code", "Cannot delete", errno);
	
	gadget_redraw(editor);
}

static void all_click(struct menu_item *mi)
{
	cur_buf->sel_start = 0;
	cur_buf->sel_end   = cur_buf->length;
	gadget_redraw(editor);
}

static void about_click(struct menu_item *mi)
{
	dlg_about7(main_form, NULL, "Code Editor v1.3", SYS_PRODUCT, SYS_AUTHOR, SYS_CONTACT, "code.pnm");
}

static int on_close(struct form *f)
{
	int i;
	
	for (i = 0; i < sizeof buffers / sizeof *buffers; i++)
		if (asksave(&buffers[i]))
			return 0;
	return 1;
}

static void on_resize(struct form *f, int w, int h)
{
	gadget_resize(editor, w, h);
}

static void editor_redraw_line(struct gadget *g, int i, int clip)
{
	int wd = g->form->wd;
	struct cell *p, *p1;
	win_color lfg;
	win_color sbg;
	win_color sfg;
	win_color bg;
	win_color fg;
	char numstr[16];
	int font_w;
	int font_h;
	int tw, th;
	int ci = -1;
	int x;
	int y;
	
	if (i < cur_buf->top_y)
		return;
	
	if (clip)
		win_clip(wd, g->rect.x, g->rect.y, g->rect.w, g->rect.h,
			     g->rect.x, g->rect.y);
	
	win_chr_size(config.font, &font_w, &font_h, 'X');
	if (i - cur_buf->top_y > g->rect.h / font_h)
		return;
	
	p = lineptr(cur_buf, i);
	if (p)
	{
		struct cell *e = cur_buf->text + cur_buf->length;
		struct cell *v = p + cur_buf->scroll_x;
		
		for (p1 = p; p1 < v; p1++)
			if (p1 >= e || p1->ch == '\n')
			{
				p = NULL;
				break;
			}
		if (p1 == v)
		{
			ci = p - cur_buf->text + cur_buf->scroll_x;
			p += cur_buf->scroll_x;
		}
	}
	if (!p && !i)
		ci = 0;
	
	win_rgb2color(&sbg,   0, 255, 255);
	win_rgb2color(&sfg,   0,   0,   0);
	win_rgb2color(&bg,    0,   0,  64);
	win_rgb2color(&fg,  255, 255, 255);
	win_rgb2color(&lfg,  64,  64, 192);
	
	win_set_font(wd, config.font);
	
	y = font_h * (i - cur_buf->top_y);
	x = font_w * 8;
	
	sprintf(numstr, "%i", i + 1);
	win_text_size(config.font, &tw, &th, numstr);
	
	win_rect(wd,  bg, 0, y, font_w * 8, font_h);
	win_btext(wd, bg, lfg, font_w * 6 - tw, y, numstr);
	
	win_paint();
	if (p)
	{
		while (ci < cur_buf->length && p->ch != '\n')
		{
			if (ci == cur_buf->cur_index)
				win_bchr(wd, fg, bg, x, y, p->ch);
			else if (ci >= cur_buf->sel_start && ci < cur_buf->sel_end)
				win_bchr(wd, sbg, sfg, x, y, p->ch);
			else
				win_bchr(wd, bg, fg, x, y, p->ch);
			x += font_w;
			ci++;
			p++;
		}
	}
	if (ci == cur_buf->cur_index)
	{
		win_rect(wd, fg, x, y, font_w, font_h);
		x += font_w;
	}
	else if (ci >= cur_buf->sel_start && ci < cur_buf->sel_end)
	{
		win_rect(wd, sbg, x, y, font_w, font_h);
		x += font_w;
	}
	win_rect(wd, bg, x, y, g->rect.w - x, font_h);
	win_end_paint();
}

static void editor_redraw(struct gadget *g, int wd)
{
	int font_w;
	int font_h;
	int cnt;
	int i;
	
	win_chr_size(config.font, &font_w, &font_h, 'X');
	cnt = (g->rect.h + font_h - 1) / font_h;
	for (i = 0; i < cnt; i++)
		editor_redraw_line(g, i + cur_buf->top_y, 0);
}

static void editor_clrsel(struct gadget *g)
{
	cur_buf->nsel_start = -1;
	if (cur_buf->sel_start != cur_buf->sel_end)
	{
		cur_buf->sel_start = 0;
		cur_buf->sel_end   = 0;
		gadget_redraw(g);
	}
}

static void editor_updsel(struct gadget *g)
{
	if (cur_buf->nsel_start < 0)
		return;
	if (cur_buf->nsel_start < cur_buf->cur_index)
	{
		cur_buf->sel_start = cur_buf->nsel_start;
		cur_buf->sel_end   = cur_buf->cur_index;
		return;
	}
	cur_buf->sel_start = cur_buf->cur_index;
	cur_buf->sel_end   = cur_buf->nsel_start;
	return;
}

static int editor_get_indent(struct gadget *g)
{
	struct cell *cl0 = cur_buf->text;
	struct cell *cl1 = cl0 + cur_buf->cur_index;
	struct cell *end = cur_buf->text + cur_buf->length;
	struct cell *cl;
	
	if (!cur_buf->text)
		return 0;
	
	for (cl = cl1; cl >= cl0 && cl->ch != '\n'; cl--)
		;
	cl++;
	
	cl1 = cl;
	while (cl1 < end && isspace(cl1->ch))
		cl1++;
	
	return cl1 - cl;
}

static void editor_inschr(struct gadget *g, unsigned ch)
{
	struct cell *n;
	int ind = 0;
	int i;
	
	cur_buf->dirty = 1;
	
	if (ch == '\t')
	{
		int x, y;
		
		ptrxy(cur_buf, cur_buf->text + cur_buf->cur_index, &x, &y);
		
		editor_inschr(g, ' ');
		while (++x % 8 != 0)
			editor_inschr(g, ' ');
		editor_updsel(g);
		return;
	}
	
	if (ch == '\n')
		ind = editor_get_indent(g);
	
	n = realloc(cur_buf->text, (cur_buf->length + 1) * sizeof *n);
	if (!n)
	{
		wb(WB_ERROR);
		return;
	}
	cur_buf->text = n;
	
	memmove(n + cur_buf->cur_index + 1,
		n + cur_buf->cur_index,
		(cur_buf->length - cur_buf->cur_index) * sizeof *n);
	cur_buf->length++;
	
	n += cur_buf->cur_index++;
	memset(n, 0, sizeof *n);
	n->ch = ch;
	
	editor_clrsel(g);
	editor_adjpos(g);
	if (ch == '\n')
	{
		for (i = 0; i < ind; i++)
			editor_inschr(g, ' ');
		gadget_redraw(g);
	}
	else
	{
		i = ptrline(cur_buf, n);
		editor_redraw_line(g, i, 1);
	}
}

static void editor_delchr(struct gadget *g)
{
	echar ch;
	int i;
	
	cur_buf->dirty = 1;
	
	if (cur_buf->cur_index >= cur_buf->length)
		return;
	ch = cur_buf->text[cur_buf->cur_index].ch;
	
	memmove(cur_buf->text + cur_buf->cur_index,
		cur_buf->text + cur_buf->cur_index + 1,
		(cur_buf->length - cur_buf->cur_index - 1) *
			sizeof *cur_buf->text);
	cur_buf->length--;
	
	editor_clrsel(g);
	if (ch == '\n')
		gadget_redraw(g);
	else
	{
		i = ptrline(cur_buf, cur_buf->text + cur_buf->cur_index);
		editor_redraw_line(g, i, 1);
	}
}

static void editor_adjpos(struct gadget *g)
{
	int font_w;
	int font_h;
	int vc, vl;
	int x, y;
	
	win_chr_size(config.font, &font_w, &font_h, 'X');
	vc = g->rect.w / font_w;
	vl = g->rect.h / font_h;
	
	ptrxy(cur_buf, cur_buf->text + cur_buf->cur_index, &x, &y);
	if (x < cur_buf->scroll_x)
	{
		cur_buf->scroll_x = max(0, x - 8);
		gadget_redraw(g);
	}
	if (x >= cur_buf->scroll_x + vc)
	{
		cur_buf->scroll_x = x - vc + 8;
		gadget_redraw(g);
	}
	if (y < cur_buf->top_y)
	{
		cur_buf->top_y = y;
		gadget_redraw(g);
	}
	if (cur_buf->top_y + vl <= y)
	{
		cur_buf->top_y = y - vl + 1;
		gadget_redraw(g);
	}
}

static void editor_mvleft(struct gadget *g)
{
	int x, y;
	
	if (!cur_buf->cur_index)
		return;
	
	ptrxy(cur_buf, cur_buf->text + cur_buf->cur_index--, &x, &y);
	editor_updsel(g);
	editor_redraw_line(g, y, 1);
	editor_adjpos(g);
	if (!x)
		editor_redraw_line(g, y - 1, 1);
}

static void editor_mvright(struct gadget *g)
{
	int x, y;
	
	if (cur_buf->cur_index >= cur_buf->length)
		return;
	
	ptrxy(cur_buf, cur_buf->text + ++cur_buf->cur_index, &x, &y);
	editor_updsel(g);
	editor_redraw_line(g, y, 1);
	editor_adjpos(g);
	if (!x)
		editor_redraw_line(g, y - 1, 1);
}

static void editor_mvup(struct gadget *g)
{
	int x, y;
	
	ptrxy(cur_buf, cur_buf->text + cur_buf->cur_index, &x, &y);
	if (!y)
		return;
	cur_buf->cur_index = lineptr(cur_buf, y - 1) - cur_buf->text;
	editor_updsel(g);
	editor_redraw_line(g, y - 1, 1);
	editor_redraw_line(g, y, 1);
	editor_adjpos(g);
}

static void editor_mvdown(struct gadget *g)
{
	struct cell *p;
	int x, y;
	
	ptrxy(cur_buf, cur_buf->text + cur_buf->cur_index, &x, &y);
	p = lineptr(cur_buf, y + 1);
	if (!p)
		return;
	cur_buf->cur_index = p - cur_buf->text;
	editor_updsel(g);
	editor_redraw_line(g, y + 1, 1);
	editor_redraw_line(g, y, 1);
	editor_adjpos(g);
}

static void editor_mvhome(struct gadget *g)
{
	int x, y;
	
	ptrxy(cur_buf, cur_buf->text + cur_buf->cur_index, &x, &y);
	cur_buf->cur_index -= x;
	editor_updsel(g);
	editor_adjpos(g);
	editor_redraw_line(g, y, 1);
}

static void editor_mvend(struct gadget *g)
{
	while (cur_buf->cur_index < cur_buf->length && cur_buf->text[cur_buf->cur_index].ch != '\n')
		cur_buf->cur_index++;
	editor_updsel(g);
	editor_adjpos(g);
	editor_redraw_line(g, ptrline(cur_buf, cur_buf->text + cur_buf->cur_index), 1);
}

static void editor_pgup(struct gadget *g)
{
	int font_w, font_h;
	int x, y;
	int vl;
	
	win_chr_size(config.font, &font_w, &font_h, 'X');
	vl = g->rect.h / font_h;
	
	ptrxy(cur_buf, cur_buf->text + cur_buf->cur_index, &x, &y);
	if (cur_buf->top_y > vl)
	{
		cur_buf->top_y -= vl;
		y -= vl;
	}
	else
		y = 0;
	cur_buf->cur_index = lineptr(cur_buf, y) - cur_buf->text;
	
	editor_updsel(g);
	editor_adjpos(g);
	gadget_redraw(g);
}

static void editor_pgdn(struct gadget *g)
{
	int font_w, font_h;
	struct cell *p;
	int x, y;
	int vl;
	
	win_chr_size(config.font, &font_w, &font_h, 'X');
	vl = g->rect.h / font_h;
	
	ptrxy(cur_buf, cur_buf->text + cur_buf->cur_index, &x, &y);
	p = lineptr(cur_buf, y + vl);
	if (p)
		cur_buf->cur_index = p - cur_buf->text;
	else
		cur_buf->cur_index = cur_buf->length;
	
	editor_updsel(g);
	editor_adjpos(g);
	gadget_redraw(g);
}

static void editor_save(struct gadget *g)
{
	if (!*cur_buf->pathname && !dlg_save(g->form, "Save Buffer", cur_buf->pathname, sizeof cur_buf->pathname))
		return;
	
	if (save_buffer(cur_buf, cur_buf->pathname))
	{
		msgbox_perror(g->form, "Error", "Cannot save the buffer", errno);
		return;
	}
	
	msgbox(g->form, "Saved", "The buffer has been saved.");
	cur_buf->dirty = 0;
}

static void editor_copy(struct gadget *g)
{
	struct buffer sel;
	
	if (cur_buf->sel_start >= cur_buf->sel_end)
		return;
	
	if (sub_buffer(&sel, cur_buf))
	{
		msgbox_perror(g->form, "Error", "Cannot copy", errno);
		return;
	}
	if (paste_buffer(cur_buf, &sel))
	{
		msgbox_perror(g->form, "Error", "Cannot copy", errno);
		return;
	}
	free_buffer(&sel);
	
	editor_adjpos(g);
	gadget_redraw(g);
}

static void editor_move(struct gadget *g)
{
	struct buffer sel;
	int cur;
	int ss;
	int se;
	
	cur = cur_buf->cur_index;
	ss  = cur_buf->sel_start;
	se  = cur_buf->sel_end;
	
	if (ss >= se)
		return;
	
	if (sub_buffer(&sel, cur_buf))
	{
		msgbox_perror(g->form, "Error", "Cannot copy", errno);
		return;
	}
	if (paste_buffer(cur_buf, &sel))
	{
		msgbox_perror(g->form, "Error", "Cannot copy", errno);
		return;
	}
	if (ss >= cur)
	{
		ss += sel.length;
		se += sel.length;
	}
	
	cur_buf->sel_start = ss;
	cur_buf->sel_end   = se;
	delete_selection(cur_buf);
	if (ss <= cur)
		cur -= sel.length;
	
	cur_buf->cur_index = cur;
	cur_buf->sel_start = cur;
	cur_buf->sel_end   = cur + sel.length;
	free_buffer(&sel);
	
	gadget_redraw(g);
}

static void editor_delete(struct gadget *g)
{
	struct cell *p;
	int y;
	
	if (cur_buf->sel_start == cur_buf->sel_end)
	{
		y = ptrline(cur_buf, cur_buf->text + cur_buf->cur_index);
		p = lineptr(cur_buf, y);
		cur_buf->sel_start = p - cur_buf->text;
		p = lineptr(cur_buf, y + 1);
		if (p)
			cur_buf->sel_end = p - cur_buf->text;
		else
			cur_buf->sel_end = cur_buf->length;
	}
	
	delete_selection(cur_buf);
	gadget_redraw(g);
}

static void editor_key_down(struct gadget *g, unsigned ch, unsigned shift)
{
	if ((shift & WIN_SHIFT_CTRL) && ch >= WIN_KEY_F12 && ch <= WIN_KEY_F1)
	{
		cur_buf  = &buffers[11 - (ch - WIN_KEY_F12)];
		prev_buf = NULL;
		gadget_redraw(g);
		return;
	}
	
	switch (ch)
	{
	case '\b':
		if (cur_buf->cur_index)
		{
			editor_mvleft(g);
			editor_delchr(g);
		}
		break;
	case 27:
		if (prev_buf)
		{
			cur_buf  = prev_buf;
			prev_buf = NULL;
			gadget_redraw(g);
		}
		break;
	case WIN_KEY_DEL:
		editor_delchr(g);
		break;
	case WIN_KEY_LEFT:
		editor_mvleft(g);
		break;
	case WIN_KEY_RIGHT:
		editor_mvright(g);
		break;
	case WIN_KEY_UP:
		editor_mvup(g);
		break;
	case WIN_KEY_DOWN:
		editor_mvdown(g);
		break;
	case WIN_KEY_HOME:
		if (shift & WIN_KEY_CTRL)
		{
			cur_buf->cur_index = 0;
			cur_buf->top_y = 0;
			editor_updsel(g);
			gadget_redraw(g);
		}
		else
			editor_mvhome(g);
		break;
	case WIN_KEY_END:
		editor_mvend(g);
		break;
	case WIN_KEY_F2:
		editor_save(g);
		break;
	case WIN_KEY_F3:
		if (cur_buf->nsel_start >= 0)
		{
			cur_buf->nsel_start = -1;
			break;
		}
		editor_clrsel(g);
		cur_buf->nsel_start = cur_buf->cur_index;
		break;
	case WIN_KEY_F8:
		editor_delete(g);
		break;
	case WIN_KEY_F5:
		editor_copy(g);
		break;
	case WIN_KEY_F6:
		editor_move(g);
		break;
	case WIN_KEY_PGUP:
		editor_pgup(g);
		break;
	case WIN_KEY_PGDN:
		editor_pgdn(g);
		break;
	default:
		if (ch == '\n' || ch == '\t' || isprint(ch))
			editor_inschr(g, ch);
	}
}

static struct gadget *editor_creat(struct form *f, int x, int y, int w, int h)
{
	struct gadget *g;
	
	g = gadget_creat(f, x, y, w, h);
	if (!g)
		return NULL;
	
	gadget_set_redraw_cb(g, editor_redraw);
	gadget_set_key_down_cb(g, editor_key_down);
	gadget_set_want_focus(g, 1);
	gadget_set_want_tab(g, 1);
	return g;
}

static void run_click(struct menu_item *mi)
{
	char spathname[PATH_MAX];
	char pathname[PATH_MAX];
	char *ext;
	struct stat st;
	pid_t pid;
	int pst;
	
	save_click(NULL);
	
	if (!*cur_buf->pathname)
		return;
	
	strcpy(spathname, cur_buf->pathname);
	strcpy(pathname, cur_buf->pathname);
	
	ext = strrchr(pathname, '.');
	if (!ext)
		goto fail;
	
	if (strcmp(ext, ".c"))
		goto fail;
	
	signal(SIGCHLD, SIG_DFL);
	
	pid = fork();
	if (!pid)
	{
		strcpy(ext, ".err");
		
		close(1);
		close(2);
		
		if (open(pathname, O_CREAT | O_TRUNC | O_WRONLY, 0666) != 1)
			_exit(255);
		
		if (open(pathname, O_CREAT | O_TRUNC | O_WRONLY, 0666) != 2)
			_exit(255);
		
		*ext = 0;
		execl(COMPILER, COMPILER, "-o", pathname, spathname, (void *)NULL);
		_exit(255);
	}
	if (pid < 0)
		goto fail;
	
	while (wait(&pst) != pid)
		win_idle();
	
	if (WEXITSTATUS(pst) == 255)
		goto fail;
	
	strcpy(ext, ".err");
	stat(pathname, &st);
	
	if (pst || st.st_size)
	{
		load_buffer(&errbuf, pathname);
		prev_buf = cur_buf;
		cur_buf = &errbuf;
		gadget_redraw(editor);
		
		if (pst)
			return;
	}
	else
		unlink(pathname);
	
	signal(SIGCHLD, SIG_IGN);
	*ext = 0;
	
	if (_mkcanon(NULL, pathname))
		goto fail;
	
	if (mi->l_data)
		config.run_mode = mi->l_data;
	
	switch (config.run_mode)
	{
	case 1:
		_newtaskl(pathname, pathname, (void *)NULL);
		break;
	case 2:
		_newtaskl(_PATH_B_VTTY, _PATH_B_VTTY, "-T", "Standard I/O", "-w", "--", "/bin/detach", pathname, (void *)NULL);
		break;
	default:
		_newtaskl(_PATH_B_VTTY, _PATH_B_VTTY, "-T", "Standard I/O", "-wH", "--", pathname, (void *)NULL);
	}
	
	return;
fail:
	wb(WB_ERROR);
}

static void help_click(struct menu_item *mi)
{
	help();
}

static void font_click(struct menu_item *mi)
{
	config.font = mi->l_data;
	gadget_redraw(editor);
}

static void goto_click(struct menu_item *mi)
{
	struct cell *p;
	char buf[16];
	int y;
	
	sprintf(buf, "%i", ptrline(cur_buf, cur_buf->text + cur_buf->cur_index) + 1);
	dlg_input(main_form, "Go to line", buf, sizeof buf);
	y = atoi(buf) - 1;
	
	p = lineptr(cur_buf, y);
	if (!p)
		return;
	
	cur_buf->cur_index = p - cur_buf->text;
	editor_updsel(editor);
	editor_redraw_line(editor, y + 1, 1);
	editor_redraw_line(editor, y, 1);
	editor_adjpos(editor);
}

static void create_form(void)
{
	struct form_state fst;
	struct menu *m;
	
	main_menu = menu_creat();
	
	m = menu_creat();
	menu_newitem4(m, "New",		'N', new_click);
	menu_newitem (m, "-",		NULL);
	menu_newitem4(m, "Open...",	'O', open_click);
	menu_newitem (m, "-",		NULL);
	menu_newitem4(m, "Save",	'S', save_click);
	menu_newitem4(m, "Save As...",	'F', save_as_click);
	menu_newitem (m, "-",		NULL);
	menu_newitem4(m, "Close",	'W', close_click);
	menu_submenu(main_menu, "File",	m);
	
	m = menu_creat();
	menu_newitem5(m, "Copy",	WIN_KEY_INS, WIN_SHIFT_CTRL,	copy_click);
	menu_newitem5(m, "Cut",		WIN_KEY_DEL, WIN_SHIFT_SHIFT,	cut_click);
	menu_newitem5(m, "Paste",	WIN_KEY_INS, WIN_SHIFT_SHIFT,	paste_click);
	menu_newitem (m, "-",		NULL);
	menu_newitem4(m, "Delete",	WIN_KEY_DEL,			delete_click);
	menu_newitem (m, "-",		NULL);
	menu_newitem4(m, "Go to line",	'L', goto_click);
	menu_newitem (m, "-",		NULL);
	menu_newitem4(m, "Select All",	'A', all_click);
	menu_submenu(main_menu, "Edit", m);
	
	m = menu_creat();
	menu_newitem4(m, "Normal",		'1', font_click)->l_data = WIN_FONT_MONO;
	menu_newitem4(m, "Narrow",		'2', font_click)->l_data = WIN_FONT_MONO_N;
	menu_newitem (m, "-", NULL);
	menu_newitem4(m, "Large",		'3', font_click)->l_data = WIN_FONT_MONO_L;
	menu_newitem4(m, "Large Narrow",	'4', font_click)->l_data = WIN_FONT_MONO_LN;
	menu_submenu(main_menu, "Font", m);
	
	m = menu_creat();
	menu_newitem5(m, "Run on Desktop", WIN_KEY_F12,	0,		run_click)->l_data = 1;
	menu_newitem5(m, "Run in VTTY",	WIN_KEY_F11,	0,		run_click)->l_data = 2;
	menu_newitem5(m, "Run",		WIN_KEY_F10,	0,		run_click)->l_data = 3;
	menu_newitem (m, "-",		NULL);
	menu_newitem5(m, "Help ...",	WIN_KEY_F1,	0,		help_click);
	menu_newitem (m, "About ...",	about_click);
	menu_submenu(main_menu, "Options", m);
	
	menu_newitem(main_menu, "-",	NULL);
	menu_newitem(main_menu, "Run",	run_click);
	menu_newitem(main_menu, "-",	NULL);
	
	main_form = form_creat(FORM_APPFLAGS | FORM_NO_BACKGROUND, 1, -1, -1, 600, 400, "Code");
	form_set_menu(main_form, main_menu);
	form_on_close(main_form, on_close);
	form_on_resize(main_form, on_resize);
	
	editor = editor_creat(main_form, 0, 0, 600, 400);
	gadget_focus(editor);
	
	if (!c_load("code_fst", &fst, sizeof fst))
		form_set_state(main_form, &fst);
}

int main(int argc, char **argv)
{
	struct form_state fst;
	int i;
	
	if (win_attach())
		err(255, NULL);
	
	if (argc > 2)
	{
		msgbox(NULL, "Code", "usage:\n\n  code [PATHNAME]");
		return 1;
	}
	
	if (c_load("code", &config, sizeof config))
		config.font = DEFAULT_FONT;
	
	for (i = 0; i < sizeof buffers / sizeof *buffers; i++)
		if (new_buffer(&buffers[i]))
		{
			msgbox(NULL, "Code", "Cannot initialize the buffers.");
			return 1;
		}
	if (new_buffer(&errbuf))
	{
		msgbox(NULL, "Code", "Cannot initialize the buffers.");
		return 1;
	}
	
	if (argc > 1)
	{
		if (load_buffer(&buffers[0], argv[1]))
		{
			msgbox(NULL, "Code", "Cannot load the buffer.");
			return 1;
		}
		if (strlen(argv[1]) >= sizeof buffers[0].pathname)
		{
			msgbox(NULL, "Code", "Pathname too long.");
			return 1;
		}
		strcpy(buffers[0].pathname, argv[1]);
	}
	
	create_form();
	form_wait(main_form);
	form_get_state(main_form, &fst);
	c_save("code", &config, sizeof config);
	c_save("code_fst", &fst, sizeof fst);
	return 0;
}
