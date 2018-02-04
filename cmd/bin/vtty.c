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

#define _LIB_INTERNALS

#include <priv/copyright.h>
#include <wingui_metrics.h>
#include <wingui_cgadget.h>
#include <wingui_msgbox.h>
#include <wingui_color.h>
#include <wingui_form.h>
#include <wingui_menu.h>
#include <wingui_dlg.h>
#include <wingui.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/poll.h>
#include <termios.h>
#include <newtask.h>
#include <stdlib.h>
#include <string.h>
#include <confdb.h>
#include <limits.h>
#include <unistd.h>
#include <os386.h>
#include <ctype.h>
#include <event.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <timer.h>
#include <paths.h>
#include <time.h>
#include <err.h>

#define COLORS_FORM		"/lib/forms/vtty_color.frm"
#define FONT_FORM		"/lib/forms/vtty_font.frm"

#define DEFAULT_WIDTH		80
#define DEFAULT_HEIGHT		25

#define PTM_FD			3

#define BG_DEFAULT		16
#define FG_DEFAULT		17
#define CURSOR			18

#define MIN_COLUMNS		20
#define MIN_LINES		5

static const struct timeval cur_rate = { .tv_sec = 0, .tv_usec = 333333 };

static const char *title = "Command Line";

static struct form *main_form;
static struct gadget *screen;
static int form_x = -1;
static int form_y = -1;

static int save_config = 1;
static int restricted;
static int backdrop;
static int bigH;
static int bigM;
static int term;

static int *line_changed;
static int full_redraw;

static int font_w;
static int font_h;

static int cur_state = 1;
static int cur_fg = FG_DEFAULT;
static int cur_bg = BG_DEFAULT;
static int cur_x;
static int cur_y;

static int nr_col;
static int nr_lin;

static struct termios tio;

static struct
{
	struct form_state fst;
	int auto_close;
	int nr_col;
	int nr_lin;
	int ftd;
} config;

static struct win_rgba color_table[19] =
{
	{ 0,	0,	0,	255 },
	{ 192,	0,	0,	255 },
	{ 0,	192,	0,	255 },
	{ 192,	96,	0,	255 },
	{ 0,	0,	192,	255 },
	{ 192,	0,	192,	255 },
	{ 0,	192,	192,	255 },
	{ 192,	192,	192,	255 },

	{ 128,	128,	128,	255 },
	{ 255,	128,	128,	255 },
	{ 128,	255,	128,	255 },
	{ 255,	255,	0,	255 },
	{ 128,	128,	255,	255 },
	{ 255,	128,	255,	255 },
	{ 128,	255,	255,	255 },
	{ 255,	255,	255,	255 },

	{ 0,	0,	0,	255 },
	{ 192,	192,	192,	255 },
	{ 255,	255,	255,	255 },
};

static struct cell
{
	unsigned char fg;
	unsigned char bg;
	char ch;
} *screen_buf;

static void	resize_tty(int ncol, int nlin);
static void	redraw_chr(int x, int y, int set_clip);
static void	redraw_line(int line_nr, int set_clip);
static void	redraw(struct gadget *g, int wd);
static void	term_msgbox(void);
static void	key_down(struct gadget *g, unsigned ch, unsigned shift);
static void	on_resize(struct form *f, int w, int h);
static int	on_close(struct form *f);
static void	scroll(void);
static int	print_cs(char ch);
static void	update(void);
static void	print(const char *str, int len);
static char *	ptsname(int fd);
static int	unlockpt(int fd);
static void	cur_cb(void *data);
static void	sig_chld(int nr);
static char **	parse_args(int argc, char **argv);
static void	colors_click(struct menu_item *mi);
static void	font_click(struct menu_item *mi);
static void	about_click(struct menu_item *mi);
static void	load_colors();
static void	upstat(void);

static char *	canhist[100];
static int	canhistpos = -1;
static char	canbuf[MAX_CANON + 1];
static int	canpos;
static int	canlen;
static int	canmode;

