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

#ifndef _WINGUI_H
#define _WINGUI_H

#include <sys/types.h>

#define WIN_E_KEY_DOWN		1
#define WIN_E_KEY_UP		2
#define WIN_E_PTR_MOVE		3
#define WIN_E_PTR_DOWN		4
#define WIN_E_PTR_UP		5

#define WIN_E_REDRAW		6
#define WIN_E_SWITCH_FROM	7
#define WIN_E_SWITCH_TO		8
#define WIN_E_RAISE		9

#define WIN_E_SETMODE		10
#define WIN_E_UPDATE		11
#define WIN_E_RESIZE		12

#define WIN_E_CREAT		13
#define WIN_E_CLOSE		14
#define WIN_E_TITLE		15
#define WIN_E_CHANGE		16

#define WIN_E_SEC_LOCK		17
#define WIN_E_SEC_UNLOCK	18

#define WIN_E_BLINK		19

#define WIN_E_DROP		20
#define WIN_E_DRAG		21

#define WIN_KEY_UP		((unsigned) -1)
#define WIN_KEY_DOWN		((unsigned) -2)
#define WIN_KEY_LEFT		((unsigned) -3)
#define WIN_KEY_RIGHT		((unsigned) -4)

#define WIN_KEY_PGUP		((unsigned) -5)
#define WIN_KEY_PGDN		((unsigned) -6)
#define WIN_KEY_HOME		((unsigned) -7)
#define WIN_KEY_END		((unsigned) -8)

#define WIN_KEY_INS		((unsigned) -9)
#define WIN_KEY_DEL		((unsigned) -10)

#define WIN_KEY_F1		((unsigned) -11)
#define WIN_KEY_F2		((unsigned) -12)
#define WIN_KEY_F3		((unsigned) -13)
#define WIN_KEY_F4		((unsigned) -14)
#define WIN_KEY_F5		((unsigned) -15)
#define WIN_KEY_F6		((unsigned) -16)
#define WIN_KEY_F7		((unsigned) -17)
#define WIN_KEY_F8		((unsigned) -18)
#define WIN_KEY_F9		((unsigned) -19)
#define WIN_KEY_F10		((unsigned) -20)
#define WIN_KEY_F11		((unsigned) -21)
#define WIN_KEY_F12		((unsigned) -22)
#define WIN_KEY_F13		((unsigned) -23)
#define WIN_KEY_F14		((unsigned) -24)
#define WIN_KEY_F15		((unsigned) -25)
#define WIN_KEY_F16		((unsigned) -26)

#define WIN_KEY_SHIFT		((unsigned) -27)
#define WIN_KEY_CTRL		((unsigned) -28)
#define WIN_KEY_ALT		((unsigned) -29)

#define WIN_KEY_SYSRQ		((unsigned) -30)

#define WIN_KEY_CAPS		((unsigned) -31)
#define WIN_KEY_NLOCK		((unsigned) -32)
#define WIN_KEY_SLOCK		((unsigned) -33)

#define WIN_PTR_DEFAULT		1
#define WIN_PTR_MAX		8

#define WIN_PTR_INVISIBLE	0
#define WIN_PTR_ARROW		1
#define WIN_PTR_TEXT		2
#define WIN_PTR_BUSY		3
#define WIN_PTR_NW_SE		4
#define WIN_PTR_DRAG		5

#define WIN_SHIFT_SHIFT		1
#define WIN_SHIFT_CTRL		2
#define WIN_SHIFT_ALT		4
#define WIN_SHIFT_CLOCK		8
#define WIN_SHIFT_NLOCK		16
#define WIN_SHIFT_SLOCK		32

#define WIN_FONT_DEFAULT	(-1)
#define WIN_FONT_MONO		0
#define WIN_FONT_SYSTEM		1

#define FONT_NAME_MAX		31
#define WIN_TITLE_MAX		63

#define WIN_BTN_MAX		16

#define WIN_LAYER_SECURE	(-1)
#define WIN_LAYER_MENU		1
#define WIN_LAYER_BELOW_MENU	2
#define WIN_LAYER_TASKBAR	3
#define WIN_LAYER_TOP		4
#define WIN_LAYER_APP		5

#define WIN_DD_BUF		4096

#define WIN_DF_SLOWCOPY		1
#define WIN_DF_BOOTFB		2

typedef unsigned win_color;

struct win_ptrspeed
{
	int acc_threshold;
	int nor_mul;
	int nor_div;
	int acc_mul;
	int acc_div;
};

