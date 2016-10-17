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

#include <priv/wingui_theme.h>
#include <wingui_color.h>
#include <wingui_form.h>
#include <wingui.h>
#include <stdlib.h>
#include <string.h>
#include <timer.h>
#include <time.h>

static const struct timeval button_tv = { 0, 333333L };

static void button_remove(struct gadget *g)
{
	tmr_free(g->button.timer);
	free(g->text);
}

static void button_redraw(struct gadget *g, int wd)
{
	struct form_theme *th;
	win_color hl1;
	win_color hl2;
	win_color sh1;
	win_color sh2;
	win_color bg;
	win_color fg;
	int x, y;
	int w, h;
	
	th = form_th_get();
	if (th != NULL && th->d_button != NULL)
	{
		int fl = 0;
		
		if (g->button.pointed && g->button.pressed)
			fl |= DB_DEPSD;
		
		if (g == g->form->focus && g->button.c_state && g->form->focused)
			fl |= DB_FOCUS;
		
		th->d_button(wd, 0, 0, g->rect.w, g->rect.h, g->text, fl);
		return;
	}
	
	bg = wc_get(WC_BUTTON_BG);
	fg = wc_get(WC_BUTTON_FG);
	
	if (g->button.pointed && g->button.pressed)
	{
		sh1 = wc_get(WC_BUTTON_HI1);
		sh2 = wc_get(WC_BUTTON_HI2);
		hl1 = wc_get(WC_BUTTON_SH1);
		hl2 = wc_get(WC_BUTTON_SH2);
	}
	else
	{
		hl1 = wc_get(WC_BUTTON_HI1);
		hl2 = wc_get(WC_BUTTON_HI2);
		sh1 = wc_get(WC_BUTTON_SH1);
		sh2 = wc_get(WC_BUTTON_SH2);
	}
	
	win_text_size(g->font, &w, &h, g->text);
	x = (g->rect.w - w) / 2;
	y = (g->rect.h - h) / 2;
	
	win_rect(wd, bg, 0, 0, g->rect.w, g->rect.h);
	win_text(wd, fg, x, y, g->text);
	
	win_hline(wd, hl1, 0, 0, g->rect.w);
	win_hline(wd, sh1, 0, g->rect.h - 1, g->rect.w);
	win_vline(wd, hl1, 0, 0, g->rect.h);
	win_vline(wd, sh1, g->rect.w - 1, 0, g->rect.h);
	
	win_hline(wd, hl2, 1, 1, g->rect.w - 2);
	win_hline(wd, sh2, 1, g->rect.h - 2, g->rect.w - 2);
	win_vline(wd, hl2, 1, 1, g->rect.h - 2);
	win_vline(wd, sh2, g->rect.w - 2, 1, g->rect.h - 2);
	
	if (g == g->form->focus)
	{
		win_hline(wd, fg, 3, 3, g->rect.w - 6);
		win_hline(wd, fg, 3, g->rect.h - 4, g->rect.w - 6);
		win_vline(wd, fg, 3, 3, g->rect.h - 6);
		win_vline(wd, fg, g->rect.w - 4, 3, g->rect.h - 6);
	}
}

static void button_ptr_move(struct gadget *g, int x, int y)
{
	int p = 0;
	
	if (x >= 0 && x < g->rect.w && y >= 0 && y < g->rect.h)
		p = 1;
	
	if (g->button.pointed != p)
	{
		g->button.pointed = p;
		gadget_redraw(g);
	}
}

static void button_ptr_down(struct gadget *g, int x, int y, int button)
{
	if (x >= 0 && x < g->rect.w && y >= 0 && y < g->rect.h)
		g->button.pointed = 1;
	else
		g->button.pointed = 0;
	
	if (button == 0)
	{
		g->button.pressed++;
		gadget_redraw(g);
	}
}

static void button_ptr_up(struct gadget *g, int x, int y, int button)
{
	if (x >= 0 && x < g->rect.w && y >= 0 && y < g->rect.h)
		g->button.pointed = 1;
	else
		g->button.pointed = 0;
	
	if (button == 0)
	{
		g->button.pressed--;
		gadget_redraw(g);
		
		if (!g->button.pressed && g->button.pointed)
		{
			if (g->button.result)
				g->form->result = g->button.result;
			
			if (g->button.click)
			{
				form_lock(g->form);
				g->button.click(g, x, y);
				form_unlock(g->form);
			}
		}
	}
}

static void button_key_down(struct gadget *g, unsigned ch, unsigned shift)
{
	if (ch == '\n' && !g->button.pressed)
	{
		if (g->button.result)
			g->form->result = g->button.result;
		
		if (g->button.click)
		{
			form_lock(g->form);
			g->button.click(g, -1, -1);
			form_unlock(g->form);
		}
	}
}

static void button_focus(struct gadget *g)
{
	if (g->form->focus == g)
	{
		g->button.c_state = 1;
		
		tmr_reset(g->button.timer, &button_tv, &button_tv);
		tmr_start(g->button.timer);
	}
	else
		tmr_stop(g->button.timer);
	
	gadget_redraw(g);
}

static void button_tmr(void *p)
{
	struct gadget *g = p;
	
	g->button.c_state = !g->button.c_state;
	gadget_redraw(g);
}

struct gadget *button_creat(struct form *form, int x, int y, int w, int h,
			    const char *text, button_click_cb *on_click)
{
	struct gadget *g;
	
	g = gadget_creat(form, x, y, w, h);
	if (!g)
		return NULL;
	
	g->text = strdup(text);
	if (!g->text)
	{
		gadget_remove(g);
		return NULL;
	}
	
	g->button.timer = tmr_creat(&button_tv, &button_tv, button_tmr, g, 0);
	if (g->button.timer == NULL)
	{
		free(g->text);
		g->text = NULL;
		gadget_remove(g);
		return NULL;
	}
	
	g->remove     = button_remove;
	g->redraw     = button_redraw;
	g->key_down   = button_key_down;
	g->ptr_move   = button_ptr_move;
	g->ptr_down   = button_ptr_down;
	g->ptr_up     = button_ptr_up;
	g->focus      = button_focus;
	g->want_focus = 1;
	
	g->button.click   = on_click;
	g->button.pressed = 0;
	
	gadget_redraw(g);
	return g;
}

void button_set_result(struct gadget *g, int result)
{
	g->button.result = result;
}

void button_on_click(struct gadget *g, button_click_cb *on_click)
{
	g->button.click = on_click;
}

void button_set_text(struct gadget *g, const char *text)
{
	free(g->text);
	
	g->text = strdup(text);
	gadget_redraw(g);
}
