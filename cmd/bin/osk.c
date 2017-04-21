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

#include <wingui_msgbox.h>
#include <wingui_form.h>
#include <wingui.h>
#include <confdb.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <err.h>

#include <priv/osk.h>

#define LAYOUT	"/lib/osk_layout"

#define ROWS	5

static struct osk_conf config;

static int btn_width  = 64;
static int btn_height = 64;
static int btn_x, btn_y;
static int form_width;
static struct form *f;
static unsigned shift;

static int x_num, x_den;
static int y_num, y_den;

static void update_title(void)
{
	char title[256];
	
	strcpy(title, "Keyboard");
	if (shift & WIN_SHIFT_SHIFT)
		strcat(title, " [SHIFT]");
	if (shift & WIN_SHIFT_CTRL)
		strcat(title, " [CTRL]");
	if (shift & WIN_SHIFT_ALT)
		strcat(title, " [ALT]");
	form_set_title(f, title);
}

static void btn_click(struct gadget *g, int x, int y)
{
	unsigned ch = g->l_data;
	
	switch (ch)
	{
	case WIN_KEY_SHIFT:
		shift ^= WIN_SHIFT_SHIFT;
		update_title();
		break;
	case WIN_KEY_CTRL:
		shift ^= WIN_SHIFT_CTRL;
		update_title();
		break;
	case WIN_KEY_ALT:
		shift ^= WIN_SHIFT_ALT;
		update_title();
		break;
	default:
		if (shift & WIN_SHIFT_SHIFT)
		{
			win_soft_keyup(WIN_KEY_SHIFT);
			if (ch >= 'a' && ch <= 'z')
			{
				ch -= 'a';
				ch += 'A';
			}
		}
		if (shift & WIN_SHIFT_CTRL)
		{
			win_soft_keyup(WIN_KEY_CTRL);
			if ((ch >= 'a' && ch <= 'z') || ch == ' ')
				ch &= 0x1f;
		}
		if (shift & WIN_SHIFT_ALT)
			win_soft_keyup(WIN_KEY_ALT);
		
		win_soft_keydown(ch);
		win_soft_keyup(ch);
		
		if (shift & WIN_SHIFT_SHIFT)
			win_soft_keyup(WIN_KEY_SHIFT);
		if (shift & WIN_SHIFT_CTRL)
			win_soft_keyup(WIN_KEY_CTRL);
		if (shift & WIN_SHIFT_ALT)
			win_soft_keyup(WIN_KEY_ALT);
		if (shift)
		{
			shift = 0;
			update_title();
		}
	}
}

static void add_key(char *str)
{
	struct gadget *g;
	unsigned c;
	
	if (!strcmp(str, "TAB"))
		c = '\t';
	else if (!strcmp(str, "BS"))
		c = '\b';
	else if (!strcmp(str, "RET"))
		c = '\n';
	else if (!strcmp(str, "SPACE"))
		c = ' ';
	else if (!strcmp(str, "SHIFT"))
		c = WIN_KEY_SHIFT;
	else if (!strcmp(str, "CTRL"))
		c = WIN_KEY_CTRL;
	else if (!strcmp(str, "ALT"))
		c = WIN_KEY_ALT;
	else
		c = *str;
	
	g = button_creat(f, btn_x, btn_y, btn_width, btn_height, str, btn_click);
	g->l_data = c;
	
	btn_x += btn_width;
	if (btn_x > form_width)
		form_width = btn_x;
}

static void do_command(char *buf)
{
	while (isspace(*buf))
		buf++;
	if (*buf == '#' || !*buf)
		return;
	if (!strncmp(buf, "space=", 6))
	{
		btn_x += atoi(buf + 6) * x_num / x_den;
		return;
	}
	if (!strncmp(buf, "height=", 7))
	{
		btn_height  = atoi(buf + 7);
		btn_height *= y_num;
		btn_height /= y_den;
		return;
	}
	if (!strncmp(buf, "width=", 6))
	{
		btn_width = atoi(buf + 6);
		btn_width *= x_num;
		btn_width /= x_den;
		return;
	}
	if (!strncmp(buf, "key=", 4))
	{
		add_key(buf + 4);
		return;
	}
	if (!strcmp(buf, "newline"))
	{
		btn_y += btn_height;
		btn_x = 0;
		return;
	}
	msgbox(NULL, "Keyboard", "Invalid layout file.");
	exit(127);
}

static void load_layout(void)
{
	char buf[80];
	char *p;
	FILE *f;
	
	f = fopen(LAYOUT, "r");
	if (!f)
	{
		msgbox_perror(NULL, "Keyboard", "Cannot open layout file", errno);
		exit(errno);
	}
	while (fgets(buf, sizeof buf, f))
	{
		p = strchr(buf, '\n');
		if (p)
			*p = 0;
		do_command(buf);
	}
	fclose(f);
}

static void adjust_scale(void)
{
	win_chr_size(WIN_FONT_DEFAULT, &x_num, &y_num, 'M');
	x_den = 6;
	y_den = 11;
}

int main(int argc, char **argv)
{
	int taskbar_mode = 0;
	struct win_rect wsr;
	struct gadget *g;
	int flags;
	int i;
	
	if (argc == 2 && !strcmp(argv[1], "--taskbar"))
		taskbar_mode = 1;
	
	for (i = 1; i <= SIGMAX; i++)
		signal(i, SIG_DFL);
	
	if (win_attach())
		err(255, NULL);
	
	win_ws_getrect(&wsr);
	adjust_scale();
	
	if (c_load("osk", &config, sizeof config))
		config.floating = 1;
	if (config.floating)
		flags = FORM_FRAME | FORM_TITLE | FORM_ALLOW_CLOSE;
	else
		flags = 0;
	
	f = form_creat(FORM_NO_FOCUS | FORM_EXCLUDE_FROM_LIST | flags, 0, 10, 10, 320, 240, "Keyboard");
	load_layout();
	if (btn_x)
		btn_y += btn_height;
	form_resize(f, form_width, btn_y);
	if (config.floating)
	{
		form_move(f, wsr.w - f->win_rect.w - 10, wsr.h - f->win_rect.h - 10);
		form_resize(f, form_width, btn_y);
	}
	else
	{
		for (g = gadget_first(f); g; g = gadget_next(g))
			g->rect.x += (wsr.w - form_width) / 2;
		form_move(f, wsr.x, wsr.h - f->win_rect.h);
		form_resize(f, wsr.w, btn_y);
	}
	
	if (taskbar_mode)
	{
		if (config.floating)
			printf("0\n");
		else
			printf("%i\n", btn_y);
	}
	
	form_set_layer(f, WIN_LAYER_TASKBAR);
	form_raise(f);
	form_wait(f);
	return 0;
}
