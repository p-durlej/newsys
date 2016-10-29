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

#include <config/defaults.h>
#include <wingui_msgbox.h>
#include <wingui_color.h>
#include <wingui_form.h>
#include <sys/stat.h>
#include <dirent.h>
#include <wingui.h>
#include <stdlib.h>
#include <stdio.h>
#include <confdb.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <paths.h>
#include <err.h>

#define MAIN_FORM	"/lib/forms/color.frm"

static struct gadget *main_list;
static struct form *main_form;

static char **names;
static int name_cnt;

static int scolt(const char *cp)
{
	char name[NAME_MAX + 1];
	char *p1;
	char *p;
	char *n;
	
	p = strdup(cp);
	if (!p)
		return -1;
	
	p1 = strchr(p, '/');
	if (p1)
		n = p1 + 1;
	else
		n = p;
	
	memset(name, 255, sizeof name);
	strcpy(name, n);
	
	c_save("w_colors", name, sizeof name);
	wc_load_cte();
	win_update();
	win_redraw_all();
	return 0;
}

static int update(void)
{
	int i;
	
	i = list_get_index(main_list);
	if (i < 0)
		return -1;
	return scolt(list_get_text(main_list, i));
}

static void load_names(const char *path, int complain)
{
	char pathname[PATH_MAX];
	struct dirent *de;
	struct stat st;
	DIR *d;
	
	d = opendir(path);
	if (!d)
	{
		if (complain)
		{
			msgbox(main_form, "Colors", "Could not open scheme list.");
			exit(errno);
		}
		return;
	}
	
	while ((de = readdir(d)))
	{
		sprintf(pathname, "%s/%s", path, de->d_name);
		
		if (stat(pathname, &st) || !S_ISREG(st.st_mode))
			continue;
		
		names = realloc(names, ++name_cnt * sizeof *names);
		names[name_cnt - 1] = strdup(de->d_name);
	}
	
	closedir(d);
}

static int namecmp(const void *p1, const void *p2)
{
	return strcmp(*(char **)p1, *(char **)p2);
}

int main(int argc, char **argv)
{
	char curr[NAME_MAX + 1];
	char path[PATH_MAX];
	char *home;
	int i, n;
	
	if (win_attach())
		err(255, NULL);
	
	if (argc > 1)
	{
		if (scolt(argv[1]))
			return errno;
		return 0;
	}
	
	if (c_load("w_colors", &curr, sizeof curr))
	{
		struct win_modeinfo mi;
		
		if (win_getmode(&i) || win_modeinfo(i, &mi) || mi.ncolors < 256)
			strcpy(curr, "Low Color Default");
		else
			strcpy(curr, DEFAULT_COLORS);
	}
	
	main_form = form_load(MAIN_FORM);
	main_list = gadget_find(main_form, "list");
	
	gadget_focus(main_list);
	
	home = getenv("HOME");
	if (!home)
		home = _PATH_H_DEFAULT;
	
	sprintf(path, "%s/.wc", home);
	
	load_names("/etc/w_color", 1);
	load_names(path, 0);
	
	qsort(names, name_cnt, sizeof *names, namecmp);
	
	n = name_cnt;
	for (i = n = 1; i < name_cnt; i++, n++)
	{
		if (!strcmp(names[i - 1], names[i]))
			n--;
		names[n] = names[i];
	}
	name_cnt = n;
	
	for (i = 0; i < name_cnt; i++)
		list_newitem(main_list, names[i]);
	
	for (i = 0; i < name_cnt; i++)
		if (!strcmp(list_get_text(main_list, i), curr))
			list_set_index(main_list, i);
	
	while (form_wait(main_form) == 1 && update());
	return 0;
}
