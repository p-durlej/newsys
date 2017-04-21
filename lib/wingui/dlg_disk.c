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
#include <wingui_color.h>
#include <wingui_form.h>
#include <wingui_dlg.h>
#include <wingui.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <bioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

static const char *disk_type_descr[] =
{
	[BIO_TYPE_FD]		= "3.5\" Floppy Disk Drive",
	[BIO_TYPE_HD]		= "Hard Disk",
	[BIO_TYPE_HD_SECTION]	= "Disk Partition",
	[BIO_TYPE_CD]		= "CD-ROM",
	[BIO_TYPE_SSD]		= "Solid State Disk",
};

struct part
{
	char	dev[NAME_MAX + 1];
	int	type;
};

struct dlg_disk
{
	int		part_cnt;
	struct part *	part;
	int		flags;
	const char *	title;
	struct form *	form;
	struct gadget *	list;
};

static int dlg_disk_should_show(struct dlg_disk *d, const struct bio_info *bi)
{
	int mask = DLG_DISK_ANY;
	
	switch (bi->type)
	{
	case BIO_TYPE_FD:
		mask |= DLG_DISK_FLOPPY;
	case BIO_TYPE_HD:
		mask |= DLG_DISK_HARD;
	case BIO_TYPE_HD_SECTION:
		mask |= DLG_DISK_PARTITION;
	case BIO_TYPE_CD:
		mask |= DLG_DISK_CD;
	case BIO_TYPE_SSD:
		mask |= DLG_DISK_SSD;
	default:
		;
	}
	return !!(d->flags & mask);
}

static int dlg_disk_cmp(const void *p1, const void *p2)
{
	const struct part *it1 = p1;
	const struct part *it2 = p2;
	
	return strcmp(it1->dev, it2->dev);
}

static void dlg_disk_load(struct dlg_disk *d)
{
	char pathname[PATH_MAX];
	struct dirent *de;
	struct part *part;
	struct bio_info bi;
	char msg[256];
	DIR *dir;
	int fd;
	int cnt;
	
	dir = opendir("/dev");
	if (!dir)
	{
		sprintf(msg, "Unable to open /dev:\n\n%m");
		msgbox(d->form, d->title, msg);
		return;
	}
	
	cnt = 0;
	while ((de = readdir(dir)))
	{
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;
		
		sprintf(pathname, "/dev/%s", de->d_name);
		fd = open(pathname, O_RDWR);
		
		if (!ioctl(fd, BIO_INFO, &bi) && dlg_disk_should_show(d, &bi))
			cnt++;
		
		close(fd);
	}
	part = calloc(cnt, sizeof *part);
	
	list_clear(d->list);
	rewinddir(dir);
	cnt = 0;
	while ((de = readdir(dir)))
	{
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;
		
		sprintf(pathname, "/dev/%s", de->d_name);
		fd = open(pathname, O_RDWR);
		
		if (!ioctl(fd, BIO_INFO, &bi) && dlg_disk_should_show(d, &bi))
		{
			strcpy(part[cnt].dev, de->d_name);
			part[cnt].type = bi.type;
			
			list_newitem(d->list, (void *)&part[cnt]);
			cnt++;
		}
		
		close(fd);
	}
	closedir(dir);
	
	qsort(part, cnt, sizeof *part, dlg_disk_cmp);
	
	d->part_cnt = cnt;
	d->part = part;
}

static void dlg_disk_draw_item(struct gadget *g, int index, int wd, int x, int y, int w, int h)
{
	struct part *it = (void *)list_get_text(g, index);
	win_color bg, fg;
	const char *s = NULL;
	int indent = 0;
	int cw, ch;
	
	if (list_get_index(g) == index)
	{
		bg = wc_get(WC_SEL_BG);
		fg = wc_get(WC_SEL_FG);
	}
	else
	{
		bg = wc_get(WC_BG);
		fg = wc_get(WC_FG);
	}
	
	if (it->type <= sizeof disk_type_descr / sizeof *disk_type_descr)
		s = disk_type_descr[it->type];
	if (!s)
		s = "";
	
	win_chr_size(WIN_FONT_DEFAULT, &cw, &ch, 'X');
	
	if (it->type == BIO_TYPE_HD_SECTION)
		indent = cw;
	
	win_rect(wd, bg, x, y, w, h);
	win_btext(wd, bg, fg, x + 11 * cw, y, s);
	win_btext(wd, bg, fg, x + 2 + indent, y, it->dev);
}

const char *dlg_disk(struct form *pf, const char *title, const char *pathname, int flags)
{
	static char buf[PATH_MAX];
	
	char msg[PATH_MAX + 64];
	struct dlg_disk d;
	struct stat st;
	const char *p;
	int i;
	
	if (flags & DLG_DISK_ANY)
		flags = -1U;
	
	if (!title)
		title = "Select Block Device";
	
	if (!pathname)
		pathname = "/dev/fd0,0";
	
	if (strlen(pathname) >= sizeof buf)
		pathname = "/dev/fd0,0";
	
	bzero(&d, sizeof d);
	d.title = title;
	d.flags = flags;
	
	strcpy(buf, pathname);
	
	d.form = form_load("/lib/forms/blockdev.frm");
	d.list = gadget_find(d.form, "list");
	list_set_draw_cb(d.list, dlg_disk_draw_item);
	list_set_flags(d.list, LIST_FRAME | LIST_VSBAR);
	form_set_title(d.form, title);
	
	dlg_disk_load(&d);
	
	p = strrchr(pathname, '/');
	if (p)
	{
		p++;
		for (i = 0; i < d.part_cnt; i++)
			if (!strcmp(p, d.part[i].dev))
			{
				list_set_index(d.list, i);
				break;
			}
	}
	
	if (pf)
		form_set_dialog(pf, d.form);
	for (;;)
	{
		gadget_focus(d.list);
		switch (form_wait(d.form))
		{
		case 1:
			p = list_get_text(d.list, list_get_index(d.list));
			if (!p)
			{
				msgbox(d.form, title, "Select a device.");
				continue;
			}
			sprintf(buf, "/dev/%s", p);
			
			if (stat(buf, &st))
			{
				int se = _get_errno();
				
				sprintf(msg, "Cannot stat %s", buf);
				msgbox_perror(d.form, title, msg, se);
				continue;
			}
			
			if (!S_ISBLK(st.st_mode))
			{
				msgbox(d.form, title, "Not a block device.");
				continue;
			}
			
			if (pf)
				form_set_dialog(pf, NULL);
			form_close(d.form);
			return buf;
		case 2:
			if (pf)
				form_set_dialog(pf, NULL);
			form_close(d.form);
			return NULL;
		}
	}
}
