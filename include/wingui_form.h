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

#ifndef _WINGUI_FORM_H
#define _WINGUI_FORM_H

#include <wingui.h>
#include <list.h>

#define FORM_TITLE		1
#define FORM_FRAME		2
#define FORM_ALLOW_CLOSE	4
#define FORM_ALLOW_RESIZE	8
#define FORM_ALLOW_MINIMIZE	16
#define FORM_BACKDROP		32
#define FORM_NO_BACKGROUND	64
#define FORM_EXCLUDE_FROM_LIST	128
#define FORM_NO_FOCUS		256
#define FORM_CENTER		512

#define FORM_APPFLAGS		(FORM_TITLE | FORM_FRAME | FORM_ALLOW_CLOSE | \
				 FORM_ALLOW_RESIZE | FORM_ALLOW_MINIMIZE)

#define FORM_FXS_APPFLAGS	(FORM_TITLE | FORM_FRAME | FORM_ALLOW_CLOSE | \
				 FORM_ALLOW_MINIMIZE)

#define LIST_FRAME		1
#define LIST_DONT_CLEAR_POS	2
#define LIST_VSBAR		4

#define INPUT_FRAME		1
#define INPUT_PASSWORD		2

struct bitmap;
struct gadget;
struct form;

typedef int  form_key_cb(struct form  *form, unsigned ch, unsigned shift);
typedef void form_resize_cb(struct form *form, int w, int h);
typedef void form_move_cb(struct form *form, int x, int y);
typedef int  form_close_cb(struct form *form);
typedef void form_raise_cb(struct form *form);

typedef void gadget_drop_cb(struct gadget *g, const void *data, size_t len);
typedef void gadget_drag_cb(struct gadget *g);

typedef void button_click_cb(struct gadget *g, int x, int y);

typedef void list_request_cb(struct gadget *g, int index, int button);
typedef void list_select_cb(struct gadget *g, int index, int button);
typedef void list_scroll_cb(struct gadget *g, int topmost_index);
typedef void list_drawitem_cb(struct gadget *g, int index, int wd, int x, int y, int w, int h);

typedef void popup_select_cb(struct gadget *g, int index);

typedef void sbar_move_cb(struct gadget *g, int newpos);

typedef void input_change_cb(struct gadget *g);

typedef void edbox_edit_cb(struct gadget *g);
typedef void edbox_vscroll_cb(struct gadget *g, int topmost_line);
typedef void edbox_hscroll_cb(struct gadget *g, int hpix);

typedef void icbox_request_cb(struct gadget *g, int index, int button);
typedef void icbox_select_cb(struct gadget *g, int index, int button);
typedef void icbox_scroll_cb(struct gadget *g, int x, int y);
typedef void icbox_drawbg_cb(struct gadget *g, int wd, int x, int y, int w, int h);
typedef void icbox_drawitem_cb(struct gadget *g, int index, int wd,
			       int x, int y, int w, int h, int sel_only);
typedef void icbox_sizeitem_cb(struct gadget *g, int index, int *w, int *h);

typedef void colorsel_change_cb(struct gadget *g, struct win_rgba *color);

struct gadget
{
	/* public members; read but do not modify */
	
	struct win_rect rect;
	struct form *form;
	int visible;
	char *text;
	void *menu;	/* free to change this member */
	
	int read_only;	/* free to change this member */
	
	void *p_data;	/* free to change this member */
	long l_data;	/* free to change this member */
	
	char *name;
	
	/* private members; do not access */
	
	struct list_item deferred_item;
	struct gadget *next;
	int delayed_redraw;
	int is_internal;
	int want_focus;
	int want_tab;
	int deferred;
	int no_bg;
	int font;
	int ptr;
	int ptr_down_cnt;
	
	union
	{
		struct
		{
			int			pressed;
			int			pointed;
			int			result;
			button_click_cb *	click;
			struct timer *		timer;
			int			c_state;
		} button;
		
		struct
		{
			int			topmost_index;
			int			item_height;
			int			item_index;
			int			item_count;
			int			flags;
			int			last_x;
			int			last_y;
			const char **		items;
			
			struct gadget *		vsbar;
			
			list_request_cb *	request;
			list_select_cb *	select;
			list_scroll_cb *	scroll;
			list_drawitem_cb *	drawitem;
		} list;
		
		struct
		{
			int			item_index;
			int			item_count;
			int			pressed;
			const char **		items;
			
			popup_select_cb *	select;
		} popup;
		
		struct
		{
			int		limit;
			int		pos;
			int		step;
			
			int		sbox_active;
			union
			{
				int	uparrow_active;
				int	lfarrow_active;
			};
			union
			{
				int	dnarrow_active;
				int	rgarrow_active;
			};
			struct win_rect sbar_rect;
			struct win_rect sbox_rect;
			
			struct timer *	timer;
			
