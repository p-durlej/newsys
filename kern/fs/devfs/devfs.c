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

#include <kern/machine/machine.h>
#include <kern/console.h>
#include <kern/printk.h>
#include <kern/block.h>
#include <kern/errno.h>
#include <kern/clock.h>
#include <kern/task.h>
#include <kern/page.h>
#include <kern/umem.h>
#include <kern/cio.h>
#include <kern/lib.h>
#include <kern/fs.h>

#include <dev/console.h>

#include <sys/stat.h>

#define ROOT_INDEX	1
#define NULL_INDEX	2
#define ZERO_INDEX	3
#define TTY_INDEX	4
#define MEM_INDEX	5
#define CON_INDEX	6
#define PTY_INDEX	7
#define RDIR_INDEX	8
#define BDEV_INDEX	9
#define CDEV_INDEX	(BDEV_INDEX + BLK_MAXDEV)
#define RDEV_INDEX	(CDEV_INDEX + CIO_MAXDEV)
#define MAX_INDEX	(RDEV_INDEX + BLK_MAXDEV)

static struct devfs_stat
{
	mode_t	mode;
	uid_t	uid;
	gid_t	gid;
} devfs_stat[MAX_INDEX] =
{
	[ROOT_INDEX]	= { .mode = S_IFDIR | 0755 },
	[NULL_INDEX]	= { .mode = S_IFCHR | 0666 },
	[ZERO_INDEX]	= { .mode = S_IFCHR | 0666 },
	[MEM_INDEX]	= { .mode = S_IFCHR | 0600 },
	[CON_INDEX]	= { .mode = S_IFCHR | 0600 },
	[RDIR_INDEX]	= { .mode = S_IFDIR | 0755 },
};

static int devfs_mounted;

static int devfs_mount(struct fs *fs)
{
	if (devfs_mounted)
		return EBUSY;
	devfs_mounted = 1;
	return 0;
}

static int devfs_umount(struct fs *fs)
{
	devfs_mounted = 0;
	return 0;
}

static int devfs_lookup(struct fso **f, struct fs *fs, const char *name)
{
	int i;
	
	if (!*name)
		return fs_getfso(f, fs, ROOT_INDEX);
	
	if (!strcmp(name, "null"))
		return fs_getfso(f, fs, NULL_INDEX);
	
	if (!strcmp(name, "zero"))
		return fs_getfso(f, fs, ZERO_INDEX);
	
	if (!strcmp(name, "mem"))
		return fs_getfso(f, fs, MEM_INDEX);
	
	if (!strcmp(name, "console"))
		return fs_getfso(f, fs, CON_INDEX);
	
	if (!strcmp(name, "tty"))
		return ENOENT;
	
	if (!strcmp(name, "pty"))
		return ENOENT;
	
	if (!strcmp(name, "rdsk"))
		return fs_getfso(f, fs, RDIR_INDEX);
	
	if (!strncmp(name, "rdsk/", 5))
	{
		for (i = 0; i < BLK_MAXDEV; i++)
			if (blk_dev[i] && !strcmp(blk_dev[i]->name, name + 5))
				return fs_getfso(f, fs, i + RDEV_INDEX);
		return ENOENT;
	}
	
	for (i = 0; i < BLK_MAXDEV; i++)
		if (blk_dev[i] && !strcmp(blk_dev[i]->name, name))
			return fs_getfso(f, fs, i + BDEV_INDEX);
	
	for (i = 0; i < CIO_MAXDEV; i++)
		if (cio_dev[i] && !strcmp(cio_dev[i]->name, name))
			return fs_getfso(f, fs, i + CDEV_INDEX);
	
	return ENOENT;
}

static int devfs_creat(struct fso **f, struct fs *fs, const char *name, mode_t mode, dev_t rdev)
{
	return EPERM;
}

