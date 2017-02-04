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

#include <wingui_metrics.h>
#include <wingui_msgbox.h>
#include <wingui_color.h>
#include <wingui_form.h>
#include <wingui.h>
#include <string.h>
#include <stdlib.h>
#include <bioctl.h>
#include <stdio.h>
#include <timer.h>
#include <errno.h>
#include <time.h>
#include <err.h>

#define HISTORY_LENGTH	30

struct diskview
{
	struct gadget *frame;
	struct gadget *read_label;
	struct gadget *write_label;
	struct gadget *error_label;
	struct gadget *chart;
};

struct history
{
	unsigned values[HISTORY_LENGTH];
	unsigned max;
};

static struct form *main_form;
static struct gadget *vsbar;

static struct bdev_stat *curr_stat, *prev_stat;
static struct timeval curr_tv, prev_tv;

static struct history *history;

static int bdev_max;
static int bdev_cnt;

static struct diskview *views;
static int view_cnt;

static int item_height = 90;
static int item_width  = 580;
static int item_x      = 0;

static int form_w = 600;
static int form_h = 400;

struct chart_data
{
	size_t	  value_cnt;
	unsigned *values;
	unsigned  max;
};

static void chart_redraw_frame(struct gadget *g, int wd)
{
	win_color hi1;
	win_color sh1;
	win_color hi2;
	win_color sh2;
	
	hi1 = wc_get(WC_HIGHLIGHT1);
	sh1 = wc_get(WC_SHADOW1);
	hi2 = wc_get(WC_HIGHLIGHT2);
	sh2 = wc_get(WC_SHADOW2);
	
	win_hline(wd, sh2, 0, 0, g->rect.w);
	win_hline(wd, hi2, 0, g->rect.h - 1, g->rect.w);
	win_vline(wd, sh2, 0, 0, g->rect.h);
	win_vline(wd, hi2, g->rect.w - 1, 0, g->rect.h);
	
	win_hline(wd, sh1, 1, 1, g->rect.w - 2);
	win_hline(wd, hi1, 1, g->rect.h - 2, g->rect.w - 2);
	win_vline(wd, sh1, 1, 1, g->rect.h - 2);
	win_vline(wd, hi1, g->rect.w - 2, 1, g->rect.h - 2);
}

static void chart_redraw(struct gadget *g, int wd)
{
	static const int margin = 4;
	
	struct chart_data *cd = g->p_data;
	win_color bg = wc_get(WC_BG);
	win_color fg = wc_get(WC_FG);
	size_t cnt = cd->value_cnt;
	size_t i;
	unsigned max = cd->max;
	int rw = g->rect.w - 2 * margin,
	    rh = g->rect.h - 2 * margin;
	
	win_rect(wd, bg, 2, 2, g->rect.w - 4, g->rect.h - 4);
	chart_redraw_frame(g, wd);
	
	if (!cnt)
		return;
	
	if (!max)
		max = 1;
	
	for (i = 0; i < cnt; i++)
	{
		int x, y, w, h;
		
		x  = i * rw / cnt;
		y  = cd->values[i] * rh / max;
		y  = g->rect.h - y;
		w  = (i + 1) * rw / cnt;
		w -= x + 2;
		h  = rh - y;
		x += margin;
		y += margin;
		
		win_rect(wd, fg, x, y, w, h);
	}
}

static struct gadget *chart_creat(struct form *f, int x, int y, int w, int h)
{
	struct chart_data *cd;
	struct gadget *g;
	
	cd = calloc(1, sizeof *cd);
	if (cd == NULL)
		return NULL;
	
	g = gadget_creat(f, x, y, w, h);
	if (!g)
	{
		free(cd);
		return NULL;
	}
	
	g->redraw = chart_redraw;
	g->p_data = cd; // XXX
	return g;
}

static void chart_set_max(struct gadget *g, unsigned max)
{
	struct chart_data *cd = g->p_data;
	
	cd->max = max;
	
	gadget_redraw(g);
}

