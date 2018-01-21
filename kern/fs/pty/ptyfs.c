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

#include <kern/task_queue.h>
#include <kern/config.h>
#include <kern/signal.h>
#include <kern/printk.h>
#include <kern/clock.h>
#include <kern/sched.h>
#include <kern/ptyfs.h>
#include <kern/errno.h>
#include <kern/task.h>
#include <kern/stat.h>
#include <kern/umem.h>
#include <kern/lib.h>
#include <kern/fs.h>
#include <termios.h>
#include <os386.h>

#define ROOT_INDEX	1
#define PTMX_INDEX	2
#define PTM_INDEX	3
#define PTS_INDEX	(PTM_INDEX + PTY_MAX)

struct pty
{
	struct task_queue ptm_readers;
	struct task_queue ptm_writers;
	struct task_queue pts_readers;
	struct task_queue pts_writers;
	
	struct fso *ptm_fso;
	struct fso *pts_fso;
	int locked;
	
	uid_t uid;
	gid_t gid;
	
	char canon_buf[MAX_CANON];
	char in_buf[1870];
	char out_buf[1870];
	
	struct pty_size size;
	struct termios tio;
	
	int canon_count;
	int canon_intr;
	int canon_eot;
	
	int in_count;
	int in_p0;
	int in_p1;
	
	int out_count;
	int out_p0;
	int out_p1;
} *pty[PTY_MAX];

static int pty_mounted;

static void pty_signal(struct pty *pp, int nr);

static void pty_resume_ptm_r(struct pty *pp)
{
	struct task *t;
	
	if (pp->ptm_fso != NULL)
		fs_setstate(pp->ptm_fso, R_OK);
	
	while ((t = task_dequeue(&pp->ptm_readers)))
		task_resume(t);
}

static void pty_resume_ptm_w(struct pty *pp)
{
	struct task *t;
	
	if (pp->ptm_fso != NULL)
		fs_setstate(pp->ptm_fso, W_OK);
	
	while ((t = task_dequeue(&pp->ptm_writers)))
		task_resume(t);
}

static void pty_resume_pts_r(struct pty *pp)
{
	struct task *t;
	
	if (pp->pts_fso != NULL)
		fs_setstate(pp->pts_fso, R_OK);
	
	while ((t = task_dequeue(&pp->pts_readers)))
		task_resume(t);
}

static void pty_resume_pts_w(struct pty *pp)
{
	struct task *t;
	
	if (pp->pts_fso != NULL)
		fs_setstate(pp->pts_fso, W_OK);
	
	while ((t = task_dequeue(&pp->pts_writers)))
		task_resume(t);
}

static int pty_mount(struct fs *fs)
{
	if (pty_mounted)
		return EBUSY;
	pty_mounted = 1;
	return 0;
}

static int pty_umount(struct fs *fs)
{
	pty_mounted = 0;
	return 0;
}

#define isdigit(ch)	((ch) >= '0' && ch <= '9')
#define ispts(name)	(!strncmp((name), "pts", 3) && isdigit((name)[3]) && \
			 isdigit((name)[4]) && isdigit((name)[5]) && \
			 !(name)[6])

static int pty_lookup(struct fso **f, struct fs *fs, const char *name)
{
	int err;
	int i;
	
	if (!*name)
		return fs_getfso(f, fs, ROOT_INDEX);
	
	if (!strcmp(name, "ptmx"))
	{
		for (i = 0; i < PTY_MAX; i++)
			if (!pty[i])
			{
				err = kmalloc(&pty[i], sizeof *pty[i], "pty");
				if (err)
					return err;
				
				memset(pty[i], 0, sizeof *pty[i]);
				
				task_qinit(&pty[i]->ptm_readers, "ptmrd");
				task_qinit(&pty[i]->ptm_writers, "ptmwr");
				task_qinit(&pty[i]->pts_readers, "ptsrd");
				task_qinit(&pty[i]->pts_writers, "ptswr");
				
				err = fs_getfso(f, fs, PTM_INDEX + i);
				if (err)
					return err;
				return 0;
			}
		
		return ENOMEM;
	}
	
	if (ispts(name))
	{
		i  = (name[3] - '0') * 100;
		i += (name[4] - '0') * 10;
		i +=  name[5] - '0';
		
		if (i >= PTY_MAX)
			return ENOENT;
		if (!pty[i])
			return ENOENT;
		if (pty[i]->locked)
			return EPERM;
		
		return fs_getfso(f, fs, PTS_INDEX + i);
	}
	
	return ENOENT;
}