struct win_modeinfo
{
	unsigned width;
	unsigned height;
	unsigned ncolors;
	
	unsigned user_cte;
	unsigned user_cte_count;
};

struct win_rgba
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
};

struct win_rect
{
	int x;
	int y;
	int w;
	int h;
};

int win_newdesktop(const char *name);
int win_killdesktop(sig_t signal);
int win_attach(void);
int win_detach(void);
int win_owner(uid_t uid);
int win_get_ptr_pos(int *x, int *y);

int win_advise_pos(int *x, int *y, int w, int h);
int win_creat(int *wd, int visible, int x, int y, int w, int h, void *proc, void *data);
int win_change(int wd, int visible, int x, int y, int w, int h);
int win_setlayer(int wd, int layer);
int win_setptr(int wd, int ptr);
int win_deflayer(int *layer);
int win_raise(int wd);
int win_focus(int wd);
int win_close(int wd);
int win_ufocus(int wd);

int win_get_title(int wd, char *buf, int max);
int win_set_title(int wd, const char *title);
int win_is_visible(int wd, int *visible);
int win_redraw_all(void);

int win_paint(void);
int win_end_paint(void);

int win_rgba2color(win_color *c, int r, int g, int b, int a);
int win_rgb2color(win_color *c, int r, int g, int b);

int win_clip(int wd, int x, int y, int w, int h, int x0, int y0);
int win_pixel(int wd, win_color c, int x, int y);
int win_hline(int wd, win_color c, int x, int y, int l);
int win_vline(int wd, win_color c, int x, int y, int l);
int win_rect(int wd, win_color c, int x, int y, int w, int h);
int win_copy(int wd, int dx, int dy, int sx, int sy, int w, int h);
int win_text(int wd, win_color c, int x, int y, const char *text);
int win_btext(int wd, win_color bg, win_color fg, int x, int y, const char *text);
int win_text_size(int ftd, int *w, int *h, const char *text);
int win_chr(int wd, win_color c, int x, int y, unsigned ch);
int win_bchr(int wd, win_color bg, win_color fg, int x, int y, unsigned ch);
int win_chr_size(int ftd, int *w, int *h, unsigned ch);

int win_bitmap(int wd, const win_color *bmp, int x, int y, int w, int h);
int win_bconv(win_color *c, const struct win_rgba *rgba, unsigned count);

int win_desktop_size(int *width, int *height);
int win_ws_getrect(struct win_rect *r);
int win_ws_setrect(const struct win_rect *r);

int win_rect_preview(int enable, int x, int y, int w, int h);

int win_insert_font(int *ftd, const char *name, const void *image, size_t image_size);
int win_load_font(int *ftd, const char *name, const char *pathname);
int win_find_font(int *ftd, const char *name);
int win_set_font(int wd, int ftd);

int win_modeinfo(int i, struct win_modeinfo *buf);
int win_getmode(int *i);
int win_setmode(int i, int refresh);
int win_dispflags(void);

int win_setcte(win_color c, int r, int g, int b);
int win_getcte(win_color c, int *r, int *g, int *b);

int win_update(void);
int win_reset(void);

int win_insert_ptr(int ptr, const win_color *pixels, const unsigned *mask, int w, int h, int hx, int hy);
int win_set_ptr_speed(const struct win_ptrspeed *speed);
int win_get_ptr_speed(struct win_ptrspeed *speed);

int win_taskbar(void *proc);
int win_chk_taskbar(void);
int win_sec_unlock(void);

int win_map_buttons(const int *tab, int cnt);

int win_soft_keydown(unsigned ch);
int win_soft_keyup(unsigned ch);

int win_blink(int wd, int ena);

int win_gdrop(void *buf, size_t bufsz, int *serial);
int win_dragdrop(const void *data, size_t size);

/* library routines */

typedef void win_setmode_cb(void);
typedef void win_update_cb(void);
typedef void win_resize_cb(void);
typedef void win_unlock_cb(void);
typedef void win_lock_cb(void);

char *win_break(int ftd, const char *str, int width);
int win_inrect(struct win_rect *r, int x, int y);
int win_signal(int sig, void *proc);
void win_on_setmode(win_setmode_cb *proc);
void win_on_update(win_update_cb *proc);
void win_on_resize(win_update_cb *proc);
void win_on_unlock(win_unlock_cb *proc);
void win_on_lock(win_lock_cb *proc);
int win_event_count(void);
int win_wait(void);
int win_idle(void);

#endif