static int devfs_getfso(struct fso *f)
{
	struct cdev *cd;
	int err;
	int s;
	
	fs_state(f, R_OK | W_OK);
	
	f->mode	= devfs_stat[f->index].mode;
	f->uid	= devfs_stat[f->index].uid;
	f->gid	= devfs_stat[f->index].gid;
	
	if (f->index != ROOT_INDEX)
		f->rdev	= makedev(1, f->index);
	
	switch (f->index)
	{
	case ROOT_INDEX:
	case RDIR_INDEX:
		f->nlink = 1;
		f->atime = time;
		f->mtime = time;
		f->ctime = time;
		return 0;
	case NULL_INDEX:
	case ZERO_INDEX:
	case CON_INDEX:
		f->no_seek = 1;
		f->nlink = 1;
		f->atime = time;
		f->mtime = time;
		f->ctime = time;
		return 0;
	case MEM_INDEX:
		f->no_seek = 0;
		f->nlink = 1;
		f->atime = time;
		f->mtime = time;
		f->ctime = time;
		return 0;
	default:
		;
	}
	
	if (f->index >= BDEV_INDEX && f->index < BDEV_INDEX + BLK_MAXDEV)
	{
		err = blk_open(blk_dev[f->index - BDEV_INDEX]);
		if (err)
			return err;
		
		f->nlink = 1;
		
		f->atime = time;
		f->mtime = time;
		f->ctime = time;
		return 0;
	}
	
	if (f->index >= RDEV_INDEX && f->index < RDEV_INDEX + BLK_MAXDEV)
	{
		err = blk_open(blk_dev[f->index - RDEV_INDEX]);
		if (err)
			return err;
		
		f->nlink = 1;
		
		f->atime = time;
		f->mtime = time;
		f->ctime = time;
		return 0;
	}
	
	if (f->index >= CDEV_INDEX && f->index < CDEV_INDEX + CIO_MAXDEV)
	{
		cd = cio_dev[f->index - CDEV_INDEX];
		
		s = intr_dis();
		f->state = cd->state;
		cd->fso = f;
		intr_res(s);
		
		err = cio_open(cd);
		if (err)
			return err;
		
		f->no_seek = cd->no_seek;
		f->nlink   = 1;
		
		f->atime   = time;
		f->mtime   = time;
		f->ctime   = time;
		
		return 0;
	}
	
	panic("devfs_getfso: bad f->index");
}

static int devfs_putfso(struct fso *f)
{
	struct cdev *cd;
	int s;
	
	if (f->index >= BDEV_INDEX && f->index < BDEV_INDEX + BLK_MAXDEV)
	{
		blk_close(blk_dev[f->index - BDEV_INDEX]);
		goto save;
	}
	
	if (f->index >= RDEV_INDEX && f->index < RDEV_INDEX + BLK_MAXDEV)
	{
		blk_close(blk_dev[f->index - RDEV_INDEX]);
		goto save;
	}
	
	if (f->index >= CDEV_INDEX && f->index < CDEV_INDEX + CIO_MAXDEV)
	{
		cd = cio_dev[f->index - CDEV_INDEX];
		
		cio_close(cd);
		
		s = intr_dis();
		cd->fso = NULL;
		intr_res(s);
		
		goto save;
	}
	
save:
	devfs_stat[f->index].mode = f->mode;
	devfs_stat[f->index].uid  = f->uid;
	devfs_stat[f->index].gid  = f->gid;
	return 0;
}

static int devfs_syncfso(struct fso *f)
{
	return 0;
}

static int devfs_rdsk_readdir(struct fso *dir, struct fs_dirent *de, int index)
{
	switch (index)
	{
	case 0:
		strcpy(de->name, ".");
		de->index = RDIR_INDEX;
		return 0;
	case 1:
		strcpy(de->name, "..");
		de->index = ROOT_INDEX;
		return 0;
	default:
		;
	}
	
	index -= 2;
	if (index < BLK_MAXDEV)
	{
		if (!blk_dev[index])
		{
			de->index = 0;
			return 0;
		}
		strcpy(de->name, blk_dev[index]->name);
		de->index = RDEV_INDEX + index;
		return 0;
	}
	
	return ENOENT;
}