static void update_pty_size(void)
{
	struct pty_size psz;
	
	psz.w = nr_col;
	psz.h = nr_lin;
	
	ioctl(PTM_FD, PTY_SSIZE, &psz);
}

static void resize_tty(int ncol, int nlin)
{
	struct cell *new_buf;
	char msg[256];
	int xm, ym;
	int x, y;
	int sv;
	int i;
	
	if (ncol < MIN_COLUMNS)
		ncol = MIN_COLUMNS;
	if (nlin < MIN_LINES)
		nlin = MIN_LINES;
	if (ncol > 1024)
		ncol = 1024;
	if (nlin > 1024)
		nlin = 1024;
	
	if (ncol != nr_col || nlin != nr_lin)
	{
		line_changed = realloc(line_changed, nlin * sizeof *line_changed);
		new_buf = calloc(ncol * nlin, sizeof *screen_buf);
		if (!new_buf || !line_changed)
		{
			sprintf(msg, "vtty: resize_tty: %m");
			msgbox(main_form, "vtty", msg);
			exit(errno);
		}
		
		if (ncol <= nr_col)
			xm = ncol;
		else
			xm = nr_col;
		
		if (nlin <= nr_lin)
			ym = nlin;
		else
			ym = nr_lin;
		
		for (i = 0; i < ncol * nlin; i++)
		{
			new_buf[i].fg = cur_fg;
			new_buf[i].bg = cur_bg;
			new_buf[i].ch = ' ';
		}
		for (y = 0; y < ym; y++)
			for (x = 0; x < xm; x++)
				new_buf[y * ncol + x] = screen_buf[y * nr_col + x];
		screen_buf = new_buf;
		memset(line_changed, 0, nlin * sizeof *line_changed);
		
		nr_col = ncol;
		nr_lin = nlin;
		/* cur_x = 0;
		cur_y = 0; */
		if (cur_x > nr_col)
			cur_x = nr_col;
		if (cur_y >= nr_lin)
			cur_y = nr_lin - 1;
	}
	
	sv = main_form->visible;
	if (sv)
		form_hide(main_form);
	
	form_resize(main_form, ncol * font_w, nlin * font_h);
	gadget_resize(screen, ncol * font_w, nlin * font_h);
	
	if (sv)
		form_show(main_form);
	
	update_pty_size();
}

static void do_draw_chr(int x, int y, unsigned c, int bgi, int fgi, int cursor)
{
	win_color cfg;
	win_color bg;
	win_color fg;
	int wd;
	int tl;
	
	tl = wm_get(WM_THIN_LINE);
	wd = screen->form->wd;
	
	if (c < 32)
	{
		int tmp;
		
		tmp = bgi;
		bgi = fgi;
		fgi = tmp;
		c  += '@';
	}
	
	win_rgb2color(&cfg, color_table[CURSOR].r, color_table[CURSOR].g, color_table[CURSOR].b);
	win_rgb2color(&fg, color_table[fgi].r, color_table[fgi].g, color_table[fgi].b);
	win_rgb2color(&bg, color_table[bgi].r, color_table[bgi].g, color_table[bgi].b);
	
	win_bchr(wd, bg, fg, font_w * x, font_h * y, c);
	if (cursor)
		win_rect(wd, cfg, font_w * x, font_h * y + font_h - 2 * tl, font_w, 2 * tl);
}

