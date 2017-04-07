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

#include <wingui_color.h>
#include <wingui_form.h>
#include <wingui.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <err.h>

#define COLS	80
#define LINES	25

static struct form *f;

static int f_close(struct form *f)
{
	exit(0);
}

int main(int argc, char **argv)
{
	win_color bg, fg;
	char text[2 * COLS + LINES + 1];
	char buf[256];
	time_t pt, ct;
	int cw, ch;
	int w, h;
	int fps;
	int cnt;
	int i;
	
	if (win_attach())
		err(255, NULL);
	
	bg = wc_get(WC_BG);
	fg = wc_get(WC_FG);
	
	win_chr_size(WIN_FONT_MONO, &cw, &ch, 'X');
	w = cw *  COLS;
	h = ch * (LINES + 1);
	
	f = form_creat(FORM_TITLE | FORM_FRAME | FORM_ALLOW_CLOSE | FORM_ALLOW_MINIMIZE, 1, -1, -1, w, h, "Text output benchmark");
	form_on_close(f, f_close);
	
	for (i = 0; i < 2 * COLS; i++)
		text[i] = 33 + i % 94;
	text[sizeof text - 1] = 0;
	
	for (pt = 0, fps = 0, cnt = 0; ;)
	{
		win_idle();
		win_set_font(f->wd, WIN_FONT_MONO);
		win_clip(f->wd,
			 f->workspace_rect.x, f->workspace_rect.y, w, h,
			 f->workspace_rect.x, f->workspace_rect.y);
		win_paint();
		for (i = 0; i < LINES; i++)
			win_btext(f->wd, bg, fg, 0, i * ch, text + i + cnt % COLS);
		win_end_paint();
		fps++;
		
		time(&ct);
		if (pt != ct)
		{
			sprintf(buf, "FPS: %i    ", fps);
			win_paint();
			win_btext(f->wd, bg, fg, 4, h - ch, buf);
			win_end_paint();
			fps = 0;
			pt = ct;
			cnt++;
		}
	}
}
