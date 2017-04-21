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
#include <wingui_form.h>
#include <wingui_menu.h>
#include <wingui_dlg.h>
#include <wingui.h>
#include <confdb.h>
#include <sys/poll.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <fcntl.h>
#include <event.h>
#include <ctype.h>
#include <errno.h>
#include <comm.h>
#include <err.h>

#define SELECT_FORM	"/lib/forms/select.frm"
#define COMM_FORM	"/lib/forms/comm.frm"
#define KEYB_FORM	"/lib/forms/tc_keyb.frm"
#define LOGS_FORM	"/lib/forms/tc_logs.frm"

struct form *main_form;
struct gadget *display;

int nr_columns;
int nr_lines;
int cur_x;
int cur_y;
char *cells;

int *ln_dirty;

int out_log_fd = -1;
int in_log_fd = -1;
int comm_fd = -1;

int cell_width, cell_height;

struct config
{
	struct comm_config comm_config;
	char	port_name[PATH_MAX];
	char	ret[2];
	int	local_echo;
	char	out_log_path[PATH_MAX];
	char	in_log_path[PATH_MAX];
	int	out_log;
	int	in_log;
	int	nr_columns;
	int	nr_lines;
} config;

void display_redraw(struct gadget *g, int wd);
void clear(void);
void scroll(void);
void print(char *buf, int len);

int  select_port(void);
void update_title(void);
int  setup(void);
void set_return(struct menu_item *mi);
void set_logs(struct menu_item *mi);

void comm_event(struct event *e);
static int reopen_port(void);
void init_port(void);

void send(char ch);
void key_down(struct gadget *g, unsigned ch, unsigned shift);

void select_port_click(struct menu_item *mi);
void setup_click(struct menu_item *mi);
int  on_close(struct form *f);
void about(struct menu_item *mi);
void init_gui(void);
void open_logs(void);

void display_redraw_line(struct gadget *g, int wd, int y)
{
	win_color bg;
	win_color fg;
	char *p;
	int x;
	
	win_rgb2color(&bg, 255, 255, 255);
	win_rgb2color(&fg, 0, 0, 0);
	
	win_set_font(wd, WIN_FONT_MONO);
	for (p = cells + y * nr_columns, x = 0; x < nr_columns; x++, p++)
	{
		if (cur_y == y && cur_x == x)
			win_bchr(wd, fg, bg, x * cell_width, y * cell_height, *p);
		else
			win_bchr(wd, bg, fg, x * cell_width, y * cell_height, *p);
	}
	ln_dirty[y] = 0;
}

void display_redraw(struct gadget *g, int wd)
{
	win_color bg;
	win_color fg;
	char *p;
	int y;
	
	win_rgb2color(&bg, 255, 255, 255);
	win_rgb2color(&fg, 0, 0, 0);
	
	win_set_font(wd, WIN_FONT_MONO);
	for (p = cells, y = 0; y < nr_lines; y++)
		display_redraw_line(g, wd, y);
}

void clear(void)
{
	memset(cells, ' ', nr_columns * nr_lines);
	cur_x = 0;
	cur_y = 0;
}

void scroll(void)
{
	if (!cur_y)
		return;
	memmove(cells, cells + nr_columns, (nr_lines - 1) * nr_columns);
	memset(cells + (nr_lines - 1) * nr_columns, ' ', nr_columns);
	memset(ln_dirty, 255, nr_lines * sizeof *ln_dirty);
	cur_y--;
}

