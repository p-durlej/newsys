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

#include <kern/bootfs.h>
#include <kern/printk.h>
#include <kern/devfs.h>
#include <kern/ptyfs.h>
#include <kern/natfs.h>
#include <kern/errno.h>
#include <kern/block.h>
#include <kern/ptyfs.h>
#include <kern/event.h>
#include <kern/stat.h>
#include <kern/task.h>
#include <kern/page.h>
#include <kern/lib.h>
#include <kern/fs.h>

struct fstype *	fs_fstype[FS_MAXFSTYPE];
struct fso *	fs_fso;
struct fs	fs_fs[FS_MAXFS];
int		fs_fso_high;

void fs_init(void)
{
	void pipe_init(void);
	
	vpage npg = (sizeof *fs_fso * FS_MAXFSO + PAGE_SIZE - 1) >> PAGE_SHIFT;
	vpage pg;
	
	if (pg_adget(&pg, npg))
		panic("fs_init: pg_adget failed");
	
	fs_fso = pg2vap(pg);
	
	strcpy(curr->cwd, "/");
	
	devfs_init();
	bfs_init();
	nat_init();
	pty_init();
	pipe_init();
}

int fs_fqname(char *buf, const char *path)
{
	const char *src = path;
	char *dst = buf;
	char *sep;
	int l;
	
	*dst = 0;
	
	if (*src != '/')
	{
		if (curr->cwd[1])
		{
			strcpy(dst, curr->cwd);
			dst = buf + strlen(curr->cwd);
		}
	}
	else
		src++;
	
	while (*src)
	{
		while (*src == '/')
			src++;
		
		sep = strchr(src, '/');
		if (sep)
			l = sep - src;
		else
			l = strlen(src);
		
		if (!l)
			goto next;
		if (l == 1 && src[0] == '.')
			goto next;
		
		if (l == 2 && src[0] == '.' && src[1] == '.')
		{
			char *prev = strrchr(buf, '/');
			
			if (!prev)
				goto next;
			
			dst = prev;
			*dst = 0;
			
			goto next;
		}
		
		if (dst - buf + l + 1 >= PATH_MAX)
			return ENAMETOOLONG;
		
		*dst = '/';
		dst++;
		memcpy(dst, src, l);
		dst += l;
		*dst = 0;
next:
		src += l;
	}
	
	if (!*buf)
		strcpy(buf, "/");
	
	if (!strcmp(buf, "/dev/tty") && *curr->tty)
		strcpy(buf, curr->tty);
	
	return 0;
}

int fs_pathfs(struct fs **fs, int *prefix_l, const char *pathname)
{
	struct fs *f = NULL;
	int path_l = strlen(pathname);
	int l = 0;
	int i;
	
	for (i = 0; i < FS_MAXFS; i++)
	{
		if (!fs_fs[i].in_use)
			continue;
		
		if (path_l < fs_fs[i].prefix_l)
			continue;
		
		if (fs_fs[i].prefix_l < l)
			continue;
		
		if (memcmp(fs_fs[i].prefix, pathname, fs_fs[i].prefix_l))
			continue;
		
		if (pathname[fs_fs[i].prefix_l] && pathname[fs_fs[i].prefix_l] != '/')
			continue;
		
		f = &fs_fs[i];
		l = fs_fs[i].prefix_l;
	}
	
	if (!f)
		return ENOENT;
	
	while (pathname[l] == '/')
		l++;
	
	*fs	  = f;
	*prefix_l = l;
	return 0;
}

int fs_parsename4(struct fs **fs, char *buf, char *fqbuf, const char *pathname)
{
	char fqname[PATH_MAX];
	int prefix_l;
	int err;
	
	err = fs_fqname(fqname, pathname);
	if (err)
		return err;
	
	err = fs_pathfs(fs, &prefix_l, fqname);
	if (err)
		return err;
	
	err = fs_activate(*fs);
	if (err)
		return err;
	
	if (fqbuf)
		strcpy(fqbuf, fqname);
	
	strcpy(buf, fqname + prefix_l);
	return 0;
}

int fs_parsename(struct fs **fs, char *buf, const char *pathname)
{
	return fs_parsename4(fs, buf, NULL, pathname);
}

int fs_install(struct fstype *fstype)
{
	int i;
	
	for (i = 0; i < FS_MAXFSTYPE; i++)
		if (!fs_fstype[i])
		{
			fs_fstype[i] = fstype;
			return 0;
		}
	
	return ENOMEM;
}

int fs_uinstall(struct fstype *fstype)
{
	int i;
	
	for (i = 0; i < fs_fso_high; i++)
		if (fs_fso[i].refcnt && fs_fso[i].fs->type == fstype)
			return EBUSY;
	
	for (i = 0; i < FS_MAXFSTYPE; i++)
		if (fs_fstype[i] == fstype)
		{
			fs_fstype[i] = NULL;
			return 0;
		}
	
	panic("fs_uinstall: filesystem type not installed");
}

