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

#include <config/defaults.h>
#include <wingui_metrics.h>
#include <wingui_msgbox.h>
#include <wingui_form.h>
#include <wingui_bell.h>
#include <wingui.h>
#include <dev/speaker.h>
#include <string.h>
#include <errno.h>

int msgbox(struct form *form, const char *title, const char *text)
{
	int flags = FORM_FRAME | FORM_TITLE | FORM_CENTER;
	int tl = wm_get(WM_THIN_LINE);
	struct gadget *button;
	struct gadget *label;
	struct form *f = NULL;
	struct win_rect wsr;
	char *brtext;
	int text_w;
	int text_h;
	int win_w;
	int win_h;
	int btn_h;
	int btn_w;
	int r;
	
	win_ws_getrect(&wsr);
	
	brtext = win_break(WIN_FONT_DEFAULT, text, wsr.w);
	if (brtext == NULL)
		return -1;
	if (form)
		flags |= FORM_EXCLUDE_FROM_LIST;
	
	win_text_size(WIN_FONT_DEFAULT, &text_w, &text_h, brtext);
	win_text_size(WIN_FONT_DEFAULT, &btn_w, &btn_h, "XX");
	btn_w += 20 * tl;
	btn_h += 6  * tl;
	win_w = text_w + 8 * tl;
	win_h = text_h + btn_h + 22 * tl;
	
	f = form_creat(flags, 0, -1, -1, win_w, win_h, title);
	if (f == NULL)
		goto fail;
	
	label = label_creat(f, 4 * tl, 8 * tl, brtext);
	if (label == NULL)
		goto fail;
	button = button_creat(f, (win_w - btn_w) / 2, text_h + 16 * tl, btn_w, btn_h, "OK", NULL);
	if (button == NULL)
		goto fail;
	button_set_result(button, MSGBOX_OK);
	gadget_focus(button);
	
	if (form)
		form_set_dialog(form, f);
	
	wb(WB_INFO);
	r = form_wait(f);
	
	if (form)
		form_set_dialog(form, NULL);
	
	form_hide(f);
	win_idle();
	form_close(f);
	free(brtext);
	return r;
fail:
	r = _get_errno();
	if (f != NULL)
		form_close(f);
	free(brtext);
	_set_errno(r);
	return -1;
}

int msgbox_ask4(struct form *form, const char *title, const char *text, int dflt)
{
	int flags = FORM_FRAME | FORM_TITLE | FORM_CENTER;
	int tl = wm_get(WM_THIN_LINE);
	struct gadget *button_yes;
	struct gadget *button_no;
	struct gadget *label;
	struct form *f = NULL;
	struct win_rect wsr;
	char *brtext;
	int text_w;
	int text_h;
	int win_w;
	int win_h;
	int btn_w;
	int btn_h;
	int r;
	
	win_ws_getrect(&wsr);
	
	brtext = win_break(WIN_FONT_DEFAULT, text, wsr.w);
	if (brtext == NULL)
		return -1;
	if (form)
		flags |= FORM_EXCLUDE_FROM_LIST;
	
	win_text_size(WIN_FONT_DEFAULT, &text_w, &text_h, brtext);
	win_text_size(WIN_FONT_DEFAULT, &btn_w, &btn_h, "XXX");
	btn_w += 20 * tl;
	btn_h += 6  * tl;
	win_w = text_w + 8 * tl;
	win_h = text_h + btn_h + 22 * tl;
	
	f = form_creat(flags, 0, -1, -1, win_w, win_h, title);
	if (f == NULL)
		goto fail;
	
	label = label_creat(f, 4 * tl, 8 * tl, brtext);
	if (label == NULL)
		goto fail;
	
	button_yes = button_creat(f, win_w / 2 - btn_w - 1, text_h + 16 * tl, btn_w, btn_h, "Yes", NULL);
	if (button_yes == NULL)
		goto fail;
	
	button_no = button_creat(f, win_w / 2 + 1, text_h + 16 * tl, btn_w, btn_h, "No", NULL);
	if (button_no == NULL)
		goto fail;
	
	button_set_result(button_yes, MSGBOX_YES);
	button_set_result(button_no, MSGBOX_NO);
	
	switch (dflt)
	{
	case MSGBOX_YES:
		gadget_focus(button_yes);
		break;
	case MSGBOX_NO:
		gadget_focus(button_no);
		break;
	}
	
	if (form)
		form_set_dialog(form, f);
	
	wb(WB_CONFIRM);
	r = form_wait(f);
	
	if (form)
		form_set_dialog(form, NULL);
	
	form_hide(f);
	win_idle();
	form_close(f);
	free(brtext);
	return r;
fail:
	r = _get_errno();
	if (f != NULL)
		form_close(f);
	free(brtext);
	_set_errno(r);
	return -1;
}

int msgbox_ask(struct form *form, const char *title, const char *text)
{
	return msgbox_ask4(form, title, text, MSGBOX_YES);
}

int msgbox_perror(struct form *form, const char *title, const char *text, int err)
{
	size_t len;
	char *fmt;
	char *s;
	int r;
	
	if (text == NULL)
		return msgbox(form, title, strerror(err));
	
	fmt = "%s:\n\n%s";
	len = strlen(text);
	if (len && text[len - 1] == '\n')
		fmt = "%s\n%s";
	if (asprintf(&s, fmt, text, strerror(err)) < 0)
		return -1;
	r = msgbox(form, title, s);
	free(s);
	return r;
}