static void redraw_chr(int x, int y, int set_clip)
{
	int wd = screen->form->wd;
	struct cell *cl;
	int cur = 0;
	int cx;
	
	if (x < 0 || x >= nr_col)
		return;
	if (y < 0 || y >= nr_lin)
		return;
	
	cl = &screen_buf[x + y * nr_col];
	
	if (set_clip)
	{
		win_clip(wd, screen->rect.x, screen->rect.y, screen->rect.w, screen->rect.h, screen->rect.x, screen->rect.y);
		win_set_font(wd, config.ftd);
		win_paint();
	}
	
	cx = cur_x;
	if (canmode)
		cx += canpos;
	
	if (cur_state && cx == x && cur_y == y)
		cur = 1;
	
	if (canmode && cur_y == y && x >= cur_x && x < cur_x + canlen)
		do_draw_chr(x, y, canbuf[x - cur_x], BG_DEFAULT, FG_DEFAULT, cur);
	else
		do_draw_chr(x, y, cl->ch, cl->bg, cl->fg, cur);
	
	if (set_clip)
		win_end_paint();
}

static void redraw_line(int line_nr, int set_clip)
{
	int wd;
	int i;
	
	if (set_clip)
	{
		wd = screen->form->wd;
		win_clip(wd, screen->rect.x, screen->rect.y, screen->rect.w, screen->rect.h, screen->rect.x, screen->rect.y);
		win_set_font(wd, config.ftd);
		win_paint();
	}
	
	for (i = 0; i < nr_col; i++)
		redraw_chr(i, line_nr, 0);
	
	if (set_clip)
		win_end_paint();
}

static void redraw(struct gadget *g, int wd)
{
	int i;
	
	win_set_font(wd, config.ftd);
	for (i = 0; i < nr_lin; i++)
		redraw_line(i, 0);
}

static void drop(struct gadget *g, const void *data, size_t len, int x, int y, unsigned shift)
{
	char buf[len + 1];
	char *sp, *dp, *ep;
	
	memcpy(buf, data, len);
	buf[len] = 0;
	
	ep = buf + len;
	sp = buf;
	dp = buf;
	
	while (sp < ep)
		if (*sp)
			*dp++ = *sp++;
		else
			sp++;
	
	write(PTM_FD, data, dp - buf);
}

static void term_msgbox(void)
{
	msgbox(main_form, "Terminated", "This application is no longer active.\n\n"
					"Any input will be discarded.\n\n"
					"Click the close button in the window title bar,\n"
					"or press ESC to close this window.");
}

static int iscc(unsigned ch)
{
	int i;
	
	if (!tcgetattr(PTM_FD, &tio))
		for (i = 0; i < NCCS; i++)
			if (tio.c_cc[i] == ch)
				return 1;
	
	if (ch == '\n')
		return 1;
	return 0;
}

static void canrecall(void)
{
	char *s = NULL;
	int l;
	
	if (canhistpos >= 0 && canhist[canhistpos] != NULL)
		s = canhist[canhistpos];
	
	if (s == NULL)
	{
		canpos = canlen = 0;
		return;
	}
	
	l = strlen(s);
	
	memcpy(canbuf, s, l);
	canpos = canlen = l;
}

static void cansave(void)
{
	char *s;
	
	if (!canlen)
		return;
	
	s = malloc(canlen + 1);
	if (s == NULL)
		return;
	
	memcpy(s, canbuf, canlen);
	s[canlen] = 0;
	
	free(canhist[sizeof canhist / sizeof *canhist - 1]);
	memmove(canhist + 1, canhist, sizeof canhist - sizeof *canhist);
	canhist[0] = s;
}