static int devfs_readdir(struct fso *dir, struct fs_dirent *de, int index)
{
	if (index < 0)
		return EINVAL;
	
	if (dir->index == RDIR_INDEX)
		return devfs_rdsk_readdir(dir, de, index);
	
	switch (index)
	{
	case 0:
		strcpy(de->name, ".");
		de->index = ROOT_INDEX;
		return 0;
	case 1:
		strcpy(de->name, "..");
		de->index = ROOT_INDEX;
		return 0;
	case NULL_INDEX:
		strcpy(de->name, "null");
		de->index = index;
		return 0;
	case ZERO_INDEX:
		strcpy(de->name, "zero");
		de->index = index;
		return 0;
	case MEM_INDEX:
		strcpy(de->name, "mem");
		de->index = index;
		return 0;
	case CON_INDEX:
		strcpy(de->name, "console");
		de->index = index;
		return 0;
	case TTY_INDEX:
		if (*curr->tty)
		{
			strcpy(de->name, "tty");
			de->index = index;
		}
		else
			de->index = 0;
		return 0;
	case PTY_INDEX:
		strcpy(de->name, "pty");
		return 0;
	case RDIR_INDEX:
		strcpy(de->name, "rdsk");
		return 0;
	}
	
	if (index >= BDEV_INDEX && index < BDEV_INDEX + BLK_MAXDEV)
	{
		if (!blk_dev[index - BDEV_INDEX])
		{
			de->index = 0;
			return 0;
		}
		strcpy(de->name, blk_dev[index - BDEV_INDEX]->name);
		de->index = index;
		return 0;
	}
	
	if (index >= CDEV_INDEX && index < CDEV_INDEX + CIO_MAXDEV)
	{
		if (!cio_dev[index - CDEV_INDEX])
		{
			de->index = 0;
			return 0;
		}
		strcpy(de->name, cio_dev[index - CDEV_INDEX]->name);
		de->index = index;
		return 0;
	}
	
	return ENOENT;
}

static int devfs_con_ioctl(int cmd, void *buf)
{
	struct cioinfo *cgi;
	struct ciogoto *cgt;
	struct cioputa *cpa;
	const char *cpad;
	int cpax;
	int err;
	int i;
	
	if (!con_con)
		return ENODEV;
	
	switch (cmd)
	{
	case CIOGINFO:
		err = uga(&buf, sizeof *cgi, UA_WRITE);
		if (err)
			return err;
		cgi = buf;
		
		memset(cgi, 0, sizeof cgi);
		cgi->w	   = con_con->width;
		cgi->h	   = con_con->height;
		cgi->flags = con_con->flags;
		
		return 0;
	case CIOGOTO:
		err = uga(&buf, sizeof *cgt, UA_WRITE);
		if (err)
			return err;
		cgt = buf;
		
		con_con->gotoxy(con_con, cgt->x, cgt->y);
		return 0;
	case CIOPUTA:
		err = uga(&buf, sizeof *cpa, UA_WRITE);
		if (err)
			return err;
		cpa = buf;
		
		err = uaa(&cpa->data, sizeof *cpa->data, cpa->len, UA_READ);
		if (err)
			return err;
		
		cpad = cpa->data;
		cpax = cpa->x;
		
		for (i = 0; i < cpa->len; i++)
		{
			con_con->puta(con_con, cpax, cpa->y, cpad[0], cpad[1]);
			cpad += 2;
			cpax++;
		}
		
		return 0;
	case CIODISABLE:
		if (curr->euid)
			return EPERM;
		con_disable();
		return 0;
	case CIOMUTE:
		if (curr->euid)
			return EPERM;
		con_mute();
		return 0;
	default:
		return ENOTTY;
	}
}