			sbar_move_cb *	move;
		} sbar;
		
		struct
		{
			int			c_state;
			int			flags;
			int			curr;
			
			struct timer *		timer;
			
			input_change_cb *	change;
		} input;
		
		struct
		{
			int			topmost_line;
			int			line_height;
			int			curr_column;
			int			curr_line;
			int			curr;
			int			ptr_down;
			int			sel_start;
			int			sel_end;
			int			newsel_start;
			int			c_state;
			int			max_line_length;
			int			hpos;
			int			shadow_w;
			
			int			font;
			int			flags;
			
			struct timer *		timer;
			
			edbox_edit_cb *		edit;
			edbox_vscroll_cb *	vscroll;
			edbox_hscroll_cb *	hscroll;
		} edbox;
		
		struct
		{
			int ptr_down;
			int state;
			int group;
		} chkbox;
		
		struct
		{
			struct bitmap *bitmap;
		} pict;
		
		struct
		{
			int icon_width;
			int icon_height;
			int item_width;
			int item_height;
			int item_index;
			int item_count;
			int scroll_x;
			int scroll_y;
			int last_x;
			int last_y;
			
			int def_scroll_x;
			int def_scroll_y;
			int def_scroll;
			
			int dragdrop;
			
			struct icbox_item
			{
				struct bitmap *icon;
				char *text;
				void *dd_data;
				size_t dd_len;
			} *items;
			
			icbox_request_cb *	request;
			icbox_select_cb *	select;
			icbox_scroll_cb *	scroll;
			icbox_sizeitem_cb *	sizeitem;
			icbox_drawitem_cb *	drawitem;
			icbox_drawbg_cb *	drawbg;
		} icbox;
		
		struct
		{
			struct win_rgba		color;
			
			colorsel_change_cb *	change;
		} colorsel;
		
		struct
		{
			int limit;
			int value;
		} bargraph;
	};
	
	void (*blink)(struct gadget *g);
	void (*focus)(struct gadget *g);
	void (*remove)(struct gadget *g);
	void (*redraw)(struct gadget *g, int wd);
	void (*resize)(struct gadget *g, int w, int h);
	void (*move)(struct gadget *g, int x, int y);
	void (*do_defops)(struct gadget *g);
	void (*set_font)(struct gadget *g, int ftd);
	
	void (*ptr_move)(struct gadget *g, int x, int y);
	void (*ptr_down)(struct gadget *g, int x, int y, int button);
	void (*ptr_up)(struct gadget *g, int x, int y, int button);
	
	void (*key_down)(struct gadget *g, unsigned ch, unsigned shift);
	void (*key_up)(struct gadget *g, unsigned ch, unsigned shift);
	
	void (*drop)(struct gadget *g, const void *data, size_t len);
	void (*udrop)(struct gadget *g, const void *data, size_t len);
	
	void (*drag)(struct gadget *g);
	void (*udrag)(struct gadget *g);
};

struct form_defop
{
	struct list_item list_item;
	int		 listed;
	
	struct win_rect	draw_rect;
	int		draw_closebtn;
	int		draw_zoombtn;
	int		draw_menubtn;
	int		draw_minibtn;
	int		draw_frame;
	int		draw_title;
	int		update_ptr;
	int		gadgets;
	int		zoom;
};

struct form_state
{
	struct win_rect	rect;
	int		state;
};

struct form
{
	/* public members; read but do not modify */
	
	void *		p_data;	/* free to change this member */
	long		l_data;	/* free to change this member */
	int		wd;
	
	/* private members; do not access */
	
	struct list_item list_item;
	
	struct form *	child_dialog;
	struct form *	parent_form;
	
	struct form_defop defop;
	int		redraw_expected;
	int		minimized;
	int		focused;
	int		visible;
	int		zoomed;
	int		result;
	int		locked;
	int		layer;
	int		flags;
	int		busy;
	
	int		autoclose;
	int		refcnt;
	
	struct win_rect workspace_rect;
	struct win_rect closebtn_rect;
	struct win_rect title_rect;
	struct win_rect menu_rect;
	struct win_rect win_rect;
	struct win_rect zoombtn_rect;
	struct win_rect minibtn_rect;
	struct win_rect saved_rect;
	struct win_rect menubtn_rect;
	int		shadow_width;
	int		frame_width;
	
	struct win_rect *bg_rects;
	int		bg_rects_valid;
	int		bg_rects_cap;
	int		bg_rects_cnt;
	
	int		title_btn_pointed_at;
	int		control_menu_active;
	int		kbd_menu_active;
	int		menu_active;
	int		active;
	int		pulling_menu;
	int		resizing;
	int		closing;
	int		zooming;
	int		minimizing;
	int		moving;
	
	char *		title;
	
	struct list	deferred_gadgets;
	struct gadget *	gadget;
	struct gadget *	focus;
	