static int pty_creat(struct fso **f, struct fs *fs, const char *name, mode_t mode, dev_t rdev)
{
	return EPERM;
}

static int pty_getfso(struct fso *f)
{
	f->atime  = f->ctime = f->mtime = clock_time();
	f->blocks = 0;
	f->nlink  = 1;
	f->size   = 0;
	
	if (f->index == ROOT_INDEX)
	{
		f->uid  = 0;
		f->gid  = 0;
		f->mode = S_IFDIR | 0755;
		fs_state(f, R_OK | W_OK);
		return 0;
	}
	
	f->rdev = makedev(2, f->index);
	
	if (f->index >= PTM_INDEX && f->index < PTM_INDEX + PTY_MAX)
	{
		int i = f->index - PTM_INDEX;
		struct pty *p = pty[i];
		
		p->tio.c_iflag = 0;
		p->tio.c_oflag = 0;
		p->tio.c_cflag = 0;
		p->tio.c_lflag = ICANON | ISIG | ECHO | ECHONL;
		
		p->tio.c_cc[VINTR]    = 'C'  & 0x1f;
		p->tio.c_cc[VQUIT]    = '\\' & 0x1f;
		p->tio.c_cc[VERASE]   = '\b';
		p->tio.c_cc[VKILL]    = 'U'  & 0x1f;
		p->tio.c_cc[VEOF]     = 'D'  & 0x1f;
		p->tio.c_cc[VWERASE]  = 'W'  & 0x1f;
		p->tio.c_cc[VSTATUS]  = 'T'  & 0x1f;
		p->tio.c_cc[VREPRINT] = 'R'  & 0x1f;
		
		p->ptm_fso = f;
		p->locked  = 1;
		
		f->mode	   = S_IFCHR | 0600;
		f->uid	   = curr->euid;
		f->gid	   = curr->egid;
		f->no_seek = 1;
		fs_state(f, W_OK);
		
		return 0;
	}
	
	if (f->index >= PTS_INDEX && f->index < PTS_INDEX + PTY_MAX)
	{
		int i = f->index - PTS_INDEX;
		int state = 0;
		
		pty[i]->pts_fso = f;
		
		f->mode    = S_IFCHR | 0600;
		f->uid	   = pty[i]->ptm_fso->uid;
		f->gid	   = pty[i]->ptm_fso->gid;
		f->no_seek = 1;
		if (pty[i]->out_count < sizeof pty[i]->out_buf)
			state |= W_OK;
		if (pty[i]->in_count)
			state |= R_OK;
		fs_setstate(f, state);
		
		return 0;
	}
	
	return EINVAL;
}

static int pty_putfso(struct fso *f)
{
	if (f->index >= PTM_INDEX && f->index < PTM_INDEX + PTY_MAX)
	{
		if (pty[f->index - PTM_INDEX]->pts_fso)
		{
			pty[f->index - PTM_INDEX]->ptm_fso = NULL;
			pty_resume_pts_r(pty[f->index - PTM_INDEX]);
			pty_resume_pts_w(pty[f->index - PTM_INDEX]);
		}
		else
		{
			free(pty[f->index - PTM_INDEX]);
			pty[f->index - PTM_INDEX] = NULL;
		}
	}
	
	if (f->index >= PTS_INDEX && f->index < PTS_INDEX + PTY_MAX)
	{
		if (pty[f->index - PTS_INDEX]->ptm_fso)
		{
			pty[f->index - PTS_INDEX]->pts_fso = NULL;
			pty_resume_ptm_r(pty[f->index - PTS_INDEX]);
		}
		else
		{
			free(pty[f->index - PTS_INDEX]);
			pty[f->index - PTS_INDEX] = NULL;
		}
	}
	
	return 0;
}

static int pty_syncfso(struct fso *f)
{
	return 0;
}