static int devfs_ioctl(struct fso *fso, int cmd, void *buf)
{
	int err;
	int i;
	
	if (fso->index == CON_INDEX)
		return devfs_con_ioctl(cmd, buf);
	
	if (fso->index >= BDEV_INDEX && fso->index < BDEV_INDEX + BLK_MAXDEV)
	{
		i = fso->index - BDEV_INDEX;
		
		err = fs_chk_perm(fso, R_OK | W_OK, curr->euid, curr->egid);
		if (err)
			return err;
		
		return blk_dev[i]->ioctl(blk_dev[i]->unit, cmd, buf);
	}
	
	if (fso->index >= RDEV_INDEX && fso->index < RDEV_INDEX + BLK_MAXDEV)
	{
		i = fso->index - RDEV_INDEX;
		
		err = fs_chk_perm(fso, R_OK | W_OK, curr->euid, curr->egid);
		if (err)
			return err;
		
		return blk_dev[i]->ioctl(blk_dev[i]->unit, cmd, buf);
	}
	
	if (fso->index >= CDEV_INDEX && fso->index < CDEV_INDEX + CIO_MAXDEV)
	{
		i = fso->index - CDEV_INDEX;
		
		err = fs_chk_perm(fso, R_OK | W_OK, curr->euid, curr->egid);
		if (err)
			return err;
		
		return cio_dev[i]->ioctl(cio_dev[i], cmd, buf);
	}
	return ENOTTY;
}

static void devfs_mem_stmp(uoff_t off)
{
	vpage pte = (off & PAGE_VMASK) | PGF_KERN;
	
	if (off < 0)
		pte = 0;
	
	pg_tab[PAGE_TMP] = pte;
	pg_utlb();
}

static int devfs_mem_read(struct fs_rwreq *req)
{
#define VIRT	((char *)(PAGE_TMP << 12))
	size_t left = req->count;
	uoff_t off  = req->offset;
	char *buf   = req->buf;
	
	while (left)
	{
		size_t clen;
		uoff_t poff;
		
		poff = off % PAGE_SIZE;
		clen = left;
		if (clen + poff > PAGE_SIZE)
			clen = PAGE_SIZE - poff;
		
		devfs_mem_stmp(off);
		memcpy(buf, VIRT + poff, clen);
		
		buf  += clen;
		left -= clen;
		off  += clen;
	}
	devfs_mem_stmp(0);
	
	return 0;
#undef VIRT
}

static int devfs_mem_write(struct fs_rwreq *req)
{
#define VIRT	((char *)(PAGE_TMP << 12))
	size_t left = req->count;
	uoff_t off  = req->offset;
	char *buf   = req->buf;
	
	while (left)
	{
		size_t clen;
		uoff_t poff;
		
		poff = off % PAGE_SIZE;
		clen = left;
		if (clen + poff > PAGE_SIZE)
			clen = PAGE_SIZE - poff;
		
		devfs_mem_stmp(off);
		memcpy(VIRT + poff, buf, clen);
		
		buf  += clen;
		left -= clen;
		off  += clen;
	}
	devfs_mem_stmp(0);
	
	return 0;
#undef VIRT
}