static void caninput(unsigned ch)
{
	int u = 0;
	int i;
	
	switch (ch)
	{
	case WIN_KEY_UP:
		if (canhist[canhistpos + 1] == NULL)
			break;
		canhistpos++;
		canrecall();
		u = 1;
		break;
	case WIN_KEY_DOWN:
		if (canhistpos < 0)
			break;
		canhistpos--;
		canrecall();
		u = 1;
		break;
	case WIN_KEY_LEFT:
		if (!canpos)
			break;
		canpos--;
		u = 1;
		break;
	case WIN_KEY_RIGHT:
		if (canpos >= canlen)
			break;
		canpos++;
		u = 1;
		break;
	case WIN_KEY_HOME:
		canpos = 0;
		u = 1;
		break;
	case WIN_KEY_END:
		canpos = canlen;
		u = 1;
		break;
	case WIN_KEY_DEL:
		if (canpos >= canlen)
			break;
		
		memmove(canbuf + canpos, canbuf + canpos + 1, canlen - canpos);
		canlen--;
		u = 1;
		break;
	default:
		if (ch == tio.c_cc[VWERASE])
		{
			for (i = canpos - 1; i > 0 && !isspace(canbuf[i]); i--)
				;
			
			if (i < 0)
				break;
			
			memmove(canbuf + i, canbuf + canpos, canlen - canpos);
			canlen -= canpos - i;
			canpos  = i;
			u = 1;
			break;
		}
		
		if (ch == tio.c_cc[VERASE])
		{
			if (!canpos)
				break;
			
			memmove(canbuf + canpos - 1, canbuf + canpos, canlen - canpos);
			canpos--;
			canlen--;
			u = 1;
			break;
		}
		
		if (ch == tio.c_cc[VKILL])
		{
			canpos = canlen = 0;
			u = 1;
			break;
		}
		
		if (iscc(ch))
		{
			char c = ch;
			
			if (ch == '\n')
				cansave();
			
			write(PTM_FD, canbuf, canlen);
			write(PTM_FD, &c, 1);
			
			canpos = canlen = 0;
			canhistpos = -1;
			u = 1;
			break;
		}
		
		if (canlen >= MAX_CANON)
			break;
		if (ch >= 256)
			break;
		memmove(canbuf + canpos + 1, canbuf + canpos, canlen - canpos);
		canbuf[canpos] = ch;
		canpos++;
		canlen++;
		u = 1;
		break;
	}
	
	if (u)
		line_changed[cur_y] = 1;
}

static void key_down(struct gadget *g, unsigned ch, unsigned shift)
{
	char c = ch;
	
	if (term)
	{
		if (ch == 27)
			on_close(main_form);
		if (isprint(ch))
			term_msgbox();
		return;
	}
	
	upstat();
	
	if (canmode)
	{
		caninput(ch);
		return;
	}
	
	if (ch == '\\' && (shift & WIN_SHIFT_CTRL))
		c = 28;
	if (ch == WIN_KEY_DEL)
		ch = c = 127;
	if (ch < 256)
		write(PTM_FD, &c, 1);
}

static void on_resize(struct form *f, int w, int h)
{
	w /= font_w;
	h /= font_h;
	
	resize_tty(w, h);
}

static int on_close(struct form *f)
{
	if (save_config)
	{
		form_get_state(main_form, &config.fst);
		config.nr_col = nr_col;
		config.nr_lin = nr_lin;
		c_save("vtty", &config, sizeof config);
	}
	ioctl(PTM_FD, PTY_SHUP, NULL);
	exit(0);
}

static void scroll(void)
{
	struct cell *cl;
	int i;
	
	if (cur_y)
	{
		memmove(screen_buf, screen_buf + nr_col, (nr_lin - 1) * nr_col * sizeof *screen_buf);
		cur_y--;
		
		cl = &screen_buf[(nr_lin-1) * nr_col];
		for (i = 0; i < nr_col; i++, cl++)
		{
			cl->ch = ' ';
			cl->fg = cur_fg;
			cl->bg = cur_bg;
		}
		full_redraw = 1;
	}
}

static int cs_arg[4];
static int cs_argc;

