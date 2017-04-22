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

void gadget_set_blink_cb(struct gadget *g, gadget_blink_cb *cb)
{
	g->blink = cb;
}

void gadget_set_focus_cb(struct gadget *g, gadget_focus_cb *cb)
{
	g->focus = cb;
}

void gadget_set_remove_cb(struct gadget *g, gadget_remove_cb *cb)
{
	g->remove = cb;
}

void gadget_set_redraw_cb(struct gadget *g, gadget_redraw_cb *cb)
{
	g->redraw = cb;
}

void gadget_set_resize_cb(struct gadget *g, gadget_resize_cb *cb)
{
	g->resize = cb;
}

void gadget_set_move_cb(struct gadget *g, gadget_move_cb *cb)
{
	g->move = cb;
}

void gadget_set_do_defops_cb(struct gadget *g, gadget_do_defops_cb *cb)
{
	g->do_defops = cb;
}

void gadget_set_set_font_cb(struct gadget *g, gadget_set_font_cb *cb)
{
	g->set_font = cb;
}

void gadget_set_ptr_move_cb(struct gadget *g, gadget_ptr_move_cb *cb)
{
	g->ptr_move = cb;
}

void gadget_set_ptr_down_cb(struct gadget *g, gadget_ptr_down_cb *cb)
{
	g->ptr_down = cb;
}

void gadget_set_ptr_up_cb(struct gadget *g, gadget_ptr_up_cb *cb)
{
	g->ptr_up = cb;
}

void gadget_set_key_down_cb(struct gadget *g, gadget_key_down_cb *cb)
{
	g->key_down = cb;
}

void gadget_set_key_up_cb(struct gadget *g, gadget_key_up_cb *cb)
{
	g->key_up = cb;
}

void gadget_set_drop_cb(struct gadget *g, gadget_drop_cb *cb)
{
	g->drop = cb;
}

void gadget_set_drag_cb(struct gadget *g, gadget_drag_cb *cb)
{
	g->drag = cb;
}

void gadget_set_want_focus(struct gadget *g, int flag)
{
	g->want_focus = flag;
}

void gadget_set_want_tab(struct gadget *g, int flag)
{
	g->want_tab = flag;
}