static int pty_readdir(struct fso *dir, struct fs_dirent *de, int index)
{
	if (index == 0)
	{
		strcpy(de->name, ".");
		de->index = ROOT_INDEX;
		return 0;
	}
	if (index == 1)
	{
		strcpy(de->name, "..");
		de->index = ROOT_INDEX;
		return 0;
	}
	index -= 2;
	
	if (index == PTMX_INDEX)
	{
		strcpy(de->name, "ptmx");
		de->index = PTMX_INDEX;
		return 0;
	}
	
	if (index >= PTS_INDEX && index < PTS_INDEX + PTY_MAX && pty[index - PTS_INDEX])
	{
		int i = index - PTS_INDEX;
		
		strcpy(de->name, "ptsXXX");
		de->name[3] = '0' + (i / 100) % 10;
		de->name[4] = '0' + (i / 10)  % 10;
		de->name[5] = '0' +  i	      % 10;
		de->index   = index;
		return 0;
	}
	
	if (index < PTS_INDEX + PTY_MAX)
	{
		de->index = 0;
		return 0;
	}
	
	return ENOENT;
}

static int pty_ioctl(struct fso *fso, int cmd, void *buf)
{
	struct pty_size psz;
	struct termios ti;
	char name[7];
	int err;
	int i;
	
	if (fso->index >= PTM_INDEX && fso->index < PTM_INDEX + PTY_MAX)
	{
		i = fso->index - PTM_INDEX;
		
		switch (cmd)
		{
		case PTY_PTSNAME:
			err = fs_chk_perm(fso, R_OK, curr->euid, curr->egid);
			if (err)
				return err;
			strcpy(name, "ptsXXX");
			name[3] = '0' + (i / 100) % 10;
			name[4] = '0' + (i / 10)  % 10;
			name[5] = '0' +  i	  % 10;
			return tucpy(buf, name, 7);
		case PTY_UNLOCK:
			err = fs_chk_perm(fso, W_OK, curr->euid, curr->egid);
			if (err)
				return err;
			pty[i]->locked = 0;
			return 0;
		case PTY_SHUP:
			err = fs_chk_perm(fso, W_OK, curr->euid, curr->egid);
			if (err)
				return err;
			pty_signal(pty[i], SIGHUP);
			return 0;
		case TCSETS:
		case TCSETSW:
		case TCSETSF:
			err = fs_chk_perm(fso, W_OK, curr->euid, curr->egid);
			if (err)
				return err;
			
			err = fucpy(&ti, buf, sizeof ti);
			if (err)
				return err;
			pty[i]->tio = ti;
			return 0;
		case TCGETS:
			err = fs_chk_perm(fso, R_OK, curr->euid, curr->egid);
			if (err)
				return err;
			return tucpy(buf, &pty[i]->tio, sizeof ti);
		case PTY_GSIZE:
			psz = pty[i]->size;
			
			return tucpy(buf, &psz, sizeof psz);
		case PTY_SSIZE:
			err = fucpy(&psz, buf, sizeof psz);
			if (err)
				return err;
			
			pty[i]->size = psz;
			return 0;
		default:
			return ENOTTY;
		}
	}
	
	if (fso->index >= PTS_INDEX && fso->index < PTS_INDEX + PTY_MAX)
	{
		i = fso->index - PTS_INDEX;
		
		switch (cmd)
		{
		case PTY_PTSNAME:
			err = fs_chk_perm(fso, R_OK, curr->euid, curr->egid);
			if (err)
				return err;
			strcpy(name, "ptsXXX");
			name[3] = '0' + (i / 100) % 10;
			name[4] = '0' + (i / 10)  % 10;
			name[5] = '0' +  i	  % 10;
			return tucpy(buf, name, 7);
		case TCSETS:
		case TCSETSW:
		case TCSETSF:
			err = fs_chk_perm(fso, W_OK, curr->euid, curr->egid);
			if (err)
				return err;
			
			err = fucpy(&ti, buf, sizeof ti);
			if (err)
				return err;
			pty[i]->tio = ti;
			return 0;
		case TCGETS:
			err = fs_chk_perm(fso, R_OK, curr->euid, curr->egid);
			if (err)
				return err;
			return tucpy(buf, &pty[i]->tio, sizeof ti);
		case PTY_GSIZE:
			psz = pty[i]->size;
			
			return tucpy(buf, &psz, sizeof psz);
		case PTY_SSIZE:
			err = fucpy(&psz, buf, sizeof psz);
			if (err)
				return err;
			
			pty[i]->size = psz;
			return 0;
		default:
			return ENOTTY;
		}
	}
	
	return ENOTTY;
}