void print(char *buf, int len)
{
	char *p = buf;
	int i = len;
	char ch;
	
	while (i--)
	{
		ch = *(p++);
		switch (ch)
		{
			case '\a':
				break;
			case '\n':
				if (cur_y < nr_lines)
					ln_dirty[cur_y] = 1;
				cur_y++;
				if (cur_y < nr_lines)
					ln_dirty[cur_y] = 1;
				while (cur_y >= nr_lines)
					scroll();
			case '\r':
				if (cur_y < nr_lines)
					ln_dirty[cur_y] = 1;
				cur_x = 0;
				break;
			case '\b':
				if (cur_x)
				{
					if (cur_y < nr_lines)
						ln_dirty[cur_y] = 1;
					cur_x--;
				}
				break;
			default:
				while (cur_y >= nr_lines)
					scroll();
				if (isprint(ch))
					cells[cur_y * nr_columns + cur_x] = ch;
				else
					cells[cur_y * nr_columns + cur_x] = '.';
				if (cur_y < nr_lines)
					ln_dirty[cur_y] = 1;
				cur_x++;
				if (cur_x >= nr_columns)
				{
					cur_x = 0;
					cur_y++;
					if (cur_y < nr_lines)
						ln_dirty[cur_y] = 1;
				}
		}
	}
	win_clip(display->form->wd, display->rect.x, display->rect.y, display->rect.w, display->rect.h,
				    display->rect.x, display->rect.y);
	win_paint();
	for (i = 0; i < nr_lines; i++)
		if (ln_dirty[i])
			display_redraw_line(display, display->form->wd, i);
	win_end_paint();
}

int select_port(void)
{
	struct gadget *li;
	struct form *f;
	char **comms;
	struct dirent *de;
	DIR *d;
	int comm_cnt;
	int res;
	int i;
	
	f  = form_load(SELECT_FORM);
	li = gadget_find(f, "list");
	
	comm_cnt = 0;
	comms	 = NULL;
	d	 = opendir("/dev");
	while (de = readdir(d), de)
	{
		char pathname[strlen(de->d_name) + 6];
		int fd;
		
		strcpy(pathname, "/dev/");
		strcat(pathname, de->d_name);
		fd  = open(pathname, O_RDWR);
		res = is_comm(fd);
		close(fd);
		if (!res)
			continue;
		
		comms = realloc(comms, (comm_cnt + 1) * sizeof(char *));
		if (!comms)
		{
			msgbox(main_form, "TinyComm", "Out of memory");
			exit(ENOMEM);
		}
		comms[comm_cnt] = strdup(de->d_name);
		if (!comms[comm_cnt])
		{
			msgbox(main_form, "TinyComm", "Out of memory");
			exit(ENOMEM);
		}
		list_newitem(li, comms[comm_cnt]);
		comm_cnt++;
	}
	if (!comm_cnt)
	{
		msgbox(main_form, "TinyComm", "No serial ports detected.");
		exit(255);
	}
	
	form_lock(main_form);
	form_show(f);
ask_again:
	res = form_wait(f);
	if (res == 1)
	{
		i = list_get_index(li);
		if (i < 0 || i >= comm_cnt)
		{
			msgbox(f, "TinyComm", "Please select a port.");
			goto ask_again;
		}
		strcpy(config.port_name, comms[i]);
	}
	form_close(f);
	form_unlock(main_form);
	
	for (i = 0; i < comm_cnt; i++)
		free(comms[i]);
	free(comms);
	if (res != 1)
		return -1;
	
	reopen_port();
	update_title();
	return 0;
}

void update_title(void)
{
	static char title[256];
	
	if (comm_fd < 0)
	{
		form_set_title(main_form, "TinyComm [Not connected]");
		return;
	}
	
	sprintf(title, "TinyComm [%s, %i, %i, %c, %i]",
		config.port_name,
		config.comm_config.speed,
		config.comm_config.word_len,
		config.comm_config.parity,
		config.comm_config.stop);
	form_set_title(main_form, title);
}

