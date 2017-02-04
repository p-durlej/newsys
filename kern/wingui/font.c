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

#include <kern/wingui.h>
#include <kern/printk.h>
#include <kern/task.h>
#include <kern/lib.h>
#include <kern/fs.h>
#include <sys/stat.h>
#include <overflow.h>
#include <errno.h>

struct win_font win_font[FONT_MAX];

static int init_font(struct win_font *f, int size)
{
	struct win_glyph *gl = (struct win_glyph *)f->data;
	unsigned totw = 0;
	unsigned bmps;
	unsigned i;
	
	while (gl->nr != (unsigned)-1)
	{
		if ((char *)(gl + 1) > f->data + size)
			return ENOEXEC;
		
		if (gl->nr >= sizeof f->glyph / sizeof *f->glyph)
			return ENOEXEC;
		f->glyph[gl->nr] = gl;
		
		if (ov_add_u(gl->pos, gl->width))
			return ENOEXEC;
		
		if (gl->pos + gl->width > totw)
			totw += gl->width;
		gl++;
	}
	for (i = 0; i < sizeof f->glyph / sizeof *f->glyph; i++)
		if (f->glyph[i] == NULL)
			f->glyph[i] = (struct win_glyph *)f->data;
	
	totw +=  31;
	totw &= ~31;
	
	bmps = totw * f->glyph[0]->height / 8;
	if (ov_mul_u(totw, f->glyph[0]->height))
		return ENOEXEC;
	if (gl + 1 + bmps < gl + 1)
		return ENOEXEC;
	if ((char *)(gl + 1) + bmps > f->data + size)
		return ENOEXEC;
	
	f->bitmap  = (uint8_t *)(gl + 1);
	f->linelen = totw / 8;
	
	return 0;
}

int win_chk_ftd(int ftd)
{
	struct win_desktop *d = curr->win_task.desktop;
	
	if (!d)
		return ENODESKTOP;
	
	if (ftd == -1)
		ftd = DEFAULT_FTD;
	
	if (ftd < 0 || ftd >= FONT_MAX)
		return EINVAL;
	
	ftd = d->font_map[ftd];
	
	if (win_font[ftd].data)
		return 0;
	
	return EINVAL;
}

int win_load_font(int *ftd, const char *name, const char *pathname)
{
	struct fs_rwreq rq;
	struct fso *f;
	int err;
	int i;
	
	for (i = 0; i < FONT_MAX; i++)
		if (!win_font[i].data)
		{
			memset(&win_font[i], 0, sizeof win_font[i]);
			
			err = fs_lookup(&f, pathname);
			if (err)
				return err;
			
			if (!S_ISREG(f->mode))
			{
				fs_putfso(f);
				return EINVAL;
			}
			
			if (!f->size)
			{
				printk("win_load_font: !f->size\n");
				fs_putfso(f);
				return ENOEXEC;
			}
			
			err = kmalloc(&win_font[i].data, f->size, "font");
			if (err)
			{
				fs_putfso(f);
				return err;
			}
			
			rq.fso	  = f;
			rq.offset = 0;
			rq.count  = f->size;
			rq.buf	  = win_font[i].data;
			err = f->fs->type->read(&rq);
			if (err)
			{
				free(win_font[i].data);
				win_font[i].data = NULL;
				fs_putfso(f);
				return err;
			}
			
			err = init_font(&win_font[i], f->size);
			if (err)
			{
				free(win_font[i].data);
				win_font[i].data = NULL;
				fs_putfso(f);
				return err;
			}
			
			strcpy(win_font[i].name, name);
			fs_putfso(f);
			*ftd = i;
			return 0;
		}
	
	return ENOMEM;
}

int win_find_font(int *ftd, const char *name)
{
	struct win_desktop *d = curr->win_task.desktop;
	int i, n;
	
	if (!d)
		return ENODESKTOP;
	
	for (i = 0; i < FONT_MAX; i++)
	{
		n = d->font_map[i];
		
		if (win_font[n].data && !strcmp(win_font[n].name, name))
		{
			*ftd = i;
			return 0;
		}
	}
	
	return ENOENT;
}

int win_set_font(int wd, int ftd)
{
	struct win_desktop *d = curr->win_task.desktop;
	int err;
	
	err = win_chkwd(wd);
	if (err)
		return err;
	
	if (ftd == WIN_FONT_DEFAULT)
		ftd = DEFAULT_FTD;
	
	err = win_chk_ftd(ftd);
	if (err)
		return err;
	
	d->window[wd].font = d->font_map[ftd];
	return 0;
}