static int ptm_read(struct fs_rwreq *req)
{
	struct pty *pp = pty[req->fso->index - PTM_INDEX];
	char *buf = req->buf;
	int left;
	int err;
	char ch;
	
	if (!pp)
		panic("ptm_read: !pp");
	
	while (!pp->out_count)
	{
		if (req->no_delay)
			return EAGAIN;
		err = task_suspend(&pp->ptm_readers, WAIT_INTR);
		if (err)
			return err;
	}
	
	if (req->count > pp->out_count)
		req->count = pp->out_count;
	left = req->count;
	
	while (left)
	{
		ch = pp->out_buf[pp->out_p1];
		*buf = ch;
		
		pp->out_count--;
		pp->out_p1++;
		pp->out_p1 %= sizeof pp->out_buf;
		left--;
		buf++;
	}
	if (!pp->out_count)
		fs_clrstate(pp->ptm_fso, R_OK);
	pty_resume_pts_w(pp);
	return 0;
}

static int pts_read(struct fs_rwreq *req)
{
	struct pty *pp = pty[req->fso->index - PTS_INDEX];
	char *buf = req->buf;
	int left;
	int err;
	char ch;
	
	if (!pp)
		panic("pts_read: !pp");
	
restart:
	if (!pp->ptm_fso)
	{
		signal_raise(SIGHUP);
		req->count = 0;
		return 0;
	}
	if (pp->canon_intr)
	{
		signal_raise(SIGINT);
		pp->canon_count	= 0;
		pp->canon_intr	= 0;
		return EINTR;
	}
	
	while (!pp->in_count)
	{
		if (req->no_delay)
		{
			req->count = EAGAIN;
			return 0;
		}
		if (pp->canon_intr || !pp->ptm_fso)
			goto restart;
		if (pp->canon_eot)
		{
			pp->canon_eot = 0;
			break;
		}
		err = task_suspend(&pp->pts_readers, WAIT_INTR);
		if (err)
			return err;
	}
	
	if (req->count > pp->in_count)
		req->count = pp->in_count;
	left = req->count;
	
	while (left)
	{
		ch = pp->in_buf[pp->in_p1];
		*buf = ch;
		
		pp->in_count--;
		pp->in_p1++;
		pp->in_p1 %= sizeof pp->in_buf;
		left--;
		buf++;
	}
	if (!pp->in_count)
		fs_clrstate(pp->pts_fso, R_OK);
	pty_resume_ptm_w(pp);
	return 0;
}

static int pty_read(struct fs_rwreq *req)
{
	int i;
	
	i = req->fso->index;
	
	if (i >= PTM_INDEX && i < PTM_INDEX + PTY_MAX)
		return ptm_read(req);
	
	if (i >= PTS_INDEX && i < PTS_INDEX + PTY_MAX)
		return pts_read(req);
	
	return EPERM;
}

static void pty_echo(struct pty *pp, char ch)
{
	if (pp->out_count != sizeof pp->out_buf)
	{
		pp->out_buf[pp->out_p0] = ch;
		pp->out_count++;
		pp->out_p0++;
		pp->out_p0 %= sizeof pp->out_buf;
		
		pty_resume_ptm_r(pp);
	}
}

static void pty_cecho(struct pty *pp, char ch)
{
	if (ch >= 0 && ch < 0x20)
	{
		pty_echo(pp, '^');
		pty_echo(pp, ch + 0x40);
	}
	else if (ch == 127)
	{
		pty_echo(pp, '^');
		pty_echo(pp, '?');
	}
	else
		pty_echo(pp, ch);
}

static void pty_signal(struct pty *pp, int nr)
{
	struct fs_desc *f;
	struct fs_desc *e;
	struct task **p;
	struct fso *fso;
	
	fso = pp->pts_fso;
	
	for (p = task; p < task + TASK_MAX; p++)
	{
		if (!*p)
			continue;
		
		f = (*p)->file_desc;
		e = f + OPEN_MAX;
		for (; f < e; f++)
			if (f->file && f->file->fso == fso)
			{
				signal_send_k(*p, nr);
				break;
			}
	}
}

static void pty_erase_ch(struct pty *pp, char ch)
{
	if ((unsigned char)ch < 0x20 || ch == 127)
	{
		pty_echo(pp, '\b');
		pty_echo(pp, '\b');
		pty_echo(pp, ' ');
		pty_echo(pp, ' ');
		pty_echo(pp, '\b');
		pty_echo(pp, '\b');
	}
	else
	{
		pty_echo(pp, '\b');
		pty_echo(pp, ' ');
		pty_echo(pp, '\b');
	}
}