int setup(void)
{
	struct gadget *sp;
	struct gadget *w5;
	struct gadget *w6;
	struct gadget *w7;
	struct gadget *w8;
	struct gadget *pn;
	struct gadget *pe;
	struct gadget *po;
	struct gadget *ph;
	struct gadget *pl;
	struct gadget *s1;
	struct gadget *s2;
	struct form *f;
	int res;
	
	f  = form_load(COMM_FORM);
	w5 = gadget_find(f, "w5");
	w6 = gadget_find(f, "w6");
	w7 = gadget_find(f, "w7");
	w8 = gadget_find(f, "w8");
	pn = gadget_find(f, "pn");
	pe = gadget_find(f, "pe");
	po = gadget_find(f, "po");
	ph = gadget_find(f, "ph");
	pl = gadget_find(f, "pl");
	s1 = gadget_find(f, "s1");
	s2 = gadget_find(f, "s2");
	sp = gadget_find(f, "sp");
	
	popup_newitem(sp, "300");
	popup_newitem(sp, "600");
	popup_newitem(sp, "1200");
	popup_newitem(sp, "2400");
	popup_newitem(sp, "4800");
	popup_newitem(sp, "9600");
	popup_newitem(sp, "14400");
	popup_newitem(sp, "19200");
	popup_newitem(sp, "28800");
	popup_newitem(sp, "38400");
	popup_newitem(sp, "57600");
	popup_newitem(sp, "115200");
	popup_newitem(sp, "230400");
	popup_set_index(sp, 5);
	
	chkbox_set_group(w5, 1);
	chkbox_set_group(w6, 1);
	chkbox_set_group(w7, 1);
	chkbox_set_group(w8, 1);
	
	chkbox_set_group(pn, 2);
	chkbox_set_group(pe, 2);
	chkbox_set_group(po, 2);
	chkbox_set_group(ph, 2);
	chkbox_set_group(pl, 2);
	
	chkbox_set_group(s1, 3);
	chkbox_set_group(s2, 3);
	
	switch (config.comm_config.speed)
	{
	case 300:
		popup_set_index(sp, 0);
		break;
	case 600:
		popup_set_index(sp, 1);
		break;
	case 1200:
		popup_set_index(sp, 2);
		break;
	case 2400:
		popup_set_index(sp, 3);
		break;
	case 4800:
		popup_set_index(sp, 4);
		break;
	case 9600:
		popup_set_index(sp, 5);
		break;
	case 14400:
		popup_set_index(sp, 6);
		break;
	case 19200:
		popup_set_index(sp, 7);
		break;
	case 28800:
		popup_set_index(sp, 8);
		break;
	case 38400:
		popup_set_index(sp, 9);
		break;
	case 57600:
		popup_set_index(sp, 10);
		break;
	case 115200:
		popup_set_index(sp, 11);
		break;
	case 230400:
		popup_set_index(sp, 12);
		break;
	}
	switch (config.comm_config.word_len)
	{
	case 5:
		chkbox_set_state(w5, 1);
		break;
	case 6:
		chkbox_set_state(w6, 1);
		break;
	case 7:
		chkbox_set_state(w7, 1);
		break;
	default:
		chkbox_set_state(w8, 1);
	}
	switch (config.comm_config.parity)
	{
	case 'e':
		chkbox_set_state(pe, 1);
		break;
	case 'o':
		chkbox_set_state(po, 1);
		break;
	case '1':
		chkbox_set_state(ph, 1);
		break;
	case '0':
		chkbox_set_state(pl, 1);
		break;
	default:
		chkbox_set_state(pn, 1);
	}
	switch (config.comm_config.stop)
	{
	case 2:
		chkbox_set_state(s2, 1);
		break;
	default:
		chkbox_set_state(s1, 1);
	}
	
	form_lock(main_form);
	
show_again:
	
	res = form_wait(f);
	if (res == 1)
	{
		struct comm_config new_conf = config.comm_config;
		const char *p;
		int i;
		
		i = popup_get_index(sp);
		p = popup_get_text(sp, i);
		new_conf.speed = strtoul(p, NULL, 0);
		
		new_conf.word_len = 8;
		if (chkbox_get_state(w5))	new_conf.word_len = 5;
		else if (chkbox_get_state(w6))	new_conf.word_len = 6;
		else if (chkbox_get_state(w7))	new_conf.word_len = 7;
		
		new_conf.parity = 'n';
		if (chkbox_get_state(pe))	new_conf.parity = 'e';
		else if (chkbox_get_state(po))	new_conf.parity = 'o';
		else if (chkbox_get_state(ph))	new_conf.parity = '1';
		else if (chkbox_get_state(pl))	new_conf.parity = '0';
		
		new_conf.stop = 1;
		if (chkbox_get_state(s2))
			new_conf.stop = 2;
		
		if (ioctl(comm_fd, COMMIO_SETCONF, &new_conf))
		{
			msgbox(f, "TinyComm", "The serial port driver did not accept these settings.");
			goto show_again;
		}
		config.comm_config = new_conf;
		update_title();
	}
	form_close(f);
	form_unlock(main_form);
	if (res == 1)
		return 0;
	return -1;
}

