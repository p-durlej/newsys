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

#include <wingui_cgadget.h>
#include <wingui_msgbox.h>
#include <wingui_color.h>
#include <wingui_form.h>
#include <wingui.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <newtask.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <err.h>

#define HWCLOCK		"/sbin/hwclock"

#define WEEK_START	1

struct gadget *tod;
struct gadget *cal;
struct form *form;

int time_chg;

int on_close(struct form *f)
{
	exit(0);
}

#define CAL_FONT	WIN_FONT_DEFAULT

#define get_cal_data(g)	((struct cal_data *)((g)->p_data))

static void cal_focus(struct gadget *g);
static void cal_remove(struct gadget *g);
static void cal_redraw_day(struct gadget *g, int wd, time_t t);
static void cal_redraw(struct gadget *g, int wd);
static void cal_ptr_down(struct gadget *g, int x, int y, int button);
static void cal_key_down(struct gadget *g, unsigned ch, unsigned shift);

struct gadget *	cal_creat(struct form *f, int x, int y);
void		cal_on_change(struct gadget *g, void (*proc)(struct gadget *g));
void		cal_set_time(struct gadget *g, time_t t);
time_t		cal_get_time(struct gadget *g);

static char *cal_mstr[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug",
			      "Sep", "Oct", "Nov", "Dec" };
