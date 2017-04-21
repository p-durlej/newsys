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

#include <wingui_metrics.h>
#include <wingui.h>
#include <config/defaults.h>
#include <prefs/filemgr.h>
#include <string.h>
#include <confdb.h>

static struct pref_filemgr pref_filemgr;

int pref_filemgr_loaded;

void pref_filemgr_reload(void)
{
	int tl;
	
	if (c_load("/user/filemgr", &pref_filemgr, sizeof pref_filemgr))
	{
		memset(&pref_filemgr, 0, sizeof pref_filemgr);
		
		tl = wm_get(WM_THIN_LINE);
		
		pref_filemgr.show_dotfiles	= 0;
		pref_filemgr.form_w		= 332 * tl;
		pref_filemgr.form_h		= 150 * tl;
		pref_filemgr.show_path		= 0;
		pref_filemgr.large_icons	= win_get_dpi_class();
		pref_filemgr.double_click	= DEFAULT_DOUBLE_CLICK;
	}
	pref_filemgr_loaded = 1;
}

struct pref_filemgr *pref_filemgr_get(void)
{
	if (!pref_filemgr_loaded)
		pref_filemgr_reload();
	
	return &pref_filemgr;
}

int pref_filemgr_save(void)
{
	if (!pref_filemgr_get())
		return -1;
	
	return c_save("/user/filemgr", &pref_filemgr, sizeof pref_filemgr);
}