static void chart_set_data(struct gadget *g, unsigned *values, size_t cnt)
{
	struct chart_data *cd = g->p_data;
	
	cd->values = values;
	cd->value_cnt = cnt;
	
	gadget_redraw(g);
}

static void collect_devs(void)
{
	int i;
	
	bdev_max = _bdev_max();
	if (bdev_max < 0)
	{
		msgbox_perror(main_form, "Disk Performance", "", errno);
		exit(1);
	}
	
	curr_stat = calloc(bdev_max, sizeof *curr_stat);
	if (!curr_stat)
	{
		msgbox_perror(main_form, "Disk Performance", "", errno);
		exit(1);
	}
	
	prev_stat = calloc(bdev_max, sizeof *prev_stat);
	if (!prev_stat)
	{
		msgbox_perror(main_form, "Disk Performance", "", errno);
		exit(1);
	}
	
	history = calloc(bdev_max, sizeof *history);
	if (!history)
	{
		msgbox_perror(main_form, "Disk Performance", "", errno);
		exit(1);
	}
	
	if (_bdev_stat(curr_stat))
	{
		msgbox_perror(main_form, "Disk Performance", "Cannot get stats",
			      errno);
		exit(1);
	}
	memcpy(prev_stat, curr_stat, sizeof *prev_stat * bdev_max);
	
	gettimeofday(&curr_tv, NULL);
	prev_tv = curr_tv;
	prev_tv.tv_sec--;
	
	for (i = 0; i < bdev_max; i++)
		if (!*curr_stat[i].name)
			break;
	bdev_cnt = i;
}

static void create_view(int i)
{
	struct bdev_stat *bs = &curr_stat[i];
	struct diskview *v = &views[i];
	int y = i * item_height;
	
	v->frame	= frame_creat(main_form, item_x, y, item_width, item_height, bs->name);
	v->read_label	= label_creat(main_form, 20,  y + 20, "");
	v->write_label	= label_creat(main_form, 20,  y + 40, "");
	v->error_label	= label_creat(main_form, 20,  y + 60, "");
	v->chart	= chart_creat(main_form, 140, y + 10, item_width - 150, item_height - 20);
}

static void free_view(struct diskview *v)
{
	gadget_remove(v->frame);
	gadget_remove(v->read_label);
	gadget_remove(v->write_label);
	gadget_remove(v->error_label);
	gadget_remove(v->chart);
}

static void regen_views(void)
{
	int vcnt = form_h / item_height;
	int i;
	
	if (views)
	{
		for (i = 0; i < view_cnt; i++)
			free_view(&views[i]);
		
		free(views);
		
		view_cnt = 0;
		views = NULL;
	}
	
	if (vcnt > bdev_cnt)
		vcnt = bdev_cnt;
	if (vcnt < 1)
		vcnt = 1;
	
	view_cnt = vcnt;
	views = calloc(view_cnt, sizeof *views);
	
	for (i = 0; i < view_cnt; i++)
		create_view(i);
}

static const char *fmt_speed(const char *label, long usec, uint64_t prev, uint64_t curr)
{
	static char buf[64];
	
	uint64_t diff = curr - prev;
	uint64_t kib  = diff * 1000000 / usec / 2;
	uint64_t mib  = diff * 1000000 / usec / 2048;
	
	if (kib >= 10000)
	{
		sprintf(buf, "%s: %ji MiB/s", label, (uintmax_t)mib);
		return buf;
	}
	
	sprintf(buf, "%s: %ji KiB/s", label, (uintmax_t)kib);
	return buf;
}

static void refresh_view(struct diskview *v, long t, int sti)
{
	struct bdev_stat *pst = &prev_stat[sti];
	struct bdev_stat *cst = &curr_stat[sti];
	struct history *h = &history[sti];
	char buf[64];
	long err;
	
	err  = cst->error_cnt - pst->error_cnt;
	err *= 1000000;
	err /= t;
	
	sprintf(buf, "%li errors/sec", err);
	
	frame_set_text(v->frame, cst->name);
	
	label_set_text(v->read_label,  fmt_speed("Reads",  t, pst->read_cnt, cst->read_cnt));
	label_set_text(v->write_label, fmt_speed("Writes", t, pst->write_cnt, cst->write_cnt));
	label_set_text(v->error_label, buf);
	
	chart_set_data(v->chart, h->values, HISTORY_LENGTH);
	chart_set_max(v->chart, h->max);
	
	gadget_show(v->frame);
	gadget_show(v->read_label);
	gadget_show(v->write_label);
	gadget_show(v->error_label);
	gadget_show(v->chart);
}