static void pty_canon_input(struct pty *pp, char ch)
{
	int i;
	
	if (ch == pp->tio.c_cc[VKILL])
	{
		while (pp->canon_count)
		{
			pp->canon_count--;
			if (pp->tio.c_lflag & ECHO)
				pty_erase_ch(pp, pp->canon_buf[pp->canon_count]);
		}
		return;
	}
	
	if (ch == pp->tio.c_cc[VEOF])
	{
		pp->canon_eot = 1;
		if (pp->tio.c_lflag & ECHO)
		{
			pty_cecho(pp, ch);
			pty_echo(pp, '\n');
		}
		return;
	}
	
	if (ch == pp->tio.c_cc[VINTR])
	{
		if (pp->tio.c_lflag & ISIG)
		{
			pty_signal(pp, SIGINT);
			pp->canon_count = 0;
		}
		if (pp->tio.c_lflag & ECHO)
		{
			pty_cecho(pp, ch);
			pty_echo(pp, '\n');
		}
		return;
	}
	
	if (ch == pp->tio.c_cc[VREPRINT])
	{
		if (pp->tio.c_lflag & ECHO)
		{
			pty_echo(pp, '\n');
			pty_echo(pp, '>');
			pty_echo(pp, ' ');
			for (i = 0; i < pp->canon_count; i++)
				pty_cecho(pp, pp->canon_buf[i]);
		}
		return;
	}
	
	if (ch == pp->tio.c_cc[VQUIT])
	{
		if (pp->tio.c_lflag & ISIG)
		{
			pty_signal(pp, SIGQUIT);
			pp->canon_count = 0;
		}
		if (pp->tio.c_lflag & ECHO)
		{
			pty_echo(pp, '^');
			pty_echo(pp, '\\');
			pty_echo(pp, '\n');
		}
		return;
	}
	
	if (ch == pp->tio.c_cc[VERASE])
	{
		if (pp->canon_count)
		{
			pp->canon_count--;
			if (pp->tio.c_lflag & ECHO)
				pty_erase_ch(pp, pp->canon_buf[pp->canon_count]);
		}
		return;
	}
	
	if (ch == '\r')
		return;
	
	if (ch == '\n')
	{
		if (pp->canon_count < MAX_CANON)
		{
			pp->canon_buf[pp->canon_count++] = '\n';
			if (pp->tio.c_lflag & (ECHO | ECHONL))
				pty_echo(pp, '\n');
		}
		for (i = 0; i < pp->canon_count; i++)
		{
			if (pp->in_count >= sizeof pp->in_buf)
				break;
			
			pp->in_buf[pp->in_p0] = pp->canon_buf[i];
			pp->in_p0++;
			pp->in_p0 %= sizeof pp->in_buf;
			pp->in_count++;
		}
		pp->canon_count = 0;
		pty_resume_pts_r(pp);
		return;
	}
	
	if (pp->canon_count < MAX_CANON)
	{
		pp->canon_buf[pp->canon_count++] = ch;
		if (pp->tio.c_lflag & ECHO)
			pty_cecho(pp, ch);
	}
}

static int ptm_write(struct fs_rwreq *req)
{
	struct pty *pp = pty[req->fso->index - PTM_INDEX];
	char *buf = req->buf;
	int left = req->count;
	int err;
	char ch;
	
	if (!left)
		return 0;
	
	while (left)
	{
		if ((err = fucpy(&ch, buf, 1)))
			goto fail;
		
		if (pp->tio.c_lflag & ICANON)
			pty_canon_input(pp, ch);
		else
		{
			while (pp->in_count >= sizeof pp->in_buf)
			{
				if (req->no_delay)
				{
					req->count -= left;
					goto fini;
				}
				if (curr->signal_pending)
				{
					err = EINTR;
					goto fail;
				}
				pty_resume_pts_r(pp);
				err = task_suspend(&pp->ptm_writers, WAIT_INTR);
				if (err)
					goto fail;
			}
			
			pp->in_buf[pp->in_p0] = ch;
			pp->in_count++;
			pp->in_p0++;
			pp->in_p0 %= sizeof pp->in_buf;
			
			if (pp->tio.c_lflag & ECHO)
				pty_echo(pp, ch);
		}
		left--;
		buf++;
	}
	
fini:
	err = 0;
fail:
	if (pp->in_count >= sizeof pp->in_buf)
		fs_clrstate(pp->ptm_fso, W_OK);
	pty_resume_pts_r(pp);
	
	req->count -= left;
	if (req->count)
		return 0;
	return err;
}