	struct menu *	menu;
	
	struct gadget *	ptr_down_gadget;
	int		ptr_down_x;
	int		ptr_down_y;
	int		ptr_down;
	int		curr_ptr;
	int		last_x;
	int		last_y;
	
	struct gadget *	drag_gadget;
	
	form_key_cb *	key_down;
	form_key_cb *	key_up;
	form_close_cb *	close;
	form_resize_cb *resize;
	form_move_cb *	move;
	form_raise_cb *	raise;
};

struct form *form_creat(int flags, int visible, int x, int y, int w, int h, const char *title);
int form_close(struct form *form);
int form_set_title(struct form *form, const char *title);
int form_set_layer(struct form *form, int layer);
int form_set_menu(struct form *form, void *menu);
int form_wait(struct form *form);

void form_unbusy(struct form *form);
void form_busy(struct form *form);

int form_set_result(struct form *form, int result);
int form_get_result(struct form *form);
struct gadget *form_get_focus(struct form *form);

void form_get_state(struct form *form, struct form_state *buf);
void form_set_state(struct form *form, struct form_state *buf);

int form_on_key_down(struct form *form, form_key_cb *proc);
int form_on_key_up(struct form *form, form_key_cb *proc);
int form_on_close(struct form *form, form_close_cb *proc);
int form_on_raise(struct form *form, form_raise_cb *proc);
int form_on_resize(struct form *form, form_resize_cb *proc);
int form_on_move(struct form *form, form_move_cb *proc);

void form_set_dialog(struct form *form, struct form *child);
void form_unlock(struct form *form);
void form_lock(struct form *form);

void form_resize(struct form *form, int w, int h);
void form_move(struct form *form, int x, int y);
void form_redraw(struct form *form);
void form_show(struct form *form);
void form_hide(struct form *form);
void form_raise(struct form *form);
void form_focus(struct form *form);

void form_newref(struct form *form);
void form_putref(struct form *form);

struct gadget *gadget_creat(struct form *form, int x, int y, int w, int h);
int gadget_remove(struct gadget *gadget);
void gadget_hide(struct gadget *g);
void gadget_show(struct gadget *g);

void gadget_clip(struct gadget *g, int x, int y, int w, int h, int x0, int y0);
void gadget_defer(struct gadget *g);

void gadget_set_rect(struct gadget *gadget, const struct win_rect *rect);
void gadget_get_rect(struct gadget *gadget, struct win_rect *rect);
void gadget_move(struct gadget *gadget, int x, int y);
void gadget_resize(struct gadget *gadget, int w, int h);
void gadget_setptr(struct gadget *gadget, int ptr);
void gadget_redraw(struct gadget *gadget);
void gadget_focus(struct gadget *gadget);
void gadget_set_menu(struct gadget *gadget, void *menu); /* to be removed */
void gadget_set_font(struct gadget *gadget, int ftd);

int gadget_dragdrop(struct gadget *gadget, const void *data, size_t len);
void gadget_on_drop(struct gadget *gadget, gadget_drop_cb *on_drop);
void gadget_on_drag(struct gadget *gadget, gadget_drag_cb *on_drag);

struct gadget *button_creat(struct form *form, int x, int y, int w, int h,
			    const char *text, button_click_cb *on_click);
void button_set_result(struct gadget *g, int result);
void button_set_text(struct gadget *g, const char *text);
void button_on_click(struct gadget *g, button_click_cb *on_click);

struct gadget *label_creat(struct form *form, int x, int y, const char *text);
int label_set_text(struct gadget *g, const char *text);

struct gadget *list_creat(struct form *form, int x, int y, int w, int h);
void list_set_flags(struct gadget *g, int flags);
int list_newitem(struct gadget *g, const char *text);
void list_clear(struct gadget *g);
int list_scroll(struct gadget *g, int topmost_index);
int list_get_index(struct gadget *g);
int list_set_index(struct gadget *g, int index);
const char *list_get_text(struct gadget *g, int index);
int list_on_request(struct gadget *g, list_request_cb *proc);
int list_on_select(struct gadget *g, list_select_cb *proc);
int list_on_scroll(struct gadget *g, list_scroll_cb *proc);
int list_set_draw_cb(struct gadget *g, list_drawitem_cb *proc);
void list_sort(struct gadget *g);

struct gadget *popup_creat(struct form *form, int x, int y);
int popup_newitem(struct gadget *g, const char *text);
int popup_clear(struct gadget *g);
int popup_get_index(struct gadget *g);
int popup_set_index(struct gadget *g, int index);
const char *popup_get_text(struct gadget *g, int index);
int popup_on_select(struct gadget *g, popup_select_cb *proc);