int fs_find(struct fstype **fstype, const char *name)
{
	int i;
	
	for (i = 0; i < FS_MAXFSTYPE; i++)
		if (fs_fstype[i] && !strcmp(fs_fstype[i]->name, name))
		{
			*fstype = fs_fstype[i];
			return 0;
		}
	
	return EINVAL;
}

int fs_getfso(struct fso **fso, struct fs *fs, ino_t index)
{
	struct fso *n = NULL;
	struct fso *p;
	int err;
	int i;
	
	for (i = 0; i < fs_fso_high; i++)
	{
		p = &fs_fso[i];
		
		if (!p->refcnt)
		{
			n = p;
			continue;
		}
		
		if (index != -1 && p->index == index && p->fs == fs)
		{
			*fso = &fs_fso[i];
			p->refcnt++;
			return 0;
		}
	}
	
	if (!n)
	{
		if (fs_fso_high >= FS_MAXFSO)
			return ENOMEM;
		
		vpage pg = vap2pg(&fs_fso[fs_fso_high + 1]); // XXX
		
		err = pg_atmem(pg, 1, 1);
		if (err)
			return err;
		
		n = &fs_fso[fs_fso_high++];
	}
	
	memset(n, 0, sizeof *n);
	n->index  = index;
	n->refcnt = 1;
	n->fs	  = fs;
	
	err = fs->type->getfso(n);
	if (err)
	{
		n->refcnt = 0;
		return err;
	}
	
	*fso = n;
	return 0;
}

int fs_putfso(struct fso *fso)
{
	if (!fso)
		return 0;
	
	if (!fso->refcnt)
		panic("fs_putfso: !fso->refcnt");
	
	if (fso->refcnt == 1)
		fso->fs->type->putfso(fso);
	fso->refcnt--;
	
	return 0;
}

void fs_setstate(struct fso *fso, int state)
{
	int s;
	
	s = intr_dis();
	fs_state(fso, fso->state | state);
	intr_res(s);
}

void fs_clrstate(struct fso *fso, int state)
{
	int s;
	
	s = intr_dis();
	fs_state(fso, fso->state & ~state);
	intr_res(s);
}

void fs_state(struct fso *f, int state)
{
	struct task *t;
	int s;
	
	s = intr_dis();
	if (f->state == state)
	{
		intr_res(s);
		return;
	}
	f->state = state;
	intr_res(s);
	
	if (f->polling != NULL)
	{
		task_resume(f->polling);
		return;
	}
	if (f->pollcnt)
		while (t = task_dequeue(&fs_pollq), t != NULL)
			task_resume(t);
}

int fs_sync(struct fs *fs)
{
	struct fso *p;
	int lasterr;
	int err = 0;
	int i;
	
	if (fs->read_only)
		return 0;
	
	for (i = 0; i < fs_fso_high; i++)
	{
		p = &fs_fso[i];
		if (p->fs == fs && p->refcnt && p->dirty)
		{
			lasterr = fs->type->syncfso(p);
			if (!err)
				err = lasterr;
		}
	}
	
	lasterr = fs->type->sync(fs);
	if (!err)
		err = lasterr;
	
	return err;
}

int fs_syncall(void)
{
	int lasterr;
	int err = 0;
	int i;
	
	for (i = 0; i < FS_MAXFS; i++)
		if (fs_fs[i].in_use)
		{
			lasterr = fs_sync(&fs_fs[i]);
			if (!err)
				err = lasterr;
		}
	
	return err;
}

int fs_lookup(struct fso **fso, const char *pathname)
{
	char dname[PATH_MAX];
	struct fs *fs;
	int err;
	
	err = fs_parsename(&fs, dname, pathname);
	if (err)
		return err;
	
	return fs->type->lookup(fso, fs, dname);
}

void fs_newtask(struct task *p)
{
	int i;
	
	memcpy(&p->file_desc, &curr->file_desc, sizeof p->file_desc);
	strcpy(p->cwd, curr->cwd);
	strcpy(p->tty, curr->tty);
	p->ocwd = curr->ocwd;
	
	for (i = 0; i < OPEN_MAX; i++)
		if (p->file_desc[i].file)
			p->file_desc[i].file->refcnt++;
}

void fs_clean(void)
{
	int i;
	
	curr->multipoll = 0;
	
	for (i = 0; i < OPEN_MAX; i++)
		if (curr->file_desc[i].file)
			fs_putfd(i);
}

void fs_notify(const char *pathname)
{
	struct event e;
	struct task *t;
	char *p;
	int i;
	
	p = strrchr(pathname, '/');
	if (!p)
		panic("fs_notify: !p");
	
	memset(&e, 0, sizeof e);
	e.type = E_FNOTIF;
	
	for (i = 0; i < TASK_MAX; i++)
	{
		t = task[i];
		
		if (!t || !t->ocwd)
			continue;
		
		if (memcmp(t->cwd, pathname, p - pathname))
			continue;
		
		if (t->cwd[p - pathname])
			continue;
		
		send_event(t, &e);
	}
}
