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

#ifndef _WINGUI_MENU_H
#define _WINGUI_MENU_H

#include <wingui.h>

struct menu_item
{
	/* public members */
	
	void		(*proc)(struct menu_item *m);
	char *		text;
	
	void *		p_data;
	long		l_data;
	
	unsigned	ch;
	unsigned	shift;
	
	/* private members; do not access */
	
	struct menu *	submenu;
	struct win_rect	rect;
};

struct menu
{
	struct menu_item *item;
	int		item_count;
	
	int		wd;
	int		visible;
	struct win_rect	rect;
	
	int		selection;
};

typedef void menu_cb(struct menu_item *mi);

struct menu *menu_creat(void);
void menu_free(struct menu *menu);

struct menu_item *menu_newitem5(struct menu *menu, const char *text, unsigned ch, unsigned shift, menu_cb *proc);
struct menu_item *menu_newitem4(struct menu *menu, const char *text, unsigned ch, menu_cb *proc);
struct menu_item *menu_newitem(struct menu *menu, const char *text, menu_cb *proc);
struct menu_item *menu_submenu(struct menu *menu, const char *text, struct menu *submenu);

int menu_popup_l(struct menu *menu, int x, int y, int layer);
int menu_popup(struct menu *menu, int x, int y);
int menu_popdown(struct menu *menu);

#endif