static int print_cs(char ch)
{
	int px, py;
	int i;
	
	if (isdigit(ch))
	{
		if (!cs_argc)
			cs_argc++;
		cs_arg[0] *= 10;
		cs_arg[0] += ch - '0';
		return 1;
	}
	
	switch (ch)
	{
	case ';':
		cs_arg[3] = cs_arg[2];
		cs_arg[2] = cs_arg[1];
		cs_arg[1] = cs_arg[0];
		cs_arg[0] = 0;
		if (cs_argc < sizeof cs_arg / sizeof *cs_arg)
			cs_argc++;
		return 1;
	case 'H':
		px = cur_x;
		py = cur_y;
		cur_x = cs_arg[0];
		cur_y = cs_arg[1];
		redraw_chr(cur_x, cur_y, 1);
		redraw_chr(px, py, 1);
		return 0;
	case 'J':
		for (i = 0; i < nr_col * nr_lin; i++)
		{
			screen_buf[i].ch = ' ';
			screen_buf[i].fg = cur_fg;
			screen_buf[i].bg = cur_bg;
		}
		full_redraw = 1;
		return 0;
	case 'm':
		for (i = 0; i < cs_argc; i++)
		{
			switch (cs_arg[i])
			{
			case 0:
				cur_fg = FG_DEFAULT;
				cur_bg = BG_DEFAULT;
				break;
			case 30 ... 37:
				cur_fg = cs_arg[i] - 30;
				break;
			case 40 ... 47:
				cur_bg = cs_arg[i] - 40;
				break;
			}
		}
		return 0;
	default:
		return 0;
	}
}

static void update(void)
{
	int i;
	
	if (full_redraw)
	{
		gadget_redraw(screen);
		full_redraw = 0;
	}
	else
	{
		for (i = 0; i < nr_lin; i++)
			if (line_changed[i])
				redraw_line(i, 1);
	}
	for (i = 0; i < nr_lin; i++)
		line_changed[i] = 0;
}

static void print(const char *str, int len)
{
	static int esc_mode;
	static int cs_mode;
	int pcy = cur_y;
	int pcx = cur_x;
	
	if (len)
		form_show(main_form);
	while (len)
	{
		if (esc_mode)
		{
			if (*str == '[')
			{
				cs_arg[0] = 0;
				cs_arg[1] = 0;
				cs_arg[2] = 0;
				cs_arg[3] = 0;
				cs_argc = 0;
				cs_mode = 1;
			}
			esc_mode = 0;
			goto next;
		}
		
		if (cs_mode)
		{
			cs_mode = print_cs(*str);
			goto next;
		}
		
		switch (*str)
		{
		case '\e':
			esc_mode = 1;
			break;
		case '\0':
		case '\a':
			break;
		case '\b':
			if (cur_x)
				cur_x--;
			break;
		case '\r':
			cur_x = 0;
			break;
		case '\n':
			cur_x = 0;
			cur_y++;
			break;
		case '\t':
			cur_x &= ~7;
			cur_x +=  8;
			break;
		default:
			if (cur_x >= nr_col)
			{
				cur_x = 0;
				cur_y++;
			}
			while (cur_y >= nr_lin)
				scroll();
			
			screen_buf[cur_y * nr_col + cur_x].fg = cur_fg;
			screen_buf[cur_y * nr_col + cur_x].bg = cur_bg;
			screen_buf[cur_y * nr_col + cur_x].ch = *str;
			line_changed[cur_y] = 1;
			cur_x++;
		}
		
next:
		while (cur_y >= nr_lin)
			scroll();
		
		str++;
		len--;
	}
	
	if (pcy != cur_y || pcx != cur_x)
	{
		line_changed[cur_y] = 1;
		line_changed[pcy] = 1;
	}
}

static char *ptsname(int fd)
{
	static char name[PATH_MAX];
	
	strcpy(name, _PATH_M_PTY "/");
	if (ioctl(fd, PTY_PTSNAME, name + sizeof _PATH_M_PTY))
		return NULL;
	return name;
}

static int unlockpt(int fd)
{
	return ioctl(fd, PTY_UNLOCK, NULL);
}

static void cur_cb(void *data)
{
	int x;
	
	x = cur_x;
	if (canmode)
		x += canpos;
	
	cur_state = !cur_state;
	if (x < nr_col && cur_y < nr_lin)
		redraw_chr(x, cur_y, 1);
}