static int pts_write(struct fs_rwreq *req)
{
	struct pty *pp = pty[req->fso->index - PTS_INDEX];
	char *buf = req->buf;
	int left = req->count;
	int err;
	char ch;
	
	if (!left)
		return 0;
	
	while (left)
	{
		if ((err = fucpy(&ch, buf, 1)))
			goto fail;
		
		while (pp->out_count >= sizeof pp->out_buf)
		{
			if (!pp->ptm_fso)
			{
				signal_raise(SIGHUP);
				req->count = 0;
				goto fini;
			}
			if (pp->canon_intr)
			{
				signal_raise(SIGINT);
				pp->canon_intr = 0;
				err = EINTR;
				goto fail;
			}
			if (req->no_delay)
				goto fini;
			if (curr->signal_pending)
			{
				err = EINTR;
				goto fail;
			}
			pty_resume_ptm_r(pp);
			err = task_suspend(&pp->pts_writers, WAIT_INTR);
			if (err)
				goto fail;
		}
		
		pp->out_buf[pp->out_p0] = ch;
		pp->out_count++;
		pp->out_p0++;
		pp->out_p0 %= sizeof pp->out_buf;
		left--;
		buf++;
	}

fini:
	err = 0;
fail:
	if (pp->out_count >= sizeof pp->out_buf)
		fs_clrstate(pp->pts_fso, W_OK);
	pty_resume_ptm_r(pp);
	
	req->count -= left;
	if (req->count)
		return 0;
	return err;
}

static int pty_write(struct fs_rwreq *req)
{
	int i;
	
	i = req->fso->index;
	
	if (i >= PTM_INDEX && i < PTM_INDEX + PTY_MAX)
		return ptm_write(req);
	
	if (i >= PTS_INDEX && i < PTS_INDEX + PTY_MAX)
		return pts_write(req);
	
	return EPERM;
}

static int pty_trunc(struct fso *fso)
{
	return 0;
}

static int pty_chk_perm(struct fso *f, int mode, uid_t uid, gid_t gid)
{
	return 0;
}

static int pty_link(struct fso *f, const char *name)
{
	return EPERM;
}

static int pty_unlink(struct fs *fs, const char *name)
{
	return EPERM;
}

static int pty_rename(struct fs *fs, const char *oldname, const char *newname)
{
	return EPERM;
}

static int pty_chdir(struct fs *fs, const char *name)
{
	if (*name)
		return ENOENT;
	
	return 0;
}

static int pty_mkdir(struct fs *fs, const char *name, mode_t mode)
{
	return EPERM;
}

static int pty_rmdir(struct fs *fs, const char *name)
{
	return EPERM;
}

static int pty_statfs(struct fs *fs, struct statfs *st)
{
	return 0;
}

static int pty_sync(struct fs *fs)
{
	return 0;
}

static int pty_ttyname(struct fso *fso, char *buf, int buf_size)
{
	int i = fso->index - PTS_INDEX;
	char *p;
	int l;
	
	l = strlen(fso->fs->prefix);
	p = buf + l;
	if (l + 8 > buf_size)
		return ENOMEM;
	
	strcpy(buf, fso->fs->prefix);
	*p = '/';
	strcpy(p + 1, "ptsXXX");
	p[4] = '0' + (i / 100) % 10;
	p[5] = '0' + (i / 10)  % 10;
	p[6] = '0' +  i	       % 10;
	return 0;
}

struct fstype ptsfs_type =
{
	name:		"pty",
	umount:		pty_umount,
	mount:		pty_mount,
	lookup:		pty_lookup,
	creat:		pty_creat,
	getfso:		pty_getfso,
	putfso:		pty_putfso,
	syncfso:	pty_syncfso,
	readdir:	pty_readdir,
	ioctl:		pty_ioctl,
	read:		pty_read,
	write:		pty_write,
	trunc:		pty_trunc,
	chk_perm:	pty_chk_perm,
	link:		pty_link,
	unlink:		pty_unlink,
	rename:		pty_rename,
	chdir:		pty_chdir,
	mkdir:		pty_mkdir,
	rmdir:		pty_rmdir,
	statfs:		pty_statfs,
	sync:		pty_sync,
	ttyname:	pty_ttyname
};

void pty_init(void)
{
#if KVERBOSE
	printk("pty_init: sizeof(struct pty) = %i\n", sizeof(struct pty));
#endif
	fs_install(&ptsfs_type);
}
