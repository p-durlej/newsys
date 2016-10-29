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

#include <wingui_metrics.h>
#include <wingui_bitmap.h>
#include <wingui_msgbox.h>
#include <wingui_color.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <newtask.h>
#include <wingui.h>
#include <string.h>
#include <unistd.h>
#include <systat.h>
#include <string.h>
#include <stdlib.h>
#include <timer.h>
#include <event.h>
#include <errno.h>
#include <stdio.h>
#include <paths.h>
#include <mount.h>
#include <time.h>
#include <err.h>

#define OSK_ICON64	"/lib/icons64/keyboard_small.pnm"
#define OSK_ICON	"/lib/icons/keyboard_small.pnm"

#define WIN_MAX		64

#define DEBUG		0

static int tb_height = 24;

struct win_data
{
	char	title[WIN_TITLE_MAX + 1];
	int	visible;
	int	valid;
};

static struct win_data *win_list[WIN_MAX];
static struct win_data	win_data[WIN_MAX];
static int		show_memory = 0;
static int		show_rfsact = 1;
static int		win_vcount;
static int		win_count;
static int		focus = -1;
static int		sel = -1;
static int	msgbox_visible;

static int		desk_w, desk_h;
static int		tb_wd;

static int		clk_x;

static pid_t		avail_pid;
static int		osk_height;
static pid_t		osk_pid;

static int	tbb_max_width;

static struct win_rect	frame_rect;
static struct win_rect	task_rect;
static struct win_rect	rsrc_rect;
static struct win_rect	rfsa_rect;
static struct win_rect	osk_rect;
static struct win_rect	clk_rect;

static struct bitmap *	osk_icon_s;
static struct bitmap *	osk_icon_l;
static struct bitmap *	osk_icon;

static int tbb_width(void)
{
	int w;
	
	if (!win_vcount)
		return tbb_max_width;
	
	w = task_rect.w / win_vcount;
	if (w > tbb_max_width)
		return tbb_max_width;
	return w;
}

static void make_rects(void)
{
	int tl = wm_get(WM_THIN_LINE);
	int x = desk_w - 6 * tl;
	
	win_text_size(WIN_FONT_DEFAULT, &clk_rect.w, &clk_rect.h, "00:00:00");
	clk_rect.x = x - clk_rect.w;
	clk_rect.y = 2 * tl + (tb_height - clk_rect.h - 2 * tl) / 2;
	x = clk_rect.x - 2 * tl;
	
	if (show_memory)
	{
		rsrc_rect.x = x - clk_rect.h;
		rsrc_rect.y = clk_rect.y;
		rsrc_rect.w = clk_rect.h;
		rsrc_rect.h = clk_rect.h;
		x = rsrc_rect.x - 2 * tl;
	}
	
	if (show_rfsact)
	{
		rfsa_rect.x = x - clk_rect.h;
		rfsa_rect.y = clk_rect.y;
		rfsa_rect.w = clk_rect.h;
		rfsa_rect.h = clk_rect.h;
		x = rfsa_rect.x - 2 * tl;
	}
	
	if (osk_icon || osk_icon_l)
	{
		struct bitmap *icon;
		
		osk_rect.x = x - clk_rect.h;
		osk_rect.y = clk_rect.y;
		osk_rect.w = clk_rect.h;
		osk_rect.h = clk_rect.h;
		x = osk_rect.x - 2 * tl;
		
		icon = osk_icon;
		if (!icon)
			icon = osk_icon_l;
		
		if (osk_rect.h > 11 && osk_icon_l)
			icon = osk_icon_l;
		
		if (!osk_icon_s)
		{
			osk_icon_s = bmp_scale(icon, osk_rect.w, osk_rect.h);
			if (osk_icon_s)
				bmp_conv(osk_icon_s);
		}
	}
	
	frame_rect.x = x - 2 * tl;
	frame_rect.y = 3 * tl;
	frame_rect.w = clk_rect.x + clk_rect.w - frame_rect.x + 3 * tl;
	frame_rect.h = tb_height - 5 * tl;
	x -= 2 * tl;
	
	task_rect.x = 0;
	task_rect.y = 0;
	task_rect.w = x;
	task_rect.h = tb_height;
}