static void sig_chld(int nr)
{
	char msg[256];
	int status;
	
	while (wait(&status) < 0);
	if (WTERMSIG(status))
	{
		sprintf(msg, "Signal %i forced the application to terminate", WTERMSIG(status));
		
		update();
		msgbox(main_form, "VTTY", msg);
	}
	
	if (config.auto_close || !main_form->visible)
		form_close(main_form);
	else
	{
		char *p;
		
		if (asprintf(&p, "%s (Terminated)", title) >= 0)
			form_set_title(main_form, p);
		term = 1;
	}
}

static char **parse_args(int argc, char **argv)
{
	int opt;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		fprintf(stderr, "Usage: vtty [OPTIONS] [--] [CMD]\n\n"
				"Run programs designed for text mode in a window.\n\n"
				"    -T STR set the window title\n"
				"    -c COL set the number of columns\n"
				"    -l LIN set the number of lines\n"
				"    -x COL set the window position\n"
				"    -y LIN set the window position\n"
				"    -b     backdrop mode\n"
				"    -w     wait after termination\n"
				"    -f     full screen mode\n"
				"    -r     do not allow for saving configuration\n\n"
				"Default command is $SHELL or " _PATH_B_DEFSHELL ".\n\n");
		exit(0);
	}
	
	while (opt = getopt(argc, argv, "HMwbc:l:x:y:frT:"), opt > 0)
		switch (opt)
		{
		case 'H':
			bigH = 1;
			break;
		case 'T':
			title = optarg;
			break;
		case 'M':
			bigM = 1;
			break;
		case 'w':
			config.auto_close = 0;
			save_config	  = 0;
			break;
		case 'b':
			backdrop    = 1;
			save_config = 0;
			break;
		case 'c':
			config.nr_col = strtoul(optarg, NULL, 0);
			save_config   = 0;
			break;
		case 'l':
			config.nr_lin = strtoul(optarg, NULL, 0);
			save_config   = 0;
			break;
		case 'x':
			form_x	    = strtoul(optarg, NULL, 0);
			save_config = 0;
			break;
		case 'y':
			form_y	    = strtoul(optarg, NULL, 0);
			save_config = 0;
			break;
		case 'f':
		{
			struct win_rect r;
			int w, h;
			
			win_chr_size(config.ftd, &w, &h, 'X');
			win_ws_getrect(&r);
			config.nr_col = r.w / w;
			config.nr_lin = r.h / h;
			backdrop      = 1;
			form_x	      = r.x;
			form_y	      = r.y;
			save_config   = 0;
			break;
		}
		case 'r':
			restricted  = 1;
			save_config = 0;
			break;
		default:
			exit(255);
	}
	return argv + optind;
}

static void colors_click(struct menu_item *mi)
{
	struct gadget *cs[16];
	struct gadget *bgcs;
	struct gadget *fgcs;
	struct gadget *cscs;
	struct form *f;
	char gnam[8];
	int i;
	
	f = form_load(COLORS_FORM);
	if (!f)
	{
		msgbox_perror(main_form, title, "Cannot load " COLORS_FORM, errno);
		return;
	}
	bgcs = gadget_find(f, "bgcs");
	fgcs = gadget_find(f, "fgcs");
	cscs = gadget_find(f, "cscs");
	for (i = 0; i < 16; i++)
	{
		sprintf(gnam, "c%i", i);
		cs[i] = gadget_find(f, gnam);
		
		colorsel_set(cs[i], &color_table[i]);
	}
	colorsel_set(bgcs, &color_table[BG_DEFAULT]);
	colorsel_set(fgcs, &color_table[FG_DEFAULT]);
	colorsel_set(cscs, &color_table[CURSOR]);
	
	form_set_dialog(main_form, f);
restart:
	switch (form_wait(f))
	{
	case 0:
	case 4:
		break;
	case 1:
		break;
	case 2:
		break;
	case 3:
		for (i = 0; i < 16; i++)
			colorsel_get(cs[i], &color_table[i]);
		colorsel_get(bgcs, &color_table[BG_DEFAULT]);
		colorsel_get(fgcs, &color_table[FG_DEFAULT]);
		colorsel_get(cscs, &color_table[CURSOR]);
		gadget_redraw(screen);
		if (!restricted && c_save("vtty-color", &color_table, sizeof color_table))
		{
			msgbox_perror(main_form, title, "Cannot save settings", errno);
			goto restart;
		}
		break;
	default:
		abort();
	}
	form_set_dialog(main_form, NULL);
	form_close(f);
}

