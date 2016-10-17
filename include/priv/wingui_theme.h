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

#ifndef _PRIV_WINGUI_THEME_H
#define _PRIV_WINGUI_THEME_H

#define DB_FOCUS	1
#define DB_DEPSD	2

struct form_theme
{
	int	(*d_button)(int wd, int x, int y, int w, int h, const char *label, int flags);
	int	(*d_closebtn)(int wd, int x, int y, int w, int h, int flags);
	int	(*d_zoombtn)(int wd, int x, int y, int w, int h, int flags);
	int	(*d_minibtn)(int wd, int x, int y, int w, int h, int flags);
	int	(*d_menubtn)(int wd, int x, int y, int w, int h, int flags);
	int	(*d_menuframe)(int wd, int x, int y, int w, int h);
	int	(*d_formframe)(int wd, int x, int y, int w, int h, int th, int act);
	int	(*d_formbg)(int wd, int x, int y, int w, int h);
	int	(*d_edboxframe)(int wd, int x, int y, int w, int h, int th, int act);
	
	int	edbox_shadow_w;
	int	form_shadow_w;
};

extern const struct form_theme std_theme;
extern const struct form_theme bl_theme;
extern const struct form_theme fl_theme;

const struct form_theme *form_th_get(void);
void form_th_reload(void);

#endif