static char *cal_dstr[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

struct cal_data
{
	void (*on_change)(struct gadget *g);
	
	time_t time;
	int cell_w;
	int cell_h;
};

static void cal_focus(struct gadget *g)
{
	win_paint();
	gadget_clip(g, 0, 0, g->rect.w, g->rect.h, 0, 0);
	cal_redraw_day(g, g->form->wd, get_cal_data(g)->time);
	win_end_paint();
}

static void cal_remove(struct gadget *g)
{
	free(get_cal_data(g));
}

static void cal_redraw_day(struct gadget *g, int wd, time_t t)
{
	struct cal_data *cd = get_cal_data(g);
	struct tm *tm = localtime(&t);
	win_color s_bg;
	win_color s_fg;
	win_color bg;
	win_color fg;
	char str[4];
	int cell_i;
	int w, h;
	int x, y;
	
	if (form_get_focus(g->form) == g)
	{
		s_bg = wc_get(WC_SEL_BG);
		s_fg = wc_get(WC_SEL_FG);
	}
	else
	{
		s_bg = wc_get(WC_NFSEL_BG);
		s_fg = wc_get(WC_NFSEL_FG);
	}
	bg = wc_get(WC_BG);
	fg = wc_get(WC_FG);
	
	cell_i  = (42 + tm->tm_wday - (tm->tm_mday - 1) - WEEK_START) % 7;
	cell_i += tm->tm_mday - 1;
	
	x  = (cell_i % 7) * cd->cell_w;
	y  = (cell_i / 7) * cd->cell_h;
	y += cd->cell_h * 2;
	
	sprintf(str, "%i ", (int)tm->tm_mday);
	win_text_size(CAL_FONT, &w, &h, str);
	
	if (t == cd->time)
	{
		win_rect(wd, s_bg, x, y, cd->cell_w, cd->cell_h);
		win_btext(wd, s_bg, s_fg, x + cd->cell_w - w, y, str);
		return;
	}
	win_rect(wd, bg, x, y, cd->cell_w, cd->cell_h);
	win_btext(wd, bg, fg, x + cd->cell_w - w, y, str);
}

static void cal_redraw(struct gadget *g, int wd)
{
	struct cal_data *cd = get_cal_data(g);
	struct tm *tm = localtime(&cd->time);
	time_t t = cd->time;
	win_color h_bg;
	win_color h_fg;
	win_color bg;
	win_color fg;
	char str[64];
	int mon;
	int i;
	
	h_bg = wc_get(WC_TITLE_BG);
	h_fg = wc_get(WC_TITLE_FG);
	bg   = wc_get(WC_BG);
	fg   = wc_get(WC_FG);
	
	win_set_font(wd, CAL_FONT);
	win_paint();
	win_rect(wd, h_bg, 0, 0, g->rect.w, cd->cell_h);
	win_rect(wd, bg, 0, cd->cell_h, g->rect.w, g->rect.h - cd->cell_h);
	sprintf(str, "%s %i", cal_mstr[tm->tm_mon], (int)tm->tm_year + 1900);
	win_text(wd, h_fg, 1, 0, str);
	
	for (i = 0; i < 7; i++)
	{
		const char *dstr = cal_dstr[(i + WEEK_START) % 7];
		int w, h;
		int x;
		
		win_text_size(CAL_FONT, &w, &h, dstr);
		x  = i * cd->cell_w;
		x += (cd->cell_w - w) / 2;
		win_text(wd, fg, x + 1, cd->cell_h, dstr);
		win_text(wd, fg, x,	cd->cell_h, dstr);
	}
	win_hline(wd, fg, 0, cd->cell_h * 2 - 1, g->rect.w);
	
	t  -= (tm->tm_mday - 1) * 60 * 60 * 24;
	tm  = localtime(&t);
	mon = tm->tm_mon;
	do
	{
		cal_redraw_day(g, wd, t);
		
		t += 60 * 60 * 24;
		tm = localtime(&t);
	} while (tm && tm->tm_mon == mon);
	
	win_end_paint();
}

static void cal_ptr_down(struct gadget *g, int x, int y, int button)
{
	struct cal_data *cd = get_cal_data(g);
	struct tm *tm;
	int cell_i;
	time_t t;
	
	cell_i = x / cd->cell_w + (y / cd->cell_h) * 7 - 14;
	if (cell_i < 0)
		return;
	
	tm = localtime(&cd->time);
	t  = cd->time;
	t -= (tm->tm_mday - 1) * 60 * 60 * 24;
	t += cell_i * 60 * 60 * 24;
	t -= (42 + tm->tm_wday - (tm->tm_mday - 1) - WEEK_START) % 7 * 60 * 60 * 24;
	cal_set_time(g, t);
}

static const int __libc_monthlen[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31  };

static void cal_key_down(struct gadget *g, unsigned ch, unsigned shift)
{
	struct cal_data *cd = get_cal_data(g);
	struct tm *tm;
	time_t t;
	
	switch (ch)
	{
	case WIN_KEY_HOME:
		cal_set_time(g, time(NULL));
		break;
	case WIN_KEY_LEFT:
		cal_set_time(g, cd->time - 60 * 60 * 24);
		break;
	case WIN_KEY_RIGHT:
		cal_set_time(g, cd->time + 60 * 60 * 24);
		break;
	case WIN_KEY_UP:
		cal_set_time(g, cd->time - 60 * 60 * 24 * 7);
		break;
	case WIN_KEY_DOWN:
		cal_set_time(g, cd->time + 60 * 60 * 24 * 7);
		break;
	case WIN_KEY_PGUP:
		tm = localtime(&cd->time);
		if (shift & WIN_SHIFT_CTRL)
			tm->tm_year -= 100;
		else if (shift & WIN_SHIFT_SHIFT)
			tm->tm_year--;
		else if (!tm->tm_mon)
		{
			tm->tm_year--;
			tm->tm_mon = 11;
		}
		else
			tm->tm_mon--;
		cltime(tm);
		t = mktime(tm);
		if (t != -1 || tm->tm_year == 69)
			cal_set_time(g, mktime(tm));
		break;
	case WIN_KEY_PGDN:
		tm = localtime(&cd->time);
		if (shift & WIN_SHIFT_CTRL)
			tm->tm_year += 100;
		else if (shift & WIN_SHIFT_SHIFT)
			tm->tm_year++;
		else if (tm->tm_mon == 11)
		{
			tm->tm_year++;
			tm->tm_mon = 0;
		}
		else
			tm->tm_mon++;
		cltime(tm);
		t = mktime(tm);
		if (t != -1 || tm->tm_year == 69)
			cal_set_time(g, mktime(tm));
		break;
	}
}

struct gadget *cal_creat(struct form *f, int x, int y)
{
	struct cal_data *cd;
	struct gadget *g;
	int w, h;
	
	cd = malloc(sizeof *cd);
	if (!cd)
		return NULL;
	cd->on_change = NULL;
	cd->time = time(NULL);
	win_text_size(CAL_FONT, &cd->cell_w, &cd->cell_h, " 13 ");
	
	w = cd->cell_w * 7;
	h = cd->cell_h * 8;
	
	g = gadget_creat(f, x, y, w, h);
	if (!g)
	{
		free(cd);
		return NULL;
	}
	g->p_data = cd;
	
	gadget_set_want_focus(g, 1);
	gadget_set_focus_cb(g, cal_focus);
	gadget_set_remove_cb(g, cal_remove);
	gadget_set_redraw_cb(g, cal_redraw);
	gadget_set_ptr_down_cb(g, cal_ptr_down);
	gadget_set_key_down_cb(g, cal_key_down);
	return g;
}

void cal_on_change(struct gadget *g, void (*proc)(struct gadget *g))
{
	struct cal_data *cd = get_cal_data(g);
	
	cd->on_change = proc;
}

void cal_set_time(struct gadget *g, time_t t)
{
	struct cal_data *cd = get_cal_data(g);
	struct tm *tm;
	struct tm otm;
	time_t ot;
	
	if (cd->time == t)
		return;
	
	ot = cd->time;
	tm = localtime(&ot);
	if (!tm)
		return;
	otm = *tm;
	
	tm = localtime(&t);
	if (!tm)
		return;
	
	cd->time = t;
	if (cd->on_change)
		cd->on_change(g);
	
	if (otm.tm_mon != tm->tm_mon || otm.tm_year != tm->tm_year)
		gadget_redraw(g);
	else
	{
		win_paint();
		gadget_clip(g, 0, 0, g->rect.w, g->rect.h, 0, 0);
		cal_redraw_day(g, g->form->wd, ot);
		cal_redraw_day(g, g->form->wd, t);
		win_end_paint();
	}
}

time_t cal_get_time(struct gadget *g)
{
	return get_cal_data(g)->time;
}

static void sig_alrm(int nr)
{
	struct tm *tm;
	time_t t;
	char str[16];
	
	if (form_get_focus(form) != tod && !time_chg)
	{
		t  = time(NULL);
		tm = localtime(&t);
		sprintf(str, "%i:%02i:%02i", (int)tm->tm_hour, (int)tm->tm_min, (int)tm->tm_sec);
		input_set_text(tod, str);
		time_chg = 0;
	}
	
	win_signal(SIGALRM, sig_alrm);
	alarm(1);
}

static void tod_on_chg(struct gadget *g)
{
	time_chg = 1;
}

static int update_hw(void)
{
	pid_t pid;
	int st;
	
	pid = _newtaskl(HWCLOCK, HWCLOCK, "w", NULL);
	if (pid < 0)
		return -1;
	while (wait(&st) != pid);
	if (WEXITSTATUS(st))
	{
		errno = WEXITSTATUS(st);
		return -1;
	}
	if (WTERMSIG(st))
	{
		errno = EINTR;
		return -1;
	}
	return 0;
}

static void set_time(void)
{
	struct tm tm;
	time_t t;
	char *p;
	
	t  = cal_get_time(cal);
	tm = *localtime(&t);
	
	tm.tm_hour = strtoul(tod->text, &p, 10);
	if (*p != ':')
		goto fail;
	tm.tm_min = strtoul(p + 1, &p, 10);
	if (*p != ':')
		goto fail;
	tm.tm_sec = strtoul(p + 1, &p, 10);
	if (*p)
		goto fail;
	
	t = mktime(&tm);
	if (stime(&t))
	{
		msgbox_perror(form, "Error", "Cannot set time", errno);
		return;
	}
	if (msgbox_ask(form, "Save", "Software clock has been updated.\n\n"
				     "Do you want to write the time to the\n"
				     "hardware clock?") == MSGBOX_YES &&
	    update_hw())
		msgbox_perror(form, "Error", "Cannot set hardware clock", errno);
	exit(0);
	
fail:
	msgbox(form, "Error", "Invalid time specification.");
}

int main()
{
	struct win_rect cfr;
	struct gadget *cancel;
	struct gadget *ok;
	struct gadget *cf;
	
	if (win_attach())
		err(255, NULL);
	
	form = form_load("/lib/forms/pref.time.frm");
	form_on_close(form, on_close);
	
	cancel	= gadget_find(form, "cancel");
	ok	= gadget_find(form, "ok");
	cf	= gadget_find(form, "calframe");
	tod	= gadget_find(form, "input");
	
	input_on_change(tod, tod_on_chg);
	
	gadget_get_rect(cf, &cfr);
	gadget_remove(cf);
	
	if (geteuid())
	{
		button_set_text(ok, "Close");
		gadget_remove(cancel);
	}
	
	cal = cal_creat(form, cfr.x, cfr.y);
	
	gadget_focus(cal);
	
	win_signal(SIGALRM, sig_alrm);
	raise(SIGALRM);
	
	if (form_wait(form) == 1 && !geteuid())
		set_time();
	return 0;
}