static void font_click(struct menu_item *mi)
{
	struct gadget *chk_normal, *chk_narrow;
	struct form *f;
	
	f = form_load(FONT_FORM);
	form_set_dialog(main_form, f);
	
	chk_normal = gadget_find(f, "normal");
	chk_narrow = gadget_find(f, "narrow");
	
	chkbox_set_group(chk_normal, 1);
	chkbox_set_group(chk_narrow, 1);
	
	if (config.ftd == WIN_FONT_MONO_N)
		chkbox_set_state(chk_narrow, 1);
	else
		chkbox_set_state(chk_normal, 1);
	
	if (form_wait(f) == 1)
	{
		if (chkbox_get_state(chk_narrow))
			config.ftd = WIN_FONT_MONO_N;
		else
			config.ftd = WIN_FONT_MONO;
		
		win_chr_size(config.ftd, &font_w, &font_h, 'X');
		form_min_size(main_form, font_w * MIN_COLUMNS, font_h * MIN_LINES);
		form_resize(main_form, font_w * nr_col, font_h * nr_lin);
	}
	
	form_set_dialog(main_form, NULL);
	form_close(f);
}

static void about_click(struct menu_item *mi)
{
	dlg_about7(main_form, NULL, "Virtual TTY v1.4", SYS_PRODUCT, SYS_AUTHOR, SYS_CONTACT, "cdev.pnm");
}

static void nullio(void)
{
	int fd;
	int i;
	
	fd = open("/dev/null", O_RDWR);
	if (fd < 0)
		exit(errno);
	
	for (i = 0; i < 3; i++)
		dup2(fd, i);
	
	_ctty("/dev/null");
}

static void load_colors(void)
{
	wc_get_rgba(WC_TTY_CURSOR, &color_table[CURSOR]);
	wc_get_rgba(WC_TTY_BG, &color_table[BG_DEFAULT]);
	wc_get_rgba(WC_TTY_FG, &color_table[FG_DEFAULT]);
	
	c_load("vtty-color", &color_table, sizeof color_table);
}

static void on_update(void)
{
	load_colors();
	gadget_redraw(screen);
}

static void upstat(void)
{
	if (tcgetattr(PTM_FD, &tio))
	{
		canmode = 0;
		return;
	}
	
	if ((tio.c_lflag & (ICANON | ECHO)) == (ICANON | ECHO))
		canmode = 1;
	else
		canmode = 0;
}

