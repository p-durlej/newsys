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
#include <confdb.h>
#include <string.h>
#include <errno.h>

typedef int wm_tab_t[WM_COUNT];

static wm_tab_t	wm_tab;
static int	wm_loaded;

static const int wm_default_hidpi_tab[] =
{
	[WM_DOUBLECLICK] = 6,
	[WM_FRAME]	 = 10,
	[WM_SCROLLBAR]	 = 24,
	[WM_THIN_LINE]	 = 2,
};

static const int wm_default_tab[] =
{
	[WM_DOUBLECLICK] = 3,
	[WM_FRAME]	 = 5,
	[WM_SCROLLBAR]	 = 12,
	[WM_THIN_LINE]	 = 1,
};

static void wm_default(void)
{
	if (win_get_dpi_class() > 0)
		memcpy(&wm_tab, &wm_default_hidpi_tab, sizeof wm_tab);
	else
		memcpy(&wm_tab, &wm_default_tab, sizeof wm_tab);
}

int wm_get(int sel)
{
	if (sel < 0 || sel >= WM_COUNT)
	{
		_set_errno(EINVAL);
		return -1;
	}
	wm_load();
	return wm_tab[sel];
}

int wm_load(void)
{
	if (wm_loaded)
		return;
	if (c_load("metrics", &wm_tab, sizeof wm_tab))
		wm_default();
	wm_loaded = 1;
}
