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

#ifndef _WINGUI_CGADGET_H
#define _WINGUI_CGADGET_H

#include <wingui_form.h>

struct gadget *gadget_creat(struct form *form, int x, int y, int w, int h);

typedef void gadget_blink_cb(struct gadget *g);
typedef void gadget_focus_cb(struct gadget *g);
typedef void gadget_remove_cb(struct gadget *g);
typedef void gadget_redraw_cb(struct gadget *g, int wd);
typedef void gadget_resize_cb(struct gadget *g, int w, int h);
typedef void gadget_move_cb(struct gadget *g, int x, int y);
typedef void gadget_do_defops_cb(struct gadget *g);
typedef void gadget_set_font_cb(struct gadget *g, int ftd);

typedef void gadget_ptr_move_cb(struct gadget *g, int x, int y);
typedef void gadget_ptr_down_cb(struct gadget *g, int x, int y, int button);
typedef void gadget_ptr_up_cb(struct gadget *g, int x, int y, int button);

typedef void gadget_key_down_cb(struct gadget *g, unsigned ch, unsigned shift);
typedef void gadget_key_up_cb(struct gadget *g, unsigned ch, unsigned shift);

void gadget_set_blink_cb(struct gadget *g, gadget_blink_cb *cb);
void gadget_set_focus_cb(struct gadget *g, gadget_focus_cb *cb);
void gadget_set_remove_cb(struct gadget *g, gadget_remove_cb *cb);
void gadget_set_redraw_cb(struct gadget *g, gadget_redraw_cb *cb);
void gadget_set_resize_cb(struct gadget *g, gadget_resize_cb *cb);
void gadget_set_move_cb(struct gadget *g, gadget_move_cb *cb);
void gadget_set_do_defops_cb(struct gadget *g, gadget_do_defops_cb *cb);
void gadget_set_set_font_cb(struct gadget *g, gadget_set_font_cb *cb);

void gadget_set_ptr_move_cb(struct gadget *g, gadget_ptr_move_cb *cb);
void gadget_set_ptr_down_cb(struct gadget *g, gadget_ptr_down_cb *cb);
void gadget_set_ptr_up_cb(struct gadget *g, gadget_ptr_up_cb *cb);

void gadget_set_key_down_cb(struct gadget *g, gadget_key_down_cb *cb);
void gadget_set_key_up_cb(struct gadget *g, gadget_key_up_cb *cb);

void gadget_set_drop_cb(struct gadget *g, gadget_drop_cb *cb);
void gadget_set_drag_cb(struct gadget *g, gadget_drag_cb *cb);

void gadget_set_want_focus(struct gadget *g, int flag);

#endif