static int devfs_read(struct fs_rwreq *req)
{
	unsigned long offset;
	unsigned long count;
	struct bdev *bd;
	char *buf;
	ino_t ino;
	int err;
	int l;
	
	ino = req->fso->index;
	
	switch (ino)
	{
	case NULL_INDEX:
		req->count = 0;
		return 0;
	case ZERO_INDEX:
		memset(req->buf, 0, req->count);
		return 0;
	case MEM_INDEX:
		return devfs_mem_read(req);
	case CON_INDEX:
		return ENOSYS;
	default:
		;
	}
	
	if (req->offset + req->count < 0)
		return EINVAL;
	
	if (ino == ROOT_INDEX)
		return EISDIR;
	
	if (ino >= BDEV_INDEX && ino < BDEV_INDEX + BLK_MAXDEV)
	{
		offset = req->offset;
		count = req->count;
		buf = req->buf;
		
		bd = blk_dev[ino - BDEV_INDEX];
		
		while (count)
		{
			l = BLK_SIZE - (offset % BLK_SIZE);
			
			if (l > count)
				l = count;
			
			err = blk_pread(bd, offset / BLK_SIZE, offset % BLK_SIZE, l, buf);
			if (err)
				return err;
			
			offset += l;
			count -= l;
			buf += l;
		}
		return 0;
	}
	
	if (ino >= CDEV_INDEX && ino < CDEV_INDEX + CIO_MAXDEV)
	{
		struct cio_request crq;
		struct cdev *cd;
		
		cd = cio_dev[ino - CDEV_INDEX];
		
		crq.offset   = req->offset;
		crq.count    = req->count;
		crq.buf	     = req->buf;
		crq.cdev     = cd;
		crq.no_delay = req->no_delay;
		err = cd->read(&crq);
		if (err)
			return err;
		req->count = crq.count;
		return 0;
	}
	
	if (ino >= RDEV_INDEX && ino < RDEV_INDEX + BLK_MAXDEV)
	{
		char dbuf[BLK_SIZE];
		
		offset = req->offset;
		count = req->count;
		buf = req->buf;
		
		bd = blk_dev[ino - RDEV_INDEX];
		
		while (count)
		{
			l = BLK_SIZE - (offset % BLK_SIZE);
			
			if (l > count)
				l = count;
			
			if (l == BLK_SIZE)
			{
				err = bd->read(bd->unit, offset / BLK_SIZE, buf);
				if (err)
				{
					bd->error_cnt++;
					return err;
				}
				bd->read_cnt++;
			}
			else
			{
				err = bd->read(bd->unit, offset / BLK_SIZE, dbuf);
				if (err)
				{
					bd->error_cnt++;
					return err;
				}
				bd->read_cnt++;
				
				memcpy(buf, dbuf + offset % BLK_SIZE, l);
			}
			
			offset += l;
			count -= l;
			buf += l;
		}
		return 0;
	}
	
	panic("devfs_read: bad req->fso->index");
}

static int devfs_cwrite(struct fs_rwreq *req)
{
	char *buf = req->buf;
	int cnt = req->count;
	int err;
	int i;
	
	err = uga(&buf, cnt, UA_READ);
	if (err)
		return err;
	
	for (i = 0; i < cnt; i++)
		putchar(buf[i]);
	return 0;
}

static int devfs_write(struct fs_rwreq *req)
{
	unsigned long offset;
	unsigned long count;
	struct bdev *bd;
	char *buf;
	ino_t ino;
	int err;
	int l;
	
	ino = req->fso->index;
	
	switch (ino)
	{
	case NULL_INDEX:
	case ZERO_INDEX:
		return 0;
	case MEM_INDEX:
		return devfs_mem_write(req);
	case CON_INDEX:
		return devfs_cwrite(req);
	}
	
	if (req->offset + req->count < 0)
		return EINVAL;
	
	if (ino == ROOT_INDEX)
		return EISDIR;
	
	if (ino >= BDEV_INDEX && ino < BDEV_INDEX + BLK_MAXDEV)
	{
		offset = req->offset;
		count = req->count;
		buf = req->buf;
		
		bd = blk_dev[ino - BDEV_INDEX];
		
		while (count)
		{
			l = BLK_SIZE - (offset % BLK_SIZE);
			
			if (l > count)
				l = count;
			
			err = blk_pwrite(bd, offset / BLK_SIZE, offset % BLK_SIZE, l, buf);
			if (err)
				return err;
			
			offset += l;
			count -= l;
			buf += l;
		}
		return 0;
	}
	
	if (ino >= CDEV_INDEX && ino < CDEV_INDEX + CIO_MAXDEV)
	{
		struct cio_request crq;
		struct cdev *cd;
		
		cd = cio_dev[ino - CDEV_INDEX];
		
		crq.offset   = req->offset;
		crq.count    = req->count;
		crq.buf	     = req->buf;
		crq.cdev     = cd;
		crq.no_delay = req->no_delay;
		err = cd->write(&crq);
		if (err)
			return err;
		req->count = crq.count;
		return 0;
	}
	
	if (ino >= RDEV_INDEX && ino < RDEV_INDEX + BLK_MAXDEV)
	{
		char dbuf[BLK_SIZE];
		
		offset = req->offset;
		count = req->count;
		buf = req->buf;
		
		bd = blk_dev[ino - RDEV_INDEX];
		
		while (count)
		{
			l = BLK_SIZE - (offset % BLK_SIZE);
			
			if (l > count)
				l = count;
			
			if (l == BLK_SIZE)
			{
				err = bd->write(bd->unit, offset / BLK_SIZE, buf);
				if (err)
				{
					bd->error_cnt++;
					return err;
				}
				bd->write_cnt++;
			}
			else
			{
				err = bd->read(bd->unit, offset / BLK_SIZE, dbuf);
				if (err)
				{
					bd->error_cnt++;
					return err;
				}
				bd->read_cnt++;
				
				memcpy(dbuf + offset % BLK_SIZE, buf, l);
				
				err = bd->write(bd->unit, offset / BLK_SIZE, dbuf);
				if (err)
				{
					bd->error_cnt++;
					return err;
				}
				bd->write_cnt++;
			}
			
			offset += l;
			count -= l;
			buf += l;
		}
		return 0;
	}
	
	panic("devfs_write: bad req->fso->index");
}