void set_return(struct menu_item *mi)
{
	struct gadget *lecho;
	struct gadget *crlf;
	struct gadget *cr;
	struct gadget *lf;
	struct form *f;
	
	f     = form_load(KEYB_FORM);
	crlf  = gadget_find(f, "crlf");
	cr    = gadget_find(f, "cr");
	lf    = gadget_find(f, "lf");
	lecho = gadget_find(f, "lecho");
	
	chkbox_set_group(crlf, 1);
	chkbox_set_group(cr, 1);
	chkbox_set_group(lf, 1);
	chkbox_set_state(lecho, config.local_echo);
	
	if (config.ret[0] == '\r')
	{
		if (config.ret[1] == '\n')
			chkbox_set_state(crlf, 1);
		else
			chkbox_set_state(cr, 1);
	}
	else
		chkbox_set_state(lf, 1);
	
	form_lock(main_form);
	if (form_wait(f) == 1)
	{
		if (chkbox_get_state(crlf))
		{
			config.ret[0] = '\r';
			config.ret[1] = '\n';
		}
		else if (chkbox_get_state(cr))
		{
			config.ret[0] = '\r';
			config.ret[1] = 0;
		}
		else if (chkbox_get_state(lf))
		{
			config.ret[0] = '\n';
			config.ret[1] = 0;
		}
		config.local_echo = chkbox_get_state(lecho);
	}
	form_close(f);
	form_unlock(main_form);
}

void set_logs(struct menu_item *mi)
{
	struct gadget *ie;
	struct gadget *in;
	struct gadget *oe;
	struct gadget *on;
	struct form *f;
	
	f  = form_load(LOGS_FORM);
	on = gadget_find(f, "on");
	oe = gadget_find(f, "oe");
	in = gadget_find(f, "in");
	ie = gadget_find(f, "ie");
	
	input_set_text(on, config.out_log_path);
	input_set_text(in, config.in_log_path);
	chkbox_set_state(oe, config.out_log);
	chkbox_set_state(ie, config.in_log);
	
	form_lock(main_form);
	
log_error:
	
	if (form_wait(f) == 1)
	{
		strcpy(config.out_log_path, on->text);
		strcpy(config.in_log_path, in->text);
		config.out_log = chkbox_get_state(oe);
		config.in_log = chkbox_get_state(ie);
		
		close(out_log_fd);
		close(in_log_fd);
		out_log_fd = -1;
		in_log_fd = -1;
		form_lock(f);
		open_logs();
		form_unlock(f);
		
		if (config.out_log && out_log_fd < 0)
			goto log_error;
		if (config.in_log && in_log_fd < 0)
			goto log_error;
	}
	form_close(f);
	form_unlock(main_form);
}

void receive(void)
{
	char buf[256];
	int cnt;
	
	while (cnt = read(comm_fd, buf, sizeof buf), cnt)
	{
		if (cnt < 0)
		{
			if (errno == EAGAIN)
				return;
			sprintf(buf, "Receive error:\n\n%m");
			msgbox(main_form, "TinyComm", buf);
			continue;
		}
		print(buf, cnt);
	}
}

static int reopen_port(void)
{
	char buf[256];
	int se;
	
	close(comm_fd);
	comm_fd = open(config.port_name, O_RDWR|O_NDELAY);
	if (comm_fd < 0)
	{
		se = errno;
		sprintf(buf, "Cannot open %s:\n\n%m", config.port_name);
		msgbox(main_form, "TinyComm", buf);
		errno = se;
		return -1;
	}
	return 0;
}

void init_port(void)
{
	char buf[256];
	
	comm_fd = open(config.port_name, O_RDWR|O_NDELAY);
	if (comm_fd < 0)
	{
		sprintf(buf, "Cannot open %s:\n\n%m", config.port_name);
		form_lock(main_form);
		msgbox(main_form, "TinyComm", buf);
		if (select_port())
			exit(errno);
		form_unlock(main_form);
		
		comm_fd = open(config.port_name, O_RDWR|O_NDELAY);
		if (comm_fd < 0)
		{
			sprintf(buf, "Cannot open %s:\n\n%m", config.port_name);
			form_lock(main_form);
			msgbox(main_form, "TinyComm", buf);
			exit(errno);
		}
	}
	if (ioctl(comm_fd, COMMIO_SETCONF, &config.comm_config))
	{
		sprintf(buf, "Cannot configure comm port:\n\n%m");
		form_lock(main_form);
		msgbox(main_form, "TinyComm", buf);
		if (setup())
			exit(errno);
		form_unlock(main_form);
		
		if (ioctl(comm_fd, COMMIO_SETCONF, &config.comm_config))
		{
			sprintf(buf, "Cannot configure comm port:\n\n%m");
			form_lock(main_form);
			msgbox(main_form, "TinyComm", buf);
			exit(errno);
		}
	}
}

