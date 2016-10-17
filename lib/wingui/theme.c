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

#include <priv/wingui_theme.h>
#include <config/defaults.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

static const struct form_theme *form_th_cur;

int form_th_loaded;

const struct form_theme std_theme;

const struct form_theme *form_th_get(void)
{
	if (!form_th_loaded)
		form_th_reload();
	return form_th_cur;
}

void form_th_reload(void)
{
	char name[NAME_MAX + 1] = "";
	char pathname[PATH_MAX];
	char buf[256];
	char *p;
	FILE *f;
	
	form_th_cur    = &std_theme;
	form_th_loaded = 1;
	
	if (c_load("/user/w_theme", name, sizeof name - 1))
		strcpy(name, DEFAULT_THEME);
	
	sprintf(pathname, "/lib/w_themes/%s/info", name);
	
	f = fopen(pathname, "r");
	if (f)
	{
		while (fgets(buf, sizeof buf, f))
		{
			p = strchr(buf, '\n');
			if (p)
				*p = 0;
			
			switch (*buf)
			{
			case 'W':
				if (!strcmp(buf + 1, "flat"))
				{
					form_th_cur = &fl_theme;
					break;
				}
				break;
			default:
				;
			}
		}
		fclose(f);
	}
}
