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

#ifndef _KERN_WINGUI_H
#define _KERN_WINGUI_H

#include <kern/limits.h>
#include <sys/types.h>
#include <stdint.h>
#include <wingui.h>
#include <event.h>
#include <list.h>

#define WIN_MAX			64
#define FONT_MAX		16

#define PTR_WIDTH		64
#define PTR_HEIGHT		64

#define DEFAULT_FTD		1

struct win_glyph
{
	unsigned	nr;
	unsigned	pos;
	unsigned	width;
	unsigned	height;
};

extern struct win_font
{
	char *		  data;
	char		  name[FONT_NAME_MAX+1];
	struct win_glyph *glyph[256];
	uint8_t *	  bitmap;
	unsigned	  linelen;
} win_font[FONT_MAX];

struct win_pixbuf
{
	unsigned	width, height;
	void *		data;
};

struct win_display
{
	int		flags;
	int		width;
	int		height;
	int		ncolors;
	int		user_cte;
	int		user_cte_count;
	int		mode;
	win_color	transparent;
	void *		data;
	
	int (*modeinfo)(void *dd, int mode, struct win_modeinfo *buf);
	int (*setmode)(void *dd, int mode, int refresh);
	
	void (*setptr)(void *dd, const win_color *shape, const unsigned *mask);
	void (*moveptr)(void *dd, int x, int y);
	void (*showptr)(void *dd);
	void (*hideptr)(void *dd);
	
	void (*putpix)(void *dd, int x, int y, win_color c);
	void (*getpix)(void *dd, int x, int y, win_color *c);
	void (*hline)(void *dd, int x, int y, int len, win_color c);
	void (*vline)(void *dd, int x, int y, int len, win_color c);
	void (*rect)(void *dd, int x, int y, int w, int h, win_color c);
	void (*copy)(void *dd, int x0, int y0, int x1, int y1, int w, int h);
	
	void (*rgba2color)(void *dd, int r, int g, int b, int a, win_color *c);
	void (*color2rgba)(void *dd, int *r, int *g, int *b, int *a, win_color c);
	void (*invert)(void *dd, win_color *c);
	
	void (*setcte)(void *dd, win_color c, int r, int g, int b);
	
	int  (*newbuf)(void *dd, struct win_pixbuf *pb);
	void (*freebuf)(void *dd, struct win_pixbuf *pb);
	void (*setbuf)(void *dd, struct win_pixbuf *pb);
	void (*copybuf)(void *dd, struct win_pixbuf *pb, int x, int y);
	void (*combbuf)(void *dd, struct win_pixbuf *pb, int x, int y);
	
	int  (*swapctl)(void *dd, int ena);
	void (*swap)(void *dd);
};

struct win_pointer
{
	win_color		pixels[PTR_WIDTH * PTR_HEIGHT];
	unsigned		mask[PTR_WIDTH * PTR_HEIGHT];
	int			hx, hy;
};

struct win_window
{
	struct list_item	stack_item;
	
	struct task *		task;
	void *			proc;
	void *			data;
	
	int			x0;
	int			y0;
	int			font;
	int			visible;
	int			layer;
	struct win_rect		clip;
	struct win_rect		rect;
	int			blink_ena;
	char			title[WIN_TITLE_MAX + 1];
	struct win_pointer *	pointer;
	struct win_pixbuf	buf;
};

struct win_desktop
{
	struct list_item	list_item;
	
	char			name[NAME_MAX + 1];
	int			sec_locked;
	int			painting;
	int			refcnt;
	int			display_locked;
	struct win_ptrspeed	ptr_speed;
	int			ptr_x;
	int			ptr_y;
	int			ptr_down;
	int			ptr_down_wd;
	int			ptr_moved;
	int			ptr_force_rmb;
	unsigned		key_shift;
	int			focus;
	int			window_count;
	struct win_display *	display;
	struct win_window	window[WIN_MAX];
	struct list		stack;
	int			rect_preview_ena;
	struct win_rect		rect_preview;
	int			input_md[16];
	int			display_md;
	struct task *		taskbar;
	void *			taskbar_proc;
	struct win_rect		ws_rect;
	int			buttons[WIN_BTN_MAX];
	time_t			last_update;
	int			last_auto_x;
	int			last_auto_y;
	struct win_pointer	pointers[WIN_PTR_MAX];
	struct win_pointer *	currptr;
	uid_t			owner_uid;
	struct win_rect		invalid_rect;
	char			dd_buf[WIN_DD_BUF];
	size_t			dd_len;
	int			dd_serial;
	int			dragdrop;
	int			dpi_class;
	int			font_map[FONT_MAX];
};

struct win_task
{
	struct win_desktop *	inh_desktop;
	struct win_desktop *	desktop;
	int			painting;
};

extern struct win_display *win_syscon; /* XXX to be removed */
extern struct win_display null_display;
extern struct list win_desktops;

struct win_desktop *win_getdesktop(void);

void	win_init(void);

void	win_newtask(struct task *p);
void	win_clean(void);

int	win_display(int md, struct win_display *disp);
int	win_input(int md);
void	win_uninstall_display(struct win_desktop *d, struct win_display *disp);
void	win_uninstall_input(struct win_desktop *d, int md);

void	win_rect_preview_invert(int ox, int oy, int w, int h);

void	win_update_ptr(struct win_desktop *d);

void	win_broadcast_setmode(struct win_desktop *d);
void	win_broadcast_resize(struct win_desktop *d);

int	win_chk_ftd(int ftd);

int	win_redraw(int wd);
int	win_chkwd(int wd);

int	win_event(struct win_desktop *d, struct event *e);
void	win_bcast(struct win_desktop *d, struct event *e);

int	win_keydown(struct win_desktop *d, unsigned ch);
int	win_keyup(struct win_desktop *d, unsigned ch);

void	win_xywd(struct win_desktop *d, int *wd, int x, int y);

int	win_ptrmove_rel(struct win_desktop *d, int x, int y);
int	win_ptrmove(struct win_desktop *d, int x, int y);
int	win_ptrdown(struct win_desktop *d, int button);
int	win_ptrup(struct win_desktop *d, int button);

void	win_lock(void);
void	win_unlock(void);

#endif