int main(int argc, char **argv)
{
	struct timer *tmr;
	struct menu *mm, *m;
	struct pollfd pfd[2];
	const char *shell;
	char buf[64];
	pid_t pid;
	int c_loaded = 1;
	int form_flags;
	int cnt;
	int se;
	int i;
	
	if (win_attach())
		err(255, NULL);
	
	if (c_load("vtty", &config, sizeof config))
	{
		struct win_rect wsr;
		int cw, ch;
		int ftd;
		
		win_ws_getrect(&wsr);
		if (wsr.w > 700)
			ftd = WIN_FONT_MONO;
		else
			ftd = WIN_FONT_MONO_N;
		
		win_chr_size(ftd, &cw, &ch, 'X');
		
		config.auto_close = 1;
		config.nr_col	  = 80;
		config.nr_lin	  = 25;
		config.ftd	  = ftd;
		if ((config.nr_col + 2) * cw > wsr.w)
			config.nr_col = wsr.w / cw - 2;
		c_loaded = 0;
	}
	load_colors();
	
	argv = parse_args(argc, argv);
	evt_signal(SIGCHLD, sig_chld);
	
	for (i = 0; i < OPEN_MAX; i++)
		close(i);
	
	win_chr_size(config.ftd, &font_w, &font_h, 'X');
	
	mm = menu_creat();
	
	m = menu_creat();
	menu_newitem(m, "Colors ...", colors_click);
	menu_newitem(m, "Font ...", font_click);
	menu_newitem(m, "-", NULL);
	menu_newitem(m, "About ...", about_click);
	menu_submenu(mm, "Options", m);
	
	form_flags = FORM_APPFLAGS | FORM_NO_BACKGROUND;
	
	if (backdrop)
		main_form = form_creat(FORM_EXCLUDE_FROM_LIST | FORM_BACKDROP | FORM_NO_BACKGROUND, 0, form_x, form_y, config.nr_col * font_w, config.nr_lin * font_h, title);
	else
	{
		if (bigM)
			form_flags = FORM_FRAME | FORM_TITLE | FORM_NO_BACKGROUND | FORM_ALLOW_CLOSE | FORM_ALLOW_RESIZE;
		main_form = form_creat(form_flags, 0, form_x, form_y, config.nr_col * font_w, config.nr_lin * font_h, title);
		form_min_size(main_form, font_w * MIN_COLUMNS, font_h * MIN_LINES);
		form_set_menu(main_form, mm);
	}
	form_on_resize(main_form, on_resize);
	form_on_close(main_form, on_close);
	
	screen = gadget_creat(main_form, 0, 0, 160, 100);
	gadget_setptr(screen, WIN_PTR_TEXT);
	gadget_set_want_focus(screen, 1);
	gadget_set_key_down_cb(screen, key_down);
	gadget_set_redraw_cb(screen, redraw);
	gadget_set_drop_cb(screen, drop);
	gadget_focus(screen);
	
	if (c_loaded && form_x < 0 && form_y < 0)
		form_set_state(main_form, &config.fst);
	resize_tty(config.nr_col, config.nr_lin);
	if (!bigH)
		form_show(main_form);
	
	win_on_update(on_update);
	
	if (open(_PATH_D_PTMX, O_RDWR | O_NDELAY))
	{
		se = errno;
		msgbox_perror(main_form, "vtty", "vtty: " _PATH_D_PTMX, se);
		return se;
	}
	dup2(0, PTM_FD);
	close(0);
	unlockpt(PTM_FD);
	fcntl(PTM_FD, F_SETFD, 1);
	
	_ctty(ptsname(PTM_FD));
	if (open(_PATH_TTY, O_RDONLY) != 0)
		return errno;
	if (open(_PATH_TTY, O_WRONLY) != 1)
		return errno;
	if (open(_PATH_TTY, O_WRONLY) != 2)
		return errno;
	
	update_pty_size();
	
	shell = getenv("SHELL");
	if (shell == NULL)
		shell = _PATH_B_DEFSHELL;
	
	if (*argv)
		pid = _newtaskv(*argv, argv);
	else
		pid = _newtaskl(shell, shell, NULL);
	if (pid < 0)
	{
		se = errno;
		msgbox_perror(main_form, "vtty", "Cannot create task", se);
		return se;
	}
	
	tmr = tmr_creat(&cur_rate, &cur_rate, cur_cb, NULL, 1);
	if (!tmr)
	{
		se = errno;
		msgbox_perror(main_form, "vtty", "Cannot create timer", se);
		return se;
	}
	
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	
	pfd[0].fd = PTM_FD;
	pfd[0].events = POLLIN;
	
	nullio();
	
	win_idle();
	for (;;)
	{
		if (poll(pfd, 1, -1) >= 0 && (pfd[0].revents & POLLIN))
			while (cnt = read(PTM_FD, buf, sizeof(buf)), cnt > 0)
				print(buf, cnt);
		win_idle();
		update();
		upstat();
	}
}