static void redraw_memory(void)
{
#define CCUR	st.cpu
#define CMAX	st.cpu_max
	win_color bg = wc_get(WC_WIN_BG);
	win_color fg = wc_get(WC_WIN_FG);
	win_color f;
	int tl = wm_get(WM_THIN_LINE);
	int rh;
	struct systat st;
	
	if (_systat(&st))
		return;
	if (!CMAX)
	{
		win_rect(tb_wd, bg, rsrc_rect.x, rsrc_rect.y, rsrc_rect.w, rsrc_rect.h);
		return;
	}
	win_rgb2color(&f, 0, 255, 0);
	
	rh = (rsrc_rect.h - tl * 2) * CCUR / CMAX;
	
	win_rect(tb_wd, bg, rsrc_rect.x, rsrc_rect.y, rsrc_rect.w, rsrc_rect.h);
	win_rect(tb_wd, bg, rsrc_rect.x + tl, rsrc_rect.y, rsrc_rect.w - tl * 2, rsrc_rect.h - rh - tl);
	win_rect(tb_wd, f,  rsrc_rect.x + tl, rsrc_rect.y + rsrc_rect.h - tl - rh, rsrc_rect.w - tl * 2, rh);
	win_rect(tb_wd, fg, rsrc_rect.x, rsrc_rect.y + rsrc_rect.h - tl, rsrc_rect.w, tl);
	win_rect(tb_wd, fg, rsrc_rect.x, rsrc_rect.y, tl, rsrc_rect.h);
	win_rect(tb_wd, fg, rsrc_rect.x + rsrc_rect.w - tl, rsrc_rect.y, tl, rsrc_rect.h);
#undef CCUR
#undef CMAX
}

static void redraw_rfsact(void)
{
	win_color bg = wc_get(WC_WIN_FG);
	win_color fg = wc_get(WC_WIN_BG);
	int tl = wm_get(WM_THIN_LINE);
	
	if (_rfsactive())
		win_rgb2color(&fg, 255, 0, 0);
	
	win_rect(tb_wd, bg, rfsa_rect.x, rfsa_rect.y, rfsa_rect.w, rfsa_rect.h);
	win_rect(tb_wd, fg, rfsa_rect.x + tl, rfsa_rect.y + tl, rfsa_rect.w - 2 * tl, rfsa_rect.h - 2 * tl);
}

static void redraw_clock(void)
{
#define clk_y	3
#define clk_h	(tb_height - 5)
	win_color hi1 = wc_get(WC_HIGHLIGHT1);
	win_color sh1 = wc_get(WC_SHADOW1);
	win_color bg = wc_get(WC_WIN_BG);
	win_color fg = wc_get(WC_WIN_FG);
	struct tm *tm;
	time_t t;
	char str[9];
	int tw, th;
	int tx, ty;
	int clk_w;
	int tl;
	
	time(&t);
	tm = localtime(&t);
	
	sprintf(str, "%2i:%02i:%02i", tm->tm_hour, tm->tm_min, tm->tm_sec);
	win_text_size(WIN_FONT_DEFAULT, &tw, &th, str);
	
	ty = (clk_h - th) / 2;
	tx = ty;
	clk_w = tw + ty * 2;
	clk_x = desk_w - clk_w - 2;
	if (show_memory)
	{
		clk_x -= clk_h;
		clk_w += clk_h;
		tx += clk_h;
	}
	
	tl = wm_get(WM_THIN_LINE);
	
	win_rect(tb_wd, bg, frame_rect.x, frame_rect.y, frame_rect.w, frame_rect.h);
	win_rect(tb_wd, sh1, frame_rect.x, frame_rect.y, frame_rect.w, tl);
	win_rect(tb_wd, sh1, frame_rect.x, frame_rect.y, tl, frame_rect.h);
	win_rect(tb_wd, hi1, frame_rect.x, frame_rect.y + frame_rect.h - tl, frame_rect.w, tl);
	win_rect(tb_wd, hi1, frame_rect.x + frame_rect.w - tl, frame_rect.y, tl, frame_rect.h);
	
	win_text(tb_wd, fg, clk_rect.x, clk_rect.y, str);
	
	if (show_memory)
		redraw_memory();
	if (show_rfsact)
		redraw_rfsact();
	if (osk_icon_s)
		bmp_draw(tb_wd, osk_icon_s, osk_rect.x, osk_rect.y);
#undef clk_y
#undef clk_h
}

