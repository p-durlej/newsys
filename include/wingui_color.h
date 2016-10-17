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

#ifndef _WINGUI_COLOR_H
#define _WINGUI_COLOR_H

#include <wingui.h>

#define WC_COUNT		62

#define WC_DESKTOP		0
#define WC_WIN_BG		1
#define WC_WIN_FG		2
#define WC_BG			3
#define WC_FG			4
#define WC_SEL_BG		5
#define WC_SEL_FG		6
#define WC_CUR_BG		7
#define WC_CUR_FG		8
#define WC_HIGHLIGHT1		9
#define WC_HIGHLIGHT2		10
#define WC_SHADOW1		11
#define WC_SHADOW2		12
#define WC_TITLE_BG		13
#define WC_TITLE_FG		14
#define WC_CURSOR		15
#define WC_SBAR			16
#define WC_DESKTOP_FG		17
#define WC_TITLE_SH1		18
#define WC_TITLE_SH2		19
#define WC_TITLE_HI1		20
#define WC_TITLE_HI2		21
#define WC_NFSEL_FG		22
#define WC_NFSEL_BG		23
#define WC_INACT_TITLE_BG	24
#define WC_INACT_TITLE_FG	25
#define WC_INACT_TITLE_SH1	26
#define WC_INACT_TITLE_SH2	27
#define WC_INACT_TITLE_HI1	28
#define WC_INACT_TITLE_HI2	29
#define WC_TITLE_BTN_BG		30
#define WC_TITLE_BTN_FG		31
#define WC_TITLE_BTN_SH1	32
#define WC_TITLE_BTN_SH2	33
#define WC_TITLE_BTN_HI1	34
#define WC_TITLE_BTN_HI2	35
#define WC_INACT_TITLE_BTN_BG	36
#define WC_INACT_TITLE_BTN_FG	37
#define WC_INACT_TITLE_BTN_SH1	38
#define WC_INACT_TITLE_BTN_SH2	39
#define WC_INACT_TITLE_BTN_HI1	40
#define WC_INACT_TITLE_BTN_HI2	41
#define WC_MENU_UNDERLINE	42
#define WC_FRAME_SH1		43
#define WC_FRAME_SH2		44
#define WC_FRAME_HI1		45
#define WC_FRAME_HI2		46
#define WC_FRAME_BG		47
#define WC_INACT_FRAME_SH1	48
#define WC_INACT_FRAME_SH2	49
#define WC_INACT_FRAME_HI1	50
#define WC_INACT_FRAME_HI2	51
#define WC_INACT_FRAME_BG	52
#define WC_BUTTON_BG		53
#define WC_BUTTON_FG		54
#define WC_BUTTON_SH1		58
#define WC_BUTTON_SH2		59
#define WC_BUTTON_HI1		60
#define WC_BUTTON_HI2		61

void	  wc_get_rgba(int id, struct win_rgba *buf);
win_color wc_get(int id);
void	  wc_load(void);
void	  wc_load_cte(void);

#endif