struct gadget *vsbar_creat(struct form *form, int x, int y, int w, int h);
int vsbar_set_step(struct gadget *g, int step);
int vsbar_set_limit(struct gadget *g, int limit);
int vsbar_set_pos(struct gadget *g, int pos);
int vsbar_get_pos(struct gadget *g);
int vsbar_on_move(struct gadget *g, sbar_move_cb *proc);

struct gadget *hsbar_creat(struct form *form, int x, int y, int w, int h);
int hsbar_set_step(struct gadget *g, int step);
int hsbar_set_limit(struct gadget *g, int limit);
int hsbar_set_pos(struct gadget *g, int pos);
int hsbar_get_pos(struct gadget *g);
int hsbar_on_move(struct gadget *g, sbar_move_cb *proc);

struct gadget *input_creat(struct form *form, int x, int y, int w, int h);
void input_set_flags(struct gadget *g, int flags);
int input_set_text(struct gadget *g, const char *text);
int input_set_pos(struct gadget *g, int pos);
void input_on_change(struct gadget *g, input_change_cb *proc);

struct gadget *edbox_creat(struct form *form, int x, int y, int w, int h);
void edbox_set_flags(struct gadget *g, int flags);
void edbox_set_font(struct gadget *g, int ftd);
int edbox_set_text(struct gadget *g, const char *text);
int edbox_get_text(struct gadget *g, char *buf, int maxlen); /* to be removed */
int edbox_get_text_len(struct gadget *g, int *len); /* to be removed */
int edbox_get_text_ptr(struct gadget *g, char **text); /* to be removed */
int edbox_get_line_count(struct gadget *g, int *count);
int edbox_get_text_width(struct gadget *g, int *width);
int edbox_vscroll(struct gadget *g, int topmost_line);
int edbox_hscroll(struct gadget *g, int hpix);
int edbox_goto(struct gadget *g, int column, int line);
int edbox_goto_index(struct gadget *g, int index);
int edbox_insert(struct gadget *g, const char *text);
int edbox_sel_set(struct gadget *g, int start, int end);
int edbox_sel_get(struct gadget *g, int *start, int *end);
int edbox_sel_delete(struct gadget *g);
int edbox_on_vscroll(struct gadget *g, edbox_vscroll_cb *proc);
int edbox_on_hscroll(struct gadget *g, edbox_hscroll_cb *proc);
int edbox_on_edit(struct gadget *g, edbox_edit_cb *proc);

struct gadget *chkbox_creat(struct form *form, int x, int y, const char *text);
void chkbox_set_state(struct gadget *g, int state);
int chkbox_get_state(struct gadget *g);
void chkbox_set_group(struct gadget *g, int group_nr);

struct gadget *pict_creat(struct form *form, int x, int y, const char *pathname);
int pict_set_bitmap(struct gadget *g, struct bitmap *bmp);
int pict_scale(struct gadget *g, int w, int h);
int pict_load(struct gadget *g, const char *pathname);

struct gadget *icbox_creat(struct form *f, int x, int y, int w, int h);
int icbox_set_icon_size(struct gadget *g, int w, int h);
int icbox_newitem(struct gadget *g, const char *text, const char *icname);
int icbox_set_dd_data(struct gadget *g, int index, const void *data, size_t len);
void icbox_clear(struct gadget *g);
int icbox_get_index(struct gadget *g);
int icbox_set_index(struct gadget *g, int index);
const struct bitmap *icbox_get_icon(struct gadget *g, int index);
const char *icbox_get_text(struct gadget *g, int index);
int icbox_get_scroll_range(struct gadget *g, int *x, int *y);
int icbox_scroll(struct gadget *g, int x, int y);
int icbox_on_request(struct gadget *g, icbox_request_cb *proc);
int icbox_on_select(struct gadget *g, icbox_select_cb *proc);
int icbox_on_scroll(struct gadget *g, icbox_scroll_cb *proc);
void icbox_set_bg_cb(struct gadget *g, icbox_drawbg_cb *proc);
void icbox_set_item_cb(struct gadget *g, icbox_drawitem_cb *proc);
void icbox_set_size_cb(struct gadget *g, icbox_sizeitem_cb *proc);

struct gadget *colorsel_creat(struct form *f, int x, int y, int w, int h);
void colorsel_on_change(struct gadget *g, colorsel_change_cb *proc);
void colorsel_set(struct gadget *g, const struct win_rgba *buf);
void colorsel_get(struct gadget *g, struct win_rgba *buf);

struct gadget *bargraph_creat(struct form *f, int x, int y, int w, int h);
void bargraph_set_limit(struct gadget *g, int limit);
void bargraph_set_value(struct gadget *g, int value);

struct gadget *frame_creat(struct form *f, int x, int y, int w, int h, const char *text);
void frame_set_text(struct gadget *g, const char *text);

/* loadable form support */

struct gadget *gadget_find(struct form *f, const char *name);
struct form *form_load(const char *pathname);

#endif