static void clear_view(struct diskview *v)
{
	gadget_hide(v->frame);
	gadget_hide(v->read_label);
	gadget_hide(v->write_label);
	gadget_hide(v->error_label);
	gadget_hide(v->chart);
}

static void refresh(void)
{
	int off = vsbar_get_pos(vsbar);
	long t;
	int i, n;
	
	t  = (curr_tv.tv_sec  - prev_tv.tv_sec) * 1000000;
	t +=  curr_tv.tv_usec - prev_tv.tv_usec;
	
	n = off;
	for (i = 0; i < view_cnt; i++, n++)
	{
		struct diskview *v = &views[i];
		
		if (n >= bdev_cnt)
			break;
		
		refresh_view(v, t, i + off);
	}
	
	for (; i < view_cnt; i++)
		clear_view(&views[i]);
}

static void sample(void)
{
	struct bdev_stat *st;
	struct timeval tv;
	long diff;
	int i;
	
	gettimeofday(&tv, NULL);
	
	diff  = (tv.tv_sec  - curr_tv.tv_sec) * 1000000;
	diff +=  tv.tv_usec - curr_tv.tv_usec;
	
	if (diff < 500000)
		return;
	
	prev_tv = curr_tv;
	curr_tv = tv;
	
	st = prev_stat;
	prev_stat = curr_stat;
	curr_stat = st;
	
	_bdev_stat(curr_stat);
	
	for (i = 0; i < bdev_cnt; i++)
	{
		struct bdev_stat *pst = &prev_stat[i];
		struct bdev_stat *cst = &curr_stat[i];
		struct history *h = &history[i];
		uint64_t diff;
		
		diff  = cst->read_cnt  - pst->read_cnt;
		diff += cst->write_cnt - pst->write_cnt;
		diff += cst->error_cnt - pst->error_cnt;
		
		memmove(h->values, h->values + 1, sizeof h->values - sizeof *h->values);
		h->values[HISTORY_LENGTH - 1] = diff;
		
		if (h->max < diff)
			h->max = diff;
	}
}

static void on_resize(struct form *form, int w, int h)
{
	int sbw = wm_get(WM_SCROLLBAR);
	int vcnt = h / item_height;
	
	if (vcnt < 1)
		vcnt = 1;
	
	form_w = w;
	form_h = h;
	
	gadget_move(vsbar, w - sbw, 0);
	gadget_resize(vsbar, sbw, h);
	
	item_width = w - sbw;
	
	regen_views();
	refresh();
	
	vsbar_set_limit(vsbar, bdev_cnt - vcnt + 1);
	vsbar_set_pos(vsbar, 0);
	
	form_resize(form, w, vcnt * item_height);
}

static void on_scroll(struct gadget *g, int pos)
{
	refresh();
}

static void create_form(void)
{
	int sbw = wm_get(WM_SCROLLBAR);
	
	main_form = form_creat(FORM_APPFLAGS, 1, -1, -1, 600, 400, "Disk Performance");
	form_on_resize(main_form, on_resize);
	
	vsbar = vsbar_creat(main_form, form_w - sbw, 0, sbw, form_h);
	vsbar_on_move(vsbar, on_scroll);
}

static void tmr_fired(void *data)
{
	sample();
	refresh();
}

int main()
{
	struct timeval tv;
	
	if (win_attach())
		err(255, NULL);
	
	collect_devs();
	create_form();
	
	tv.tv_usec = 0;
	tv.tv_sec = 1;
	tmr_creat(&tv, &tv, tmr_fired, NULL, 1);
	
	on_resize(main_form, 600, 400);
	
	form_wait(main_form);
	return 0;
}