static void redraw_button(struct win_data *wd, int x, int y, int w, int h, int sel)
{
	win_color s_bg = wc_get(WC_SEL_BG);
	win_color s_fg = wc_get(WC_SEL_FG);
	win_color hi1 = wc_get(WC_BUTTON_HI1);
	win_color sh1 = wc_get(WC_BUTTON_SH1);
	win_color hi2 = wc_get(WC_BUTTON_HI2);
	win_color sh2 = wc_get(WC_BUTTON_SH2);
	win_color wbg = wc_get(WC_WIN_BG);
	win_color bg = wc_get(WC_BUTTON_BG);
	win_color fg = wc_get(WC_BUTTON_FG);
	char title[WIN_TITLE_MAX + 1];
	int tw, th;
	int ty;
	int tl;
	
	tl = wm_get(WM_THIN_LINE);
	
	win_text_size(WIN_FONT_DEFAULT, &tw, &th, wd->title);
	ty = (h - th) / 2;
	
	strcpy(title, wd->title);
	if (ty + tw >= w - ty)
	{
		int l = strlen(title);
		
		while (l > 0 && ty + tw >= tbb_width() - ty)
		{
			title[--l] = 0;
			win_text_size(WIN_FONT_DEFAULT, &tw, &th, title);
		}
	}
	
	if (wbg != bg)
		win_rect(tb_wd, bg, x, y, w, h);
	
	if (wd - win_data == focus)
	{
		win_rect(tb_wd, sh1, x, y, w, tl);
		win_rect(tb_wd, sh1, x, y, tl, h);
		win_rect(tb_wd, hi1, x, y + h - tl, w, tl);
		win_rect(tb_wd, hi1, x + w - tl, y, tl, h);
		win_rect(tb_wd, sh2, x + tl, y + tl, w - 2 * tl, tl);
		win_rect(tb_wd, sh2, x + tl, y + tl, tl, h - 2 * tl);
		win_rect(tb_wd, hi2, x + tl, y + h - 2 * tl, w - 2 * tl, tl);
		win_rect(tb_wd, hi2, x + w - 2 * tl, y + tl, tl, h - 2 * tl);
		ty++;
	}
	else
	{
		win_rect(tb_wd, hi1, x, y, w, tl);
		win_rect(tb_wd, hi1, x, y, tl, h);
		win_rect(tb_wd, sh1, x, y + h - tl, w, tl);
		win_rect(tb_wd, sh1, x + w - tl, y, tl, h);
		win_rect(tb_wd, hi2, x + tl, y + tl, w - 2 * tl, tl);
		win_rect(tb_wd, hi2, x + tl, y + tl, tl, h - 2 * tl);
		win_rect(tb_wd, sh2, x + tl, y + h - 2 * tl, w - 2 * tl, tl);
		win_rect(tb_wd, sh2, x + w - 2 * tl, y + tl, tl, h - 2 * tl);
	}
	
	if (sel && focus == tb_wd)
		win_btext(tb_wd, s_bg, s_fg, x + ty, y + ty, title);
	else
		win_text(tb_wd, fg, x + ty, y + ty, title);
}