void send(char ch)
{
	char msg[256];
	int cnt;
	
	if (config.local_echo)
		print(&ch, 1);
	if (out_log_fd)
		write(out_log_fd, &ch, 1);
	
retry:
	cnt = write(comm_fd, &ch, 1);
	if (cnt < 0)
	{
		if (errno == EAGAIN)
			goto retry;
		sprintf(msg, "Transmit error:\n\n%m");
		msgbox(main_form, "TinyComm", msg);
	}
}

void key_down(struct gadget *g, unsigned ch, unsigned shift)
{
	if (ch >= 256)
		return;
	
	if (ch == '\n' && !(shift & WIN_SHIFT_SHIFT))
	{
		send(config.ret[0]);
		if (config.ret[1])
			send(config.ret[1]);
		return;
	}
	send(ch);
}

void on_resize(struct form *f, int w, int h)
{
	int nnr_columns;
	int nnr_lines;
	
	nnr_columns = w / cell_width;
	nnr_lines   = h / cell_height;
	if (nnr_columns < 10)
		nnr_columns = 10;
	if (nnr_lines < 5)
		nnr_lines = 5;
	
	if (nnr_columns != nr_columns || nnr_lines != nr_lines)
	{
		char *ncells;
		int *nlnd;
		char *dp;
		char *sp;
		int ccnt;
		int cl;
		
		nlnd   = malloc(nnr_lines * sizeof *nlnd);
		ncells = malloc(nnr_columns * nnr_lines);
		if (!ncells)
		{
			msgbox(main_form, "TinyComm", "Out of memory");
			free(ncells);
			free(nlnd);
			return;
		}
		memset(ncells, ' ', nnr_columns * nnr_lines);
		memset(nlnd, 0, nnr_lines * sizeof *nlnd);
		
		ccnt = nnr_lines;
		if (ccnt > nr_lines)
			ccnt = nr_lines;
		cl = nnr_columns;
		if (cl > nr_columns)
			cl = nr_columns;
		
		dp = ncells;
		sp = cells;
		while (ccnt--)
		{
			memcpy(dp, sp, cl);
			dp += nnr_columns;
			sp += nr_columns;
		}
		if (cur_x > nnr_columns)
			cur_x = nnr_columns;
		if (cur_y >= nnr_lines)
			cur_y = nnr_lines;
		
		nr_columns = nnr_columns;
		nr_lines   = nnr_lines;
		ln_dirty   = nlnd;
		cells	   = ncells;
		/* clear(); */
	}
	
	if (w != cell_width * nnr_columns || h != cell_height * nnr_lines)
	{
		w = cell_width * nnr_columns;
		h = cell_height * nnr_lines;
		form_resize(f, w, h);
	}
	
	if (display)
	{
		gadget_resize(display, w, h);
		gadget_redraw(display);
	}
}

void select_port_click(struct menu_item *mi)
{
	select_port();
}

void setup_click(struct menu_item *mi)
{
	setup();
}

int on_close(struct form *f)
{
	config.nr_columns = nr_columns;
	config.nr_lines	  = nr_lines;
	c_save("tinycomm", &config, sizeof config);
	exit(0);
}

void about(struct menu_item *mi)
{
	dlg_about7(main_form, NULL, "TinyComm v1.3", SYS_PRODUCT, SYS_AUTHOR, SYS_CONTACT, "cdev.pnm");
}