static int devfs_trunc(struct fso *fso)
{
	return 0;
}

static int devfs_chk_perm(struct fso *f, int mode, uid_t uid, gid_t gid)
{
	return 0;
}

static int devfs_link(struct fso *f, const char *name)
{
	return EPERM;
}

static int devfs_unlink(struct fs *fs, const char *name)
{
	return EPERM;
}

static int devfs_rename(struct fs *fs, const char *oldname, const char *newname)
{
	return EPERM;
}

static int devfs_sync(struct fs *fs)
{
	return 0;
}

static int devfs_chdir(struct fs *fs, const char *name)
{
	if (!*name || !strcmp(name, "rdsk"))
		return 0;
	
	return ENOENT;
}

static int devfs_mkdir(struct fs *fs, const char *name, mode_t mode)
{
	return EPERM;
}

static int devfs_rmdir(struct fs *fs, const char *name)
{
	return EPERM;
}

static int devfs_statfs(struct fs *fs, struct statfs *st)
{
	st->blk_total = 0;
	st->blk_free  = 0;
	return 0;
}

static struct fstype fstype =
{
	name:		"dev",
	mount:		devfs_mount,
	umount:		devfs_umount,
	lookup:		devfs_lookup,
	creat:		devfs_creat,
	getfso:		devfs_getfso,
	putfso:		devfs_putfso,
	syncfso:	devfs_syncfso,
	readdir:	devfs_readdir,
	ioctl:		devfs_ioctl,
	read:		devfs_read,
	write:		devfs_write,
	trunc:		devfs_trunc,
	chk_perm:	devfs_chk_perm,
	link:		devfs_link,
	unlink:		devfs_unlink,
	rename:		devfs_rename,
	chdir:		devfs_chdir,
	mkdir:		devfs_mkdir,
	rmdir:		devfs_rmdir,
	statfs:		devfs_statfs,
	sync:		devfs_sync,
};

void devfs_init(void)
{
	int i;
	
	for (i = BDEV_INDEX; i < BDEV_INDEX + BLK_MAXDEV; i++)
		devfs_stat[i].mode = S_IFBLK | 0600;
	
	for (i = RDEV_INDEX; i < RDEV_INDEX + BLK_MAXDEV; i++)
		devfs_stat[i].mode = S_IFCHR | 0600;
	
	for (i = CDEV_INDEX; i < CDEV_INDEX + CIO_MAXDEV; i++)
		devfs_stat[i].mode = S_IFCHR | 0600;
	
	fs_install(&fstype);
}
