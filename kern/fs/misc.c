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

#include <kern/fs.h>
#include <kern/lib.h>
#include <kern/stat.h>
#include <kern/errno.h>
#include <kern/block.h>
#include <kern/task.h>
#include <fcntl.h>

int fs_creat(struct fso **fso, const char *pathname, mode_t mode, dev_t rdev)
{
	char fqname[PATH_MAX];
	char dname[PATH_MAX];
	struct fs *fs;
	int err;
	
	if (curr->euid && !S_ISREG(mode) && !S_ISFIFO(mode))
		return EPERM;
	
	err = fs_parsename4(&fs, dname, fqname, pathname);
	if (err)
		return err;
	
	err = fs->type->creat(fso, fs, dname, mode, rdev);
	if (err)
		return err;
	
	fs_notify(fqname);
	return 0;
}

int fs_chk_perm(struct fso *fso, int mode, uid_t uid, gid_t gid)
{
	mode_t r;
	mode_t w;
	mode_t x;
	
	if (fso->fs->insecure)
	{
		r = 0444;
		w = 0222;
		x = 0111;
	}
	else if (fso->uid == uid)
	{
		r = S_IRUSR;
		w = S_IWUSR;
		x = S_IXUSR;
	}
	else if (fso->gid == gid)
	{
		r = S_IRGRP;
		w = S_IWGRP;
		x = S_IXGRP;
	}
	else
	{
		r = S_IROTH;
		w = S_IWOTH;
		x = S_IXOTH;
	}
	
	if ((mode & W_OK) && fso->fs->read_only)
		return EROFS;
	
	if (!uid)
		return 0;
	
	if ((mode & R_OK) && !(fso->mode & r))
		return EACCES;
	if ((mode & W_OK) && !(fso->mode & w))
		return EACCES;
	if ((mode & X_OK) && !(fso->mode & x))
		return EACCES;
	
	return fso->fs->type->chk_perm(fso, mode, uid, gid);
}

int fs_access(const char *pathname, int mode)
{
	struct fso *f;
	int err;
	
	err = fs_lookup(&f, pathname);
	if (err)
		return err;
	
	err = fs_chk_perm(f, mode, curr->ruid, curr->rgid);
	fs_putfso(f);
	return err;
}

int fs_link(const char *oldname, const char *newname)
{
	char fqname1[PATH_MAX];
	char fqname2[PATH_MAX];
	char path1[PATH_MAX];
	char path2[PATH_MAX];
	struct fs *fs1;
	struct fs *fs2;
	struct fso *f;
	int err;
	
	err = fs_parsename4(&fs2, path1, fqname1, oldname);
	if (err)
		return err;
	
	err = fs_parsename4(&fs1, path2, fqname2, newname);
	if (err)
		return err;
	
	if (fs1 != fs2)
		return EXDEV;
	
	err = fs1->type->lookup(&f, fs1, path1);
	if (err)
		return err;
	
	if (curr->euid && f->uid != curr->euid)
	{
		fs_putfso(f);
		return EACCES;
	}
	
	err = fs1->type->link(f, path2);
	fs_putfso(f);
	if (err)
		return err;
	
	fs_notify(fqname1);
	fs_notify(fqname2);
	return 0;
}

int fs_unlink(const char *pathname)
{
	char fqname[PATH_MAX];
	char dname[PATH_MAX];
	struct fs *fs;
	int err;
	
	err = fs_parsename4(&fs, dname, fqname, pathname);
	if (err)
		return err;
	
	err = fs->type->unlink(fs, dname);
	if (err)
		return err;
	
	fs_notify(fqname);
	return 0;
}

int fs_rename(const char *oldname, const char *newname)
{
	char fqname1[PATH_MAX];
	char fqname2[PATH_MAX];
	char path1[PATH_MAX];
	char path2[PATH_MAX];
	struct fs *fs1;
	struct fs *fs2;
	int err;
	
	err = fs_parsename4(&fs1, path1, fqname1, oldname);
	if (err)
		return err;
	
	err = fs_parsename4(&fs2, path2, fqname2, newname);
	if (err)
		return err;
	
	if (fs1 != fs2)
		return EXDEV;
	
	err = fs1->type->rename(fs1, path1, path2);
	if (err)
		return err;
	
	fs_notify(fqname1);
	fs_notify(fqname2);
	return 0;
}

int fs_chdir(const char *pathname)
{
	char dname[PATH_MAX];
	char fqname[PATH_MAX];
	struct fs *fs;
	int err;
	int l;
	
	err = fs_fqname(fqname, pathname);
	if (err)
		return err;
	
	l = strlen(fqname);
	if (l >= PATH_MAX)
		return ENAMETOOLONG;
	
	err = fs_parsename(&fs, dname, pathname);
	if (err)
		return err;
	
	err = fs->type->chdir(fs, dname);
	if (err)
		return err;
	
	memcpy(curr->cwd, fqname, l);
	curr->cwd[l] = 0;
	return 0;
}

int fs_mkdir(const char *pathname, mode_t mode)
{
	char fqname[PATH_MAX];
	char dname[PATH_MAX];
	struct fs *fs;
	int err;
	
	err = fs_parsename4(&fs, dname, fqname, pathname);
	if (err)
		return err;
	
	err = fs->type->mkdir(fs, dname, mode);
	if (err)
		return err;
	
	fs_notify(fqname);
	return 0;
}

int fs_rmdir(const char *pathname)
{
	char fqname[PATH_MAX];
	char dname[PATH_MAX];
	struct fs *fs;
	int err;
	
	err = fs_parsename4(&fs, dname, fqname, pathname);
	if (err)
		return err;
	
	err = fs->type->rmdir(fs, dname);
	if (err)
		return err;
	
	fs_notify(fqname);
	return 0;
}

int fs_revoke(const char *pathname)
{
	struct fso *fso;
	int err;
	int i, n;
	
	err = fs_lookup(&fso, pathname);
	if (err)
		return err;
	
	if (curr->euid && curr->euid != fso->uid)
	{
		fs_putfso(fso);
		return EACCES;
	}
	
	for (i = 0; i < TASK_MAX; i++)
	{
		if (task[i] == NULL)
			continue;
		for (n = 0; n < OPEN_MAX; n++)
			if (task[i]->file_desc[n].file && task[i]->file_desc[n].file->fso == fso)
			{
				task[i]->file_desc[n].file->omode &= ~O_DIRECTION;
				task[i]->file_desc[n].file->omode |=  O_NOIO;
			}
	}
	
	fs_putfso(fso);
	return 0;
}
