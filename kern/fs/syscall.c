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

#include <kern/syscall.h>
#include <kern/printk.h>
#include <kern/errno.h>
#include <kern/clock.h>
#include <kern/sched.h>
#include <kern/umem.h>
#include <kern/task.h>
#include <kern/lib.h>
#include <kern/fs.h>

#include <sys/signal.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <mount.h>
#include <os386.h>

struct task_queue fs_pollq =
{
	.name	= "poll",
};

char *sys_getcwd(char *path, int len)
{
	int err;
	int l;
	
	l = strlen(curr->cwd);
	if (l >= len)
	{
		uerr(ENAMETOOLONG);
		return NULL;
	}
	
	err = tucpy(path, curr->cwd, l + 1);
	if (err)
	{
		uerr(err);
		return NULL;
	}
	
	return path;
}

int sys__chdir(const char *path, int flags)
{
	int err;
	
	err = usa(&path, PATH_MAX - 1, ENAMETOOLONG);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = fs_chdir(path);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	curr->ocwd = !!flags;
	
	return 0;
}

int sys__mkdir(const char *pathname, mode_t mode)
{
	int err;
	
	err = usa(&pathname, PATH_MAX - 1, ENAMETOOLONG);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = fs_mkdir(pathname, mode);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_rmdir(const char *pathname)
{
	int err;
	
	err = usa(&pathname, PATH_MAX - 1, ENAMETOOLONG);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = fs_rmdir(pathname);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_access(const char *pathname, int mode)
{
	int err;
	
	err = usa(&pathname, PATH_MAX - 1, ENAMETOOLONG);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = fs_access(pathname, mode);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys__open(const char *pathname, int mode)
{
	struct fs_file *file;
	struct fso *fso;
	unsigned fd;
	int err;
	
	err = usa(&pathname, PATH_MAX - 1, ENAMETOOLONG);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = fs_lookup(&fso, pathname);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	switch (mode & 7)
	{
	case O_RDONLY:
		err = fs_chk_perm(fso, R_OK, curr->euid, curr->egid);
		break;
	case O_WRONLY:
		err = fs_chk_perm(fso, W_OK, curr->euid, curr->egid);
		break;
	case O_RDWR:
		err = fs_chk_perm(fso, R_OK | W_OK, curr->euid, curr->egid);
		break;
	case O_NOIO:
		break;
	default:
		err = EINVAL;
	}
	
	if (err)
	{
		fs_putfso(fso);
		uerr(err);
		return -1;
	}
	
	if (mode & O_TRUNC)
	{
		err = fs_chk_perm(fso, W_OK, curr->euid, curr->egid);
		if (err)
		{
			fs_putfso(fso);
			uerr(err);
			return -1;
		}
		
		err = fso->fs->type->trunc(fso);
		if (err)
		{
			fs_putfso(fso);
			uerr(err);
			return -1;
		}
	}
	
	err = fs_getfile(&file, fso, mode);
	if (err)
	{
		fs_putfso(fso);
		uerr(err);
		return -1;
	}
	
	err = fs_getfd(&fd, file);
	if (err)
	{
		fs_putfile(file);
		uerr(err);
		return -1;
	}
	
	return fd;
}

int sys_close(int fd)
{
	int err;
	
	err = fs_putfd(fd);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys__read(int fd, void *buf, size_t size)
{
	struct fs_rwreq rq;
	struct fs_desc *d;
	int err;
	
	err = fs_fdaccess(fd, R_OK);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = uga(&buf, size, UA_WRITE);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	d = &curr->file_desc[fd];
	
	rq.buf	    = buf;
	rq.offset   = d->file->offset;
	rq.count    = size;
	rq.fso	    = d->file->fso;
	rq.no_delay = !!(d->file->omode & O_NDELAY);
	
	err = rq.fso->fs->type->read(&rq);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	if (!d->file->fso->no_seek)
		d->file->offset += rq.count;
	return rq.count;
}

int sys__write(int fd, void *buf, size_t size)
{
	struct fs_rwreq rq;
	struct fs_desc *d;
	int err;
	
	err = fs_fdaccess(fd, W_OK);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = uga(&buf, size, UA_READ);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	d = &curr->file_desc[fd];
	
	rq.buf	    = buf;
	rq.offset   = d->file->offset;
	rq.count    = size;
	rq.fso	    = d->file->fso;
	rq.no_delay = !!(d->file->omode & O_NDELAY);
	
	if (d->file->omode & O_APPEND)
		rq.offset = rq.fso->size;
	
	err = rq.fso->fs->type->write(&rq);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	if (!d->file->fso->no_seek)
		d->file->offset += rq.count;
	return rq.count;
}

off_t sys_lseek(int fd, off_t off, int mode)
{
	struct fs_desc *d;
	int err;
	
	err = fs_fdaccess(fd, 0);
	if (err)
	{
		uerr(err);
		return -1;
	}
	d = &curr->file_desc[fd];
	if (d->file->fso->no_seek)
	{
		uerr(ESPIPE);
		return -1;
	}
	
	switch (mode)
	{
	case SEEK_SET:
		d->file->offset = off;
		break;
	case SEEK_CUR:
		d->file->offset += off;
		break;
	case SEEK_END:
		d->file->offset = d->file->fso->size + off;
		break;
	default:
		uerr(EINVAL);
		return -1;
	}
	
	return d->file->offset;
}

int sys_fcntl(int fd, int cmd, long arg)
{
	struct fs_desc *d;
	int err;
	int i;
	
	err = fs_fdaccess(fd, 0);
	if (err)
	{
		uerr(err);
		return -1;
	}
	d = &curr->file_desc[fd];
	
	switch (cmd)
	{
	case F_GETFD:
		return d->close_on_exec;
	case F_SETFD:
		d->close_on_exec = arg;
		return 0;
	case F_DUPFD:
		for (i = arg; i < OPEN_MAX; i++)
			if (!curr->file_desc[i].file)
			{
				curr->file_desc[i].file = d->file;
				d->file->refcnt++;
				return i;
			}
		
		err = EMFILE;
		break;
	case F_GETFL:
		return d->file->omode;
	case F_SETFL:
		d->file->omode &=      ~(O_APPEND | O_NONBLOCK);
		d->file->omode |= arg & (O_APPEND | O_NONBLOCK);
		return 0;
	default:
		err = EINVAL;
	}
	
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys__mknod(const char *pathname, mode_t mode, dev_t rdev)
{
	struct fso *f;
	int err;
	
	err = usa(&pathname, PATH_MAX - 1, ENAMETOOLONG);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = fs_creat(&f, pathname, mode, rdev);
	if (err)
	{
		uerr(err);
		return -1;
	}
	fs_putfso(f);
	
	return 0;
}

int sys_unlink(const char *pathname)
{
	int err;
	
	err = usa(&pathname, PATH_MAX - 1, ENAMETOOLONG);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = fs_unlink(pathname);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_link(const char *oldname, const char *newname)
{
	int err;
	
	err = usa(&oldname, PATH_MAX - 1, ENAMETOOLONG);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = usa(&newname, PATH_MAX - 1, ENAMETOOLONG);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = fs_link(oldname, newname);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_rename(const char *oldname, const char *newname)
{
	int err;
	
	err = usa(&oldname, PATH_MAX - 1, ENAMETOOLONG);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = usa(&newname, PATH_MAX - 1, ENAMETOOLONG);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = fs_rename(oldname, newname);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys_fstat(int fd, struct stat *st)
{
	struct fso *fso;
	int err;
	int i;
	
	err = fs_fdaccess(fd, 0);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = uga(&st, sizeof *st, UA_WRITE);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	fso = curr->file_desc[fd].file->fso;
	
	st->st_ino	= fso->index;
	st->st_dev	= 0;
	st->st_rdev	= fso->rdev;
	st->st_mode	= fso->mode;
	st->st_nlink	= fso->nlink;
	st->st_uid	= fso->uid;
	st->st_gid	= fso->gid;
	st->st_size	= fso->size;
	st->st_blksize	= BLK_SIZE;
	st->st_blocks	= fso->blocks;
	st->st_atime	= fso->atime;
	st->st_ctime	= fso->ctime;
	st->st_mtime	= fso->mtime;
	
	if (fso->fs->dev)
		for (i = 0; i < BLK_MAXDEV; i++)
			if (blk_dev[i] == fso->fs->dev)
			{
				st->st_dev = makedev(1, i + 9); /* XXX BDEV_INDEX */
				break;
			}
	
	return 0;
}

int sys_fchmod(int fd, mode_t mode)
{
	struct fso *fso;
	int err;
	
	err = fs_fdaccess(fd, 0);
	if (err)
	{
		uerr(err);
		return -1;
	}
	fso = curr->file_desc[fd].file->fso;
	
	if (!fso->fs->insecure && curr->euid && fso->uid != curr->euid)
	{
		uerr(EACCES);
		return -1;
	}
	
	if (fso->fs->read_only)
	{
		uerr(EROFS);
		return -1;
	}
	
	fso->mode &= ~S_IRIGHTS;
	fso->mode |= (mode & S_IRIGHTS);
	fso->ctime = clock_time();
	fso->dirty = 1;
	
	return 0;
}

int sys_fchown(int fd, uid_t uid, gid_t gid)
{
	struct fso *fso;
	int err;
	
	err = fs_fdaccess(fd, 0);
	if (err)
	{
		uerr(err);
		return -1;
	}
	fso = curr->file_desc[fd].file->fso;
	
	if (curr->euid)
	{
		uerr(EACCES);
		return -1;
	}
	
	if (fso->fs->read_only)
	{
		uerr(EROFS);
		return -1;
	}
	
	fso->mode &= ~(mode_t)(S_ISUID | S_ISGID);
	fso->uid   = uid;
	fso->gid   = gid;
	fso->ctime = clock_time();
	fso->dirty = 1;
	
	return 0;
}

int sys_ttyname_r(int fd, char *buf, int maxlen)
{
	char lbuf[PATH_MAX];
	struct fso *fso;
	int err;
	int l;
	
	err = fs_fdaccess(fd, 0);
	if (err)
	{
		uerr(err);
		return -1;
	}
	fso = curr->file_desc[fd].file->fso;
	if (!fso->fs->type->ttyname)
	{
		uerr(ENOTTY);
		return -1;
	}
	err = fso->fs->type->ttyname(fso, lbuf, sizeof lbuf);
	if (err)
	{
		uerr(err);
		return -1;
	}
	l = strlen(lbuf) + 1;
	if (l > maxlen)
	{
		uerr(ENOMEM);
		return -1;
	}
	err = tucpy(buf, lbuf, l);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys__mount(const char *prefix, const char *dev, const char *type, int ro)
{
	int err;
	
	if ((err = usa(&prefix,	PATH_MAX - 1, ENAMETOOLONG)) ||
	    (err = usa(&dev,	NAME_MAX,     ENAMETOOLONG)) ||
	    (err = usa(&type,	NAME_MAX,     ENAMETOOLONG)))
	{
		uerr(err);
		return -1;
	}
	
	err = fs_mount(prefix, dev, type, ro);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys__umount(const char *prefix)
{
	int err;
	
	err = usa(&prefix, PATH_MAX - 1, ENAMETOOLONG);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = fs_umount(prefix);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys__readdir(int fd, struct dirent *de, int index)
{
	struct fs_dirent fde;
	struct fso *fso;
	int err;
	
	err = fs_fdaccess(fd, R_OK);
	if (err)
		goto err;
	
	err = uga(&de, sizeof *de, UA_WRITE | UA_INIT);
	if (err)
		goto err;
	
	fso = curr->file_desc[fd].file->fso;
	
	if (!S_ISDIR(fso->mode))
	{
		err = ENOTDIR;
		goto err;
	}
	
	err = fso->fs->type->readdir(fso, &fde, index);
	if (err)
		goto err;
	
	de->d_ino = fde.index;
	if (fde.index)
		strcpy(de->d_name, fde.name);
	
	return 0;
err:
	uerr(err);
	return -1;
}

int sys_ioctl(int fd, int cmd, void *buf)
{
	struct fxstat xst;
	struct fso *fso;
	int err;
	
	err = fs_fdaccess(fd, 0);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	fso = curr->file_desc[fd].file->fso;
	switch (cmd)
	{
	case FIOCXSTAT:
		if (curr->euid)
			return EPERM;
		
		memset(&xst, 0, sizeof xst);
		xst.refcnt = fso->refcnt;
		intr_dis();
		xst.state  = fso->state;
		intr_ena();
		
		err = tucpy(buf, &xst, sizeof xst);
		break;
	default:
		err = fso->fs->type->ioctl(fso, cmd, buf);
	}
	
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys__statfs(const char *pathname, struct statfs *st)
{
	struct statfs lst;
	struct fso *fso;
	int err;
	
	memset(&lst, 0, sizeof lst);
	
	err = usa(&pathname, PATH_MAX - 1, ENAMETOOLONG);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = fs_lookup(&fso, pathname);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = fso->fs->type->statfs(fso->fs, &lst);
	fs_putfso(fso);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = tucpy(st, &lst, sizeof lst);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys__mtab(struct _mtab *buf, int len)
{
	struct _mtab m;
	int err;
	int i;
	
	if (len < sizeof m * FS_MAXFS)
	{
		uerr(EINVAL);
		return -1;
	}
	
	for (i = 0; i < FS_MAXFS; i++)
	{
		memset(&m, 0, sizeof m);
		if (fs_fs[i].in_use)
		{
			strcpy(m.prefix, fs_fs[i].prefix);
			if (fs_fs[i].dev)
				strcpy(m.device, fs_fs[i].dev->name);
			else
				strcpy(m.device, "nodev");
			strcpy(m.fstype, fs_fs[i].type->name);
			m.flags = 0;
			if (fs_fs[i].read_only)
				m.flags |= MF_READ_ONLY;
			if (fs_fs[i].no_atime)
				m.flags |= MF_NO_ATIME;
			if (fs_fs[i].removable)
				m.flags |= MF_REMOVABLE;
			if (fs_fs[i].insecure)
				m.flags |= MF_INSECURE;
			m.mounted = 1;
		}
		err = tucpy(&buf[i], &m, sizeof m);
		if (err)
		{
			uerr(err);
			return -1;
		}
	}
	
	return 0;
}

static void unpoll1(struct fso *f)
{
	int s;
	
	if (!f->pollcnt)
		panic("unpoll1: !f->pollcnt");
	s = intr_dis();
	if (f->polling == curr)
		f->polling = NULL;
	f->pollcnt--;
	intr_res(s);
}

static void poll1(struct fso *f)
{
	int s;
	
	if (f->pollcnt)
	{
		s = intr_dis();
		curr->multipoll = 1;
		f->polling->multipoll = 1;
		f->polling = NULL;
		f->pollcnt++;
		intr_res(s);
		return;
	}
	s = intr_dis();
	f->polling = curr;
	f->pollcnt = 1;
	intr_res(s);
}

int sys__poll(struct pollfd *pfd, int cnt, int ndelay)
{
	struct fso *f;
	int rcnt;
	int err;
	int i;
	int s;
	
	err = uaa(&pfd, sizeof *pfd, cnt, UA_READ | UA_WRITE);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	for (i = 0; i < cnt; i++)
		err = fs_fdaccess(pfd[i].fd, pfd[i].events);
		if (err)
		{
			uerr(err);
			return -1;
		}
	
	if (!ndelay)
		for (i = 0; i < cnt; i++)
		{
			err = fs_fdaccess(pfd[i].fd, pfd[i].events);
			if (err)
				panic("sys_poll: fs_fdaccess failed");
			
			poll1(curr->file_desc[pfd[i].fd].file->fso);
		}
	
	rcnt = 0;
	/* err  = 0; */
	for (;;)
	{
		s = intr_dis();
		for (i = 0; i < cnt; i++)
			if (curr->file_desc[pfd[i].fd].file->fso->state & pfd[i].events)
				rcnt++;
		
		if (rcnt || ndelay)
		{
			intr_res(s);
			break;
		}
		
		err = task_suspend(&fs_pollq, WAIT_INTR);
		intr_res(s);
		if (err)
			break;
	}
	
	for (i = 0; i < cnt; i++)
	{
		f = curr->file_desc[pfd[i].fd].file->fso;
		if (!ndelay)
			unpoll1(f);
		
		s = intr_dis();
		pfd[i].revents = f->state & pfd[i].events;
		intr_res(s);
	}
	curr->multipoll = 0;
	
	if (rcnt)
		return rcnt;
	if (err)
		return err;
	return 0;
}

int sys__ctty(const char *pathname)
{
	int err;
	
	err = usa(&pathname, PATH_MAX - 1, ENAMETOOLONG);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	strcpy(curr->tty, pathname); /* XXX should call fqname */
	return 0;
}

int sys_revoke(const char *pathname)
{
	int err;
	
	err = usa(&pathname, PATH_MAX - 1, ENAMETOOLONG);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = fs_revoke(pathname);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys__rfsactive(void)
{
	int cnt = 0;
	int i;
	
	for (i = 0; i < FS_MAXFS; i++)
	{
		struct fs *fs = &fs_fs[i];
		
		if (!fs->in_use)
			continue;
		
		if (fs->removable && fs->active)
			cnt++;
	}
	
	return cnt;
}
