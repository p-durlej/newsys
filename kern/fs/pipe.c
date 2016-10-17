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

#include <kern/syscall.h>
#include <kern/printk.h>
#include <kern/signal.h>
#include <kern/sched.h>
#include <kern/clock.h>
#include <kern/task.h>
#include <kern/stat.h>
#include <kern/umem.h>
#include <kern/lib.h>
#include <kern/fs.h>
#include <fcntl.h>
#include <errno.h>

static int pipe_mount(struct fs *fs)
{
	return ENOSYS;
}

static int pipe_umount(struct fs *fs)
{
	return ENOSYS;
}

static int pipe_lookup(struct fso **f, struct fs *fs, const char *name)
{
	return ENOSYS;
}

static int pipe_creat(struct fso **f, struct fs *fs, const char *name, mode_t mode, dev_t rdev)
{
	return ENOSYS;
}

static void pipe_ustate(struct fso *f)
{
	struct task *t;
	int st = 0;
	
	if (f->size < PIPE_BUF)
		st |= W_OK;
	if (f->size)
		st |= R_OK;
	
	fs_state(f, st);
	
	if (st & R_OK)
		while (t = task_dequeue(&f->pipe.readers), t)
			task_resume(t);
	
	if (st & W_OK)
		while (t = task_dequeue(&f->pipe.writers), t)
			task_resume(t);
}

static int pipe_getfso(struct fso *f)
{
	int err;
	
	err = kmalloc(&f->pipe.buf, PIPE_BUF, "pipe");
	if (err)
	{
		printk("pipe_getfso: malloc failed\n");
		return err;
	}
	task_qinit(&f->pipe.readers, "piperd");
	task_qinit(&f->pipe.writers, "pipewr");
	f->pipe.write_p	= 0;
	f->pipe.read_p	= 0;
	f->size		= 0;
	f->mode		= S_IFIFO | 0600;
	f->uid		= curr->euid;
	f->gid		= curr->egid;
	f->atime	= f->ctime = f->mtime = clock_time();
	f->no_seek	= 1;
	fs_setstate(f, W_OK);
	return 0;
}

static int pipe_putfso(struct fso *f)
{
	free(f->pipe.buf);
	return 0;
}

static int pipe_syncfso(struct fso *f)
{
	return ENOSYS;
}

static int pipe_readdir(struct fso *dir, struct fs_dirent *de, int index)
{
	return ENOSYS;
}

static int pipe_read(struct fs_rwreq *req)
{
	struct fso *f = req->fso;
	size_t cnt;
	size_t l;
	int err;
	
	while (!f->size)
	{
		if (!f->writer_count || req->no_delay)
		{
			req->count = 0;
			return 0;
		}
		if (curr->signal_pending)
			return EINTR;
		
		err = task_suspend(&f->pipe.readers, WAIT_INTR);
		if (err)
			return err;
	}
	
	cnt = req->count;
	if (cnt > f->size)
		cnt = req->count = f->size;
	
	l = cnt;
	if (l + f->pipe.read_p > PIPE_BUF)
		l = PIPE_BUF - f->pipe.read_p;
	
	memcpy(req->buf, f->pipe.buf + f->pipe.read_p, l);
	memcpy(req->buf + l, f->pipe.buf, cnt - l);
	
	f->pipe.read_p	+= cnt;
	f->pipe.read_p	%= PIPE_BUF;
	f->size		-= cnt;
	pipe_ustate(f);
	return 0;
}

static int pipe_write(struct fs_rwreq *req)
{
	struct fso *f = req->fso;
	char *buf = req->buf;
	size_t resid;
	size_t cnt;
	size_t l;
	int err;
	
	if (!f->reader_count)
	{
		signal_raise(SIGPIPE);
		return EPIPE;
	}
	
	resid = req->count;
	while (resid)
	{
		while (f->size >= PIPE_BUF)
		{
			if (req->no_delay)
			{
				req->count -= resid;
				return 0;
			}
			if (curr->signal_pending)
				return EINTR;
			
			pipe_ustate(f);
			
			err = task_suspend(&f->pipe.writers, WAIT_INTR);
			if (err)
				return err;
		}
		
		cnt = resid;
		
		if (cnt > PIPE_BUF)
			cnt = PIPE_BUF;
		
		if (cnt + f->size > PIPE_BUF)
			cnt = PIPE_BUF - f->size;
		
		l = cnt;
		if (l + f->pipe.write_p > PIPE_BUF)
			l = PIPE_BUF - f->pipe.write_p;
		
		memcpy(f->pipe.buf + f->pipe.write_p, buf, l);
		memcpy(f->pipe.buf, buf + l, cnt - l);
		
		f->pipe.write_p	+= cnt;
		f->pipe.write_p	%= PIPE_BUF;
		f->size		+= cnt;
		resid		-= cnt;
		buf		+= cnt;
	}
	pipe_ustate(f);
	return 0;
}