static void redraw(int set_clip)
{
	win_color hi1 = wc_get(WC_HIGHLIGHT1);
	win_color hi2 = wc_get(WC_HIGHLIGHT2);
	win_color bg = wc_get(WC_WIN_BG);
	int x = 0;
	int vi;
	int tl;
	int i;
	
	tl = wm_get(WM_THIN_LINE);
	
	if (set_clip)
		win_clip(tb_wd, 0, 0, desk_w, tb_height, 0, 0);
	win_paint();
	win_rect(tb_wd, hi1, 0,  0, desk_w, tl);
	win_rect(tb_wd, hi2, 0, tl, desk_w, tl);
	win_rect(tb_wd, bg, 0, 2 * tl, desk_w, tb_height - 2 * tl);
	redraw_clock();
	for (vi = 0, i = 0; i < win_count; i++)
	{
		if (!win_list[i]->valid)
			abort();
		if (win_list[i]->visible)
		{
			redraw_button(win_list[i], x + tl, 3 * tl, tbb_width() - 4 * tl, tb_height - 5 * tl, vi == sel);
			x += tbb_width();
			vi++;
		}
	}
	win_end_paint();
}

static void on_creat(int wd)
{
#if DEBUG
	_cprintf("on_creat: wd = %i\n", wd);
#endif
	
	if (win_data[wd].valid)
		return;
	memset(&win_data[wd], 0, sizeof win_data[wd]);
	win_data[wd].valid = 1;
	win_list[win_count++] = &win_data[wd];
}

static void on_close(int wd)
{
	int i;
	
#if DEBUG
	_cprintf("on_close: wd = %i\n", wd);
#endif
	if (!win_data[wd].valid)
		return;
	if (focus == wd)
		focus = -1;
	sel = -1;
	
	for (i = 0; i < win_count; i++)
		if (win_list[i] == &win_data[wd])
		{
			if (win_data[wd].visible)
			{
#if DEBUG
				_cprintf("on_close: win_vcount--\n");
#endif
				win_vcount--;
			}
			win_count--;
			memmove(&win_list[i], &win_list[i + 1], (win_count - i) * sizeof *win_list);
			redraw(1);
		}
	memset(&win_data[wd], 0x55, sizeof win_data[wd]);
	win_data[wd].valid = 0;
}

static void on_title(int wd)
{
	char title[WIN_TITLE_MAX + 1];
	int v = 1;
	
#if DEBUG
	_cprintf("on_title: wd = %i\n", wd);
#endif
	if (!win_data[wd].valid)
		on_creat(wd);
	if (win_get_title(wd, title, sizeof title))
		return;
	if (!*title)
		v = 0;
#if DEBUG
	_cprintf("on_title:  \"%s\" -> \"%s\"\n", win_data[wd].title, title);
#endif
	
	if (win_data[wd].visible != v)
	{
		if (v)
		{
#if DEBUG
			_cprintf("on_title: win_vcount++\n");
#endif
			win_vcount++;
		}
		else
		{
#if DEBUG
			_cprintf("on_title: win_vcount--\n");
#endif
			win_vcount--;
		}
	}
	
	strcpy(win_data[wd].title, title);
	win_data[wd].visible = v;
	redraw(1);
}

static void on_switch(int wd)
{
	focus = wd;
	redraw(1);
}

static void on_key(unsigned ch, unsigned shift)
{
	int i, n;
	int wd;
	
	switch (ch)
	{
	case WIN_KEY_LEFT:
		if (!sel)
			break;
		sel--;
		redraw(1);
		break;
	case WIN_KEY_RIGHT:
		if (sel >= win_vcount - 1)
			break;
		sel++;
		redraw(1);
		break;
	case '\n':
		n = sel;
		for (i = 0; i < win_count; i++)
			if (win_list[i]->visible && !n--)
			{
				wd = win_list[i] - win_data;
				win_raise(wd);
				win_focus(wd);
				redraw(1);
				break;
			}
		break;
	case '\t':
		if (!win_vcount)
			break;
		sel++;
		sel %= win_vcount;
		n = sel;
		for (i = 0; i < win_count; i++)
			if (win_list[i]->visible && !n--)
			{
				wd = win_list[i] - win_data;
				win_raise(wd);
				win_focus(wd);
				redraw(1);
				break;
			}
		break;
	case 0x1b:
		win_raise(tb_wd);
		win_focus(tb_wd);
		redraw(1);
		if (sel < 0)
		{
			sel = 0;
			break;
		}
		break;
	}
}