void init_gui(void)
{
	struct menu *port_menu;
	struct menu *opt_menu;
	struct menu *menu;
	
	win_chr_size(WIN_FONT_MONO, &cell_width, &cell_height, 'X');
	
	nr_columns = config.nr_columns;
	nr_lines   = config.nr_lines;
	
	main_form = form_creat(FORM_TITLE | FORM_FRAME | FORM_ALLOW_CLOSE | FORM_ALLOW_MINIMIZE | FORM_NO_BACKGROUND | FORM_ALLOW_RESIZE, 0, -1, -1, nr_columns * cell_width, nr_lines * cell_height, "TinyComm");
	form_on_resize(main_form, on_resize);
	form_on_close(main_form, on_close);
	port_menu = menu_creat();
	menu_newitem(port_menu, "Select ...", select_port_click);
	menu_newitem(port_menu, "Setup ...", setup_click);
	opt_menu = menu_creat();
	menu_newitem(opt_menu, "Keyboard ...", set_return);
	menu_newitem(opt_menu, "Logs ...", set_logs);
	menu_newitem(opt_menu, "-", NULL);
	menu_newitem(opt_menu, "About ...", about);
	menu = menu_creat();
	menu_submenu(menu, "Port", port_menu);
	menu_submenu(menu, "Options", opt_menu);
	form_set_menu(main_form, menu);
	display = gadget_creat(main_form, 0, 0, nr_columns * cell_width, nr_lines * cell_height);
	gadget_set_want_focus(display, 1);
	gadget_set_redraw_cb(display, display_redraw);
	gadget_set_key_down_cb(display, key_down);
	gadget_focus(display);
	form_show(main_form);
	
	ln_dirty = malloc(nr_lines * sizeof *ln_dirty);
	cells = malloc(nr_lines * nr_columns);
	if (!ln_dirty || !cells)
	{
		msgbox(main_form, "Error", "Out of memory");
		exit(255);
	}
	clear();
}

void open_logs(void)
{
	char msg[256 + PATH_MAX];
	
	if (config.out_log)
	{
		out_log_fd = open(config.out_log_path, O_WRONLY|O_APPEND|O_CREAT, 0666);
		if (out_log_fd < 0)
		{
			sprintf(msg, "Cannot open output log file \"%s\":\n\n%m", config.out_log_path);
			form_lock(main_form);
			msgbox(main_form, "TinyComm", msg);
			form_unlock(main_form);
		}
	}
	if (config.in_log)
	{
		in_log_fd = open(config.in_log_path, O_WRONLY|O_APPEND|O_CREAT, 0666);
		if (out_log_fd < 0)
		{
			sprintf(msg, "Cannot open input log file \"%s\":\n\n%m", config.in_log_path);
			form_lock(main_form);
			msgbox(main_form, "TinyComm", msg);
			form_unlock(main_form);
		}
	}
}

int main(int argc, char **argv)
{
	struct pollfd pfd;
	
	if (chdir("/dev"))
	{
		msgbox(NULL, "TinyComm", "Cannot change into /dev");
		return errno;
	}
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		fprintf(stderr, "Usage: tinycomm [PORT [SPEED]]\n\n"
				"Control devices attached to serial ports.\n\n");
		return 0;
	}
	
	memset(&config, 0, sizeof config);
	if (c_load("tinycomm", &config, sizeof config))
	{
		memset(&config, 0, sizeof config);
		config.comm_config.word_len = 8;
		config.comm_config.parity = 'n';
		config.comm_config.stop = 1;
		config.comm_config.speed = 19200;
		config.ret[0] = '\r';
		config.ret[1] = 0;
		config.nr_columns = 80;
		config.nr_lines = 25;
	}
	
	if (win_attach())
		err(255, NULL);
	
	init_gui();
	
	if (argc >= 2)
		strcpy(config.port_name, argv[1]);
	if (argc >= 3)
		config.comm_config.speed = strtoul(argv[2], NULL, 0);
	if (!*config.port_name && select_port())
		return 0;
	
	init_port();
	update_title();
	open_logs();
	win_idle();
	
	pfd.events = POLLIN;
	pfd.fd	   = comm_fd;
	for (;;)
	{
		if (comm_fd >= 0)
		{
			if (poll(&pfd, 1, -1) > 0 && (pfd.revents & POLLIN))
				receive();
			evt_idle();
		}
		else
			evt_wait();
	}
}