static int pipe_trunc(struct fso *fso)
{
	return ENOSYS;
}

static int pipe_chk_perm(struct fso *f, int mode, uid_t uid, gid_t gid)
{
	return 0;
}

static int pipe_link(struct fso *f, const char *name)
{
	return ENOSYS;
}

static int pipe_unlink(struct fs *fs, const char *name)
{
	return ENOSYS;
}

static int pipe_rename(struct fs *fs, const char *oldname, const char *newname)
{
	return ENOSYS;
}

static int pipe_chdir(struct fs *fs, const char *name)
{
	return ENOSYS;
}

static int pipe_mkdir(struct fs *fs, const char *name, mode_t mode)
{
	return ENOSYS;
}

static int pipe_rmdir(struct fs *fs, const char *name)
{
	return ENOSYS;
}

static int pipe_statfs(struct fs *fs, struct statfs *st)
{
	memset(st, 0, sizeof *st);
	return 0;
}

static int pipe_sync(struct fs *fs)
{
	return ENOSYS;
}

static int pipe_ioctl(struct fso *f, int cmd,  void *p)
{
	return ENOTTY;
}

struct fstype pipe_fstype =
{
	name:		"pipe",
	mount:		pipe_mount,
	umount:		pipe_umount,
	lookup:		pipe_lookup,
	creat:		pipe_creat,
	getfso:		pipe_getfso,
	putfso:		pipe_putfso,
	syncfso:	pipe_syncfso,
	readdir:	pipe_readdir,
	read:		pipe_read,
	write:		pipe_write,
	trunc:		pipe_trunc,
	chk_perm:	pipe_chk_perm,
	link:		pipe_link,
	unlink:		pipe_unlink,
	rename:		pipe_rename,
	chdir:		pipe_chdir,
	mkdir:		pipe_mkdir,
	rmdir:		pipe_rmdir,
	statfs:		pipe_statfs,
	sync:		pipe_sync,
	ioctl:		pipe_ioctl
};

struct fs pipe_fs =
{
	prefix:		"",
	prefix_l:	0,
	read_only:	0,
	no_atime:	1,
	in_use:		1,
	type:		&pipe_fstype,
	dev:		NULL
};

int sys_pipe(int fd[2])
{
	struct fs_file *file0;
	struct fs_file *file1;
	struct fso *fso;
	unsigned lfd[2];
	int err;
	
	err = fs_getfso(&fso, &pipe_fs, -1);
	if (err)
	{
		uerr(err);
		return -1;
	}
	fso->refcnt++;
	
	err = fs_getfile(&file0, fso, O_RDONLY);
	if (err)
	{
		fs_putfso(fso);
		fs_putfso(fso);
		uerr(err);
		return -1;
	}
	
	err = fs_getfile(&file1, fso, O_WRONLY);
	if (err)
	{
		fs_putfile(file0);
		fs_putfso(fso);
		uerr(err);
		return -1;
	}
	
	err = fs_getfd(&lfd[0], file0);
	if (err)
	{
		fs_putfile(file0);
		fs_putfile(file1);
		uerr(err);
		return -1;
	}
	
	err = fs_getfd(&lfd[1], file1);
	if (err)
	{
		fs_putfd(lfd[0]);
		fs_putfile(file1);
		uerr(err);
		return -1;
	}
	
	err = tucpy(fd, lfd, sizeof lfd);
	if (err)
	{
		fs_putfd(lfd[0]);
		fs_putfd(lfd[1]);
		uerr(err);
		return -1;
	}
	
	return 0;
}

void pipe_init(void)
{
}