static void toggle_avail(void)
{
	if (avail_pid > 0)
	{
		kill(avail_pid, SIGKILL);
		avail_pid = -1;
		return;
	}
	avail_pid = _newtaskl(_PATH_B_WAVAIL, _PATH_B_WAVAIL, "--taskbar", NULL);
}

static void toggle_osk(void)
{
	struct win_rect wsr;
	char buf[80];
	int fd[2] = { -1, -1 };
	int sout = -1;
	FILE *f;
	
	if (osk_pid > 0)
	{
		kill(osk_pid, SIGKILL);
		return;
	}
	
	sout = dup(1);
	if (sout < 0)
	{
		msgbox_perror(NULL, "Taskbar", "Cannot dup stdout", errno);
		goto clean;
	}
	if (pipe(fd))
	{
		msgbox_perror(NULL, "Taskbar", "Cannot create pipe pair", errno);
		goto clean;
	}
	dup2(fd[1], 1);
	osk_pid = _newtaskl(_PATH_B_OSK, _PATH_B_OSK, "--taskbar", NULL);
	dup2(sout, 1);
	close(fd[1]);
	fd[1] = -1;
	
	f = fdopen(fd[0], "r");
	if (!f)
	{
		msgbox_perror(NULL, "Taskbar", "Cannot fdopen pipe", errno);
		goto clean;
	}
	if (!fgets(buf, sizeof buf, f))
	{
		if (ferror(f))
			msgbox_perror(NULL, "Taskbar", "Cannot read from pipe", errno);
		else
			msgbox(NULL, "Taskbar", "Cannot read from pipe.");
		fclose(f);
		goto clean;
	}
	fclose(f);
	
	osk_height = atoi(buf);
	if (osk_height)
	{
		win_ws_getrect(&wsr);
		wsr.h -= osk_height;
		win_ws_setrect(&wsr);
	}
clean:
	if (fd[0] >= 0)
		close(fd[0]);
	if (fd[1] >= 0)
		close(fd[1]);
	if (sout >= 0)
		close(sout);
}

static void ptr_down(int x, int y)
{
	int i, n;
	
	if (x >= rsrc_rect.x && x < rsrc_rect.x + rsrc_rect.w &&
	    y >= rsrc_rect.y && y < rsrc_rect.y + rsrc_rect.h)
	{
		toggle_avail();
		return;
	}
	
	if (x >= rfsa_rect.x && x < rfsa_rect.x + rfsa_rect.w &&
	    y >= rfsa_rect.y && y < rfsa_rect.y + rfsa_rect.h &&
	    !msgbox_visible)
	{
		msgbox_visible = 1;
		msgbox(NULL, "Removable Disks", "Do not remove floppy disks from the drives\nwhen this indicator is red.");
		msgbox_visible = 0;
		return;
	}
	
	if (x >= osk_rect.x && x < osk_rect.x + osk_rect.w &&
	    y >= osk_rect.y && y < osk_rect.y + osk_rect.h)
	{
		toggle_osk();
		return;
	}
	
	if (!win_vcount)
		return;
	
	i = x / tbb_width();
	for (n = 0; n < win_count; n++)
		if (win_list[n]->visible && !i--)
		{
			int wd = win_list[n] - win_data;
			
			win_raise(wd);
			win_focus(wd);
			return;
		}
}

static void tb_proc(struct event *e)
{
	char title[WIN_TITLE_MAX + 1];
	
	if (e->type != E_WINGUI)
		return;
	
	switch (e->win.type)
	{
	case WIN_E_CREAT:
		on_creat(e->win.wd);
		break;
	case WIN_E_CLOSE:
		on_close(e->win.wd);
		break;
	case WIN_E_TITLE:
	case WIN_E_CHANGE:
		if (win_get_title(e->win.wd, title, sizeof title))
			break;
		on_title(e->win.wd);
		break;
	case WIN_E_SWITCH_TO:
		on_switch(e->win.wd);
		break;
	case WIN_E_KEY_DOWN:
		on_key(e->win.ch, e->win.shift);
		break;
	}
	
	if (e->win.wd == tb_wd)
		switch (e->win.type)
		{
		case WIN_E_REDRAW:
			win_clip(tb_wd, e->win.redraw_x, e->win.redraw_y,
					e->win.redraw_w, e->win.redraw_h,
					0, 0);
			redraw(0);
			break;
		case WIN_E_PTR_DOWN:
			ptr_down(e->win.ptr_x, e->win.ptr_y);
		break;
		}
}

