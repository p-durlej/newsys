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

#include <kern/errno.h>
#include <kern/fs.h>
#include <kern/bootfs.h>
#include <kern/stat.h>
#include <kern/lib.h>
#include <kern/sched.h>
#include <kern/clock.h>

static int bfs_mount(struct fs *fs);
static int bfs_umount(struct fs *fs);

static int bfs_lookup(struct fso **f, struct fs *fs, const char *name);
static int bfs_creat(struct fso **f, struct fs *fs, const char *name, mode_t mode, dev_t rdev);
static int bfs_getfso(struct fso *f);
static int bfs_putfso(struct fso *f);
static int bfs_syncfso(struct fso *f);

static int bfs_readdir(struct fso *dir, struct fs_dirent *de, int index);

static int bfs_ioctl(struct fso *fso, int cmd, void *buf);
static int bfs_read(struct fs_rwreq *req);
static int bfs_write(struct fs_rwreq *req);
static int bfs_trunc(struct fso *fso);

static int bfs_chk_perm(struct fso *f, int mode, uid_t uid, gid_t gid);
static int bfs_link(struct fso *f, const char *name);
static int bfs_unlink(struct fs *fs, const char *name);
static int bfs_rename(struct fs *fs, const char *oldname, const char *newname);

static int bfs_statfs(struct fs *fs, struct statfs *st);
static int bfs_sync(struct fs *fs);

static int bfs_chdir(struct fs *fs, const char *name);
static int bfs_mkdir(struct fs *fs, const char *name, mode_t mode);
static int bfs_rmdir(struct fs *fs, const char *name);

static struct fstype fstype =
{
	name:		"boot",
	mount:		bfs_mount,
	umount:		bfs_umount,
	lookup:		bfs_lookup,
	creat:		bfs_creat,
	getfso:		bfs_getfso,
	putfso:		bfs_putfso,
	syncfso:	bfs_syncfso,
	readdir:	bfs_readdir,
	ioctl:		bfs_ioctl,
	read:		bfs_read,
	write:		bfs_write,
	trunc:		bfs_trunc,
	chk_perm:	bfs_chk_perm,
	link:		bfs_link,
	unlink:		bfs_unlink,
	rename:		bfs_rename,
	chdir:		bfs_chdir,
	mkdir:		bfs_mkdir,
	rmdir:		bfs_rmdir,
	statfs:		bfs_statfs,
	sync:		bfs_sync,
};

void bfs_init()
{
	fs_install(&fstype);
}

int bfs_mount(struct fs *fs)
{
	fs->bfs.mount_time	= clock_time();
	fs->read_only		= 1;
	return 0;
}

int bfs_umount(struct fs *fs)
{
	return 0;
}

int bfs_lookup(struct fso **f, struct fs *fs, const char *name)
{
	struct bfs_head *h;
	struct block *b;
	blk_t i = 128;
	int err;
	
	do
	{
		/* if (resched)
			sched(); */
		
		err = blk_read(&b, fs->dev, i);
		if (err)
			return err;
		h = (void *)b->data;
		if (!strcmp(h->name, name))
		{
			blk_put(b);
			return fs_getfso(f, fs, i);
		}
		i = h->next;
		blk_put(b);
	} while (i);
	
	return ENOENT;
}

int bfs_creat(struct fso **f, struct fs *fs, const char *name, mode_t mode, dev_t rdev)
{
	return EROFS;
}

int bfs_getfso(struct fso *f)
{
	struct bfs_head *h;
	struct block *b;
	time_t t;
	int err;
	
	err = blk_read(&b, f->fs->dev, f->index);
	if (err)
		return err;
	h = (void *)b->data;
	
	t = f->fs->bfs.mount_time;
	
	fs_state(f, R_OK | W_OK);
	f->nlink  = 1;
	f->blocks = (h->size + BLK_SIZE - 1) / BLK_SIZE + 1;
	f->size	  = h->size;
	f->mode	  = h->mode;
	f->atime  = t;
	f->ctime  = t;
	f->mtime  = t;
	
	blk_put(b);
	return 0;
}

int bfs_putfso(struct fso *f)
{
	return 0;
}

int bfs_syncfso(struct fso *f)
{
	return 0;
}

int bfs_readdir(struct fso *dir, struct fs_dirent *de, int index)
{
	struct bfs_dirent bd;
	int err;
	
	if (index < 0)
		return EINVAL;
	
	if (index * sizeof(struct bfs_dirent) >= dir->size)
		return ENOENT;
	
	err = blk_pread(dir->fs->dev, dir->index + 1 + index / BFS_D_PER_BLOCK,
			(index % BFS_D_PER_BLOCK) * sizeof bd, sizeof bd, &bd);
	if (err)
		return err;
	
	strcpy(de->name, bd.name);
	de->index = -1;
	
	return 0;
}

int bfs_ioctl(struct fso *fso, int cmd, void *buf)
{
	return ENOTTY;
}

int bfs_read(struct fs_rwreq *req)
{
	unsigned long offset;
	unsigned long count;
	char *buf;
	int err;
	int l;
	
	if (req->offset + req->count < 0)
		return EINVAL;
	
	if (req->offset >= req->fso->size)
	{
		req->count = 0;
		return 0;
	}
	
	if (req->offset + req->count > req->fso->size)
		req->count = req->fso->size - req->offset;
	
	offset	= req->offset;
	count	= req->count;
	buf	= req->buf;
	
	while (count)
	{
		l = BLK_SIZE - (offset % BLK_SIZE);
		
		if (l > count)
			l = count;
		
		err = blk_pread(req->fso->fs->dev,
				req->fso->index + 1 + offset / BLK_SIZE,
				offset % BLK_SIZE, l, buf);
		if (err)
			return err;
		
		offset	+= l;
		buf	+= l;
		count	-= l;
	}
	
	return 0;
}

int bfs_write(struct fs_rwreq *req)
{
	return EROFS;
}

int bfs_trunc(struct fso *fso)
{
	return EROFS;
}

int bfs_chk_perm(struct fso *f, int mode, uid_t uid, gid_t gid)
{
	return 0;
}

int bfs_link(struct fso *f, const char *name)
{
	return EROFS;
}

int bfs_unlink(struct fs *fs, const char *name)
{
	return EROFS;
}

int bfs_rename(struct fs *fs, const char *oldname, const char *newname)
{
	return EROFS;
}

int bfs_statfs(struct fs *fs, struct statfs *st)
{
	struct bfs_head *h;
	struct block *b;
	blk_t i = 128;
	int err;
	
	for (;;)
	{
		/* if (resched)
			sched(); */
		
		err = blk_read(&b, fs->dev, i);
		if (err)
			return err;
		h = (void *)b->data;
		
		if (!h->next)
		{
			i += (h->size + 511) / 512;
			break;
		}
		
		i = h->next;
		blk_put(b);
	}
	
	st->blk_total = i;
	st->blk_free  = 0;
	return 0;
}

int bfs_sync(struct fs *fs)
{
	return 0;
}

int bfs_chdir(struct fs *fs, const char *name)
{
	struct fso *fso;
	int err;
	
	err = bfs_lookup(&fso, fs, name);
	if (err)
		return err;
	if (!S_ISDIR(fso->mode))
		err = ENOTDIR;
	fs_putfso(fso);
	return err;
}

int bfs_mkdir(struct fs *fs, const char *name, mode_t mode)
{
	return EROFS;
}

int bfs_rmdir(struct fs *fs, const char *name)
{
	return EROFS;
}
