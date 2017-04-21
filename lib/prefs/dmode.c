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
#include <prefs/dmode.h>
#include <string.h>
#include <confdb.h>

static struct dmode dm_mode;
static int dm_mode_loaded;

struct dmode *dm_get(void)
{
	if (!dm_mode_loaded)
	{
		if (c_load("/desk/mode", &dm_mode, sizeof dm_mode))
		{
			memset(&dm_mode, 0, sizeof dm_mode);
			
			dm_mode.xres  = -1;
			dm_mode.yres  = -1;
			dm_mode.nclr  = -1;
			dm_mode.nr    = -1;
			dm_mode.hidpi = DEFAULT_LARGE_UI;
		}
		dm_mode_loaded = 1;
	}
	
	return &dm_mode;
}

int dm_save(void)
{
	if (!dm_get())
		return -1;
	
	return c_save("/desk/mode", &dm_mode, sizeof dm_mode);
}