static void tmr_proc(void *data)
{
	win_clip(tb_wd, 0, 0, desk_w, tb_height, 0, 0);
	win_paint();
	redraw_clock();
	win_end_paint();
}

static void scan(void)
{
	int junk;
	int i;
	
	for (i = 0; i < WIN_MAX; i++)
		if (!win_is_visible(i, &junk))
		{
			on_creat(i);
			on_title(i);
		}
}

static void on_setmode(void)
{
	struct win_rect r;
	
	if (osk_icon_s)
		bmp_conv(osk_icon_s);
	
	win_desktop_size(&desk_w, &desk_h);
	
	win_change(tb_wd, 1, 0, desk_h - tb_height, desk_w, tb_height);
	make_rects();
	redraw(1);
	
	r.x = 0;
	r.y = 0;
	r.w = desk_w;
	r.h = desk_h - tb_height;
	win_ws_setrect(&r);
}

static void sig_chld(int nr)
{
	struct win_rect wsr;
	pid_t pid;
	
	while (pid = waitpid(-1, NULL, WNOHANG), pid > 0)
		if (pid == avail_pid)
			avail_pid = -1;
		else if (pid == osk_pid)
		{
			if (osk_height)
			{
				win_ws_getrect(&wsr);
				wsr.h += osk_height;
				win_ws_setrect(&wsr);
				osk_height = 0;
			}
			osk_pid = -1;
		}
	evt_signal(SIGCHLD, sig_chld);
}

int main()
{
	struct win_rect r;
	struct timeval tv;
	struct systat st;
	int w, h;
	int tl;
	int i;
	
	if (getuid() != geteuid())
		clearenv();
	
	for (i = 1; i <= NSIG; i++)
		signal(i, SIG_DFL);
	evt_signal(SIGCHLD, sig_chld);
	
	if (!_systat(&st))
		show_memory = 1;
	
	if (win_attach())
		err(255, NULL);
	
	win_text_size(WIN_FONT_DEFAULT, &w, &h, "X");
	win_desktop_size(&desk_w, &desk_h);
	win_creat(&tb_wd, 0, 0, 0, 0, 0, tb_proc, NULL);
	win_setlayer(tb_wd, WIN_LAYER_TASKBAR);
	win_raise(tb_wd);
	scan();
	
	tl = wm_get(WM_THIN_LINE);
	
	tbb_max_width = w * 23;
	tb_height = h + 13 * tl;
	
	if (win_taskbar(tb_proc))
	{
		msgbox_perror(NULL, "TaskBar", "Unable to register taskbar", errno);
		return 1;
	}
	
	win_on_setmode(on_setmode);
	
	tv.tv_usec = 0;
	tv.tv_sec = 1;
	
	tmr_creat(&tv, &tv, tmr_proc, NULL, 1);
	
	win_change(tb_wd, 1, 0, desk_h - tb_height, desk_w, tb_height);
	make_rects();
	
	r.x = 0;
	r.y = 0;
	r.w = desk_w;
	r.h = desk_h - tb_height;
	win_ws_setrect(&r);
	win_update();
	
	osk_icon = bmp_load(OSK_ICON);
	if (!osk_icon)
		msgbox_perror(NULL, "Taskbar", "Cannot load " OSK_ICON, errno);
	
	osk_icon_l = bmp_load(OSK_ICON64);
	if (!osk_icon_l)
		msgbox_perror(NULL, "Taskbar", "Cannot load " OSK_ICON64, errno);
	
	make_rects();
	
	signal(SIGTERM, SIG_IGN);
	
	for (;;)
		evt_wait();
}
