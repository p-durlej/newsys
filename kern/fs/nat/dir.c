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

#include <kern/clock.h>
#include <kern/block.h>
#include <kern/natfs.h>
#include <kern/errno.h>
#include <kern/task.h>
#include <kern/lib.h>
#include <kern/fs.h>
#include <sys/stat.h>

struct dirent_ptr
{
	struct nat_dirent *dirent;
	struct block *block;
};

static const char *basename(const char *pathname)
{
	char *slash = strrchr(pathname, '/');
	
	if (slash)
		return slash + 1;
	
	return pathname;
}

static int find_entry(struct fso *dir, const char *name, struct dirent_ptr *dptr)
{
	struct nat_dirent *d;
	struct block *b;
	blk_t log_nr;
	int err;
	int i;
	
	if (!S_ISDIR(dir->mode))
		return ENOTDIR;
	
	err = fs_chk_perm(dir, X_OK, curr->euid, curr->egid);
	if (err)
		return err;
	
	for (log_nr = 0; log_nr < dir->size / BLK_SIZE; log_nr++)
	{
		err = nat_bmap(dir, log_nr, 0);
		if (err)
			return err;
		
		err = blk_read(&b, dir->fs->dev, dir->nat.bmap_phys);
		if (err)
			return err;
		
		d = (void *)b->data;
		for (i = 0; i < NAT_D_PER_BLOCK; i++, d++)
		{
			if (d->first_block && !strcmp(d->name, name))
			{
				dptr->dirent = d;
				dptr->block  = b;
				return 0;
			}
		}
		
		blk_put(b);
	}
	return ENOENT;
}

static int new_entry(struct fso *dir, const char *name, int first_block)
{
	struct nat_dirent *d;
	struct block *b;
	blk_t free_blk = 0;
	int free_i;
	blk_t log_nr;
	int err;
	int i;
	
	if (!S_ISDIR(dir->mode))
		return ENOTDIR;
	
	if (!*name)
		return EEXIST;
	
	err = fs_chk_perm(dir, X_OK | W_OK, curr->euid, curr->egid);
	if (err)
		return err;
	
	if (strlen(name) > NAT_NAME_MAX)
		return ENAMETOOLONG;
	
	for (log_nr = 0; log_nr < dir->size / BLK_SIZE; log_nr++)
	{
		err = nat_bmap(dir, log_nr, 0);
		if (err)
			return err;
		
		err = blk_read(&b, dir->fs->dev, dir->nat.bmap_phys);
		if (err)
			return err;
		
		d = (void *)b->data;
		for (i = 0; i < NAT_D_PER_BLOCK; i++, d++)
		{
			if (d->first_block && !strcmp(d->name, name))
			{
				blk_put(b);
				return EEXIST;
			}
			
			if (!d->first_block && !free_blk)
			{
				free_blk = dir->nat.bmap_phys;
				free_i	 = i;
			}
		}
		
		blk_put(b);
	}
	
	if (!free_blk)
	{
		if (dir->size + BLK_SIZE > NAT_DIR_SIZE_MAX)
		{
			return EFBIG;
		}
		
		err = nat_bmap(dir, dir->size / BLK_SIZE, 1);
		if (err)
			return err;
		free_blk = dir->nat.bmap_phys;
		free_i	 = 0;
		
		err = blk_get(&b, dir->fs->dev, free_blk);
		if (err)
			return err;
		
		memset(b->data, 0, BLK_SIZE);
		b->valid = 1;
		b->dirty = 1;
		
		dir->size += BLK_SIZE;
		dir->dirty = 1;
	}
	else
	{
		err = blk_read(&b, dir->fs->dev, free_blk);
		if (err)
			return err;
	}
	
	dir->mtime = clock_time();
	dir->dirty = 1;
	
	d  = (void *)b->data;
	d += free_i;
	
	memset(d, 0, sizeof *d);
	strcpy(d->name, name);
	d->first_block = first_block;
	
	b->dirty = 1;
	blk_put(b);
	return 0;
}

static int is_empty(struct fso *dir)
{
	struct nat_dirent *d;
	struct block *b;
	blk_t log_nr;
	int err;
	int i;
	
	if (!S_ISDIR(dir->mode))
		return ENOTDIR;
	
	for (log_nr = 0; log_nr < dir->size / BLK_SIZE; log_nr++)
	{
		err = nat_bmap(dir, log_nr, 0);
		if (err)
			return err;
		
		err = blk_read(&b, dir->fs->dev, dir->nat.bmap_phys);
		if (err)
			return err;
		
		d = (void *)b->data;
		for (i = 0; i < NAT_D_PER_BLOCK; i++, d++)
			if (d->first_block)
			{
				blk_put(b);
				return ENOTEMPTY;
			}
		
		blk_put(b);
	}
	return 0;
}

static int nat_lookup_dir(struct fso **f, struct fs *fs, const char *pathname)
{
	const char *p = pathname;
	struct fso *dir;
	int err;
	
	err = fs_getfso(&dir, fs, fs->nat.root_block);
	if (err)
		return err;
	
	for (;;)
	{
		char name[NAT_NAME_MAX + 1];
		struct dirent_ptr dptr;
		char *slash;
		int len;
		
		slash = strchr(p, '/');
		if (!slash)
		{
			*f = dir;
			return 0;
		}
		len = slash - p;
		
		if (len > NAT_NAME_MAX)
		{
			fs_putfso(dir);
			return err;
		}
		
		memcpy(name, p, len);
		name[len] = 0;
		p += len + 1;
		
		err = find_entry(dir, name, &dptr);
		fs_putfso(dir);
		if (err)
			return err;
		
		err = fs_getfso(&dir, fs, dptr.dirent->first_block);
		blk_put(dptr.block);
		if (err)
			return err;
	}
}

int nat_lookup(struct fso **f, struct fs *fs, const char *pathname)
{
	const char *bn = basename(pathname);
	struct dirent_ptr dptr;
	struct fso *dir;
	int err;
	
	err = nat_lookup_dir(&dir, fs, pathname);
	if (err)
		return err;
	
	if (*bn)
	{
		err = find_entry(dir, bn, &dptr);
		fs_putfso(dir);
		if (err)
			return err;
		
		err = fs_getfso(f, fs, dptr.dirent->first_block);
		blk_put(dptr.block);
		return err;
	}
	
	*f = dir;
	return 0;
}

int nat_creat(struct fso **f, struct fs *fs, const char *name, mode_t mode, dev_t rdev)
{
	struct block *hd_block;
	struct nat_header *hd;
	struct fso *dir;
	blk_t hd_block_nr;
	int err;
	
	err = nat_lookup_dir(&dir, fs, name);
	if (err)
		return err;
	
	err = fs_chk_perm(dir, W_OK, curr->euid, curr->egid);
	if (err)
	{
		fs_putfso(dir);
		return err;
	}
	
	err = nat_balloc(fs, &hd_block_nr);
	if (err)
	{
		fs_putfso(dir);
		return err;
	}
	
	err = blk_get(&hd_block, fs->dev, hd_block_nr);
	if (err)
	{
		nat_bfree(fs, hd_block_nr);
		fs_putfso(dir);
		return err;
	}
	hd = (void *)hd_block->data;
	
	memset(hd, 0, sizeof *hd);
	
	hd->atime = hd->ctime = hd->mtime = clock_time();
	
	hd->uid	   = curr->euid;
	hd->gid	   = curr->egid;
	hd->mode   = mode;
	hd->nlink  = 1;
	hd->blocks = 1;
	hd->rdev   = rdev;
	
	hd->ndirblks  = fs->nat.ndirblks;
	hd->nindirlev = fs->nat.nindirlev;
	
	hd_block->valid = 1;
	hd_block->dirty = 1;
	blk_put(hd_block);
	
	err = new_entry(dir, basename(name), hd_block_nr);
	fs_putfso(dir);
	if (err)
	{
		nat_bfree(fs, hd_block_nr);
		return err;
	}
	
	return fs_getfso(f, fs, hd_block_nr);
}

int nat_readdir(struct fso *dir, struct fs_dirent *de, int index)
{
	struct nat_dirent nde;
	unsigned off;
	blk_t log_nr;
	int err;
	int i;
	
	if (index < 0)
		return EINVAL;
	
	err = fs_chk_perm(dir, R_OK, curr->euid, curr->egid);
	if (err)
		return err;
	
	if (!index)
	{
		de->index   = dir->index;
		de->name[0] = '.';
		de->name[1] = 0;
		return 0;
	}
	
	if (index == 1)
	{
		de->index   = dir->index;
		de->name[0] = '.';
		de->name[1] = '.';
		de->name[2] = 0;
		return 0;
	}
	
	i = index - 2;
	
	if (i >= dir->size / sizeof(struct nat_dirent))
		return ENOENT;
	
	off    = (i % NAT_D_PER_BLOCK) * sizeof(struct nat_dirent);
	log_nr =  i / NAT_D_PER_BLOCK;
	
	err = nat_bmap(dir, log_nr, 0);
	if (err)
		return 0;
	
	err = blk_pread(dir->fs->dev, dir->nat.bmap_phys, off, sizeof nde, &nde);
	if (err)
		return 0;
	
	de->index = nde.first_block;
	strcpy(de->name, nde.name);
	
	return 0;
}

int nat_link(struct fso *f, const char *name)
{
	struct fso *dir;
	int err;
	
	if (S_ISDIR(f->mode))
		return EISDIR;
	
	err = nat_lookup_dir(&dir, f->fs, name);
	if (err)
		return err;
	
	err = new_entry(dir, basename(name), f->index);
	fs_putfso(dir);
	if (err)
		return err;
	
	f->ctime = clock_time();
	f->nlink++;
	f->dirty = 1;
	return 0;
}

int nat_unlink(struct fs *fs, const char *name)
{
	struct dirent_ptr dptr;
	struct fso *dir;
	struct fso *f;
	int err;
	
	err = nat_lookup_dir(&dir, fs, name);
	if (err)
		return err;
	
	err = fs_chk_perm(dir, X_OK | W_OK, curr->euid, curr->egid);
	if (err)
	{
		fs_putfso(dir);
		return err;
	}
	
	err = find_entry(dir, basename(name), &dptr);
	if (err)
	{
		fs_putfso(dir);
		return err;
	}
	
	err = fs_getfso(&f, fs, dptr.dirent->first_block);
	if (err)
	{
		blk_put(dptr.block);
		fs_putfso(dir);
		return err;
	}
	
	if (S_ISDIR(f->mode))
	{
		blk_put(dptr.block);
		fs_putfso(dir);
		fs_putfso(f);
		return EISDIR;
	}
	
	memset(dptr.dirent, 0, sizeof *dptr.dirent);
	dptr.block->dirty = 1;
	blk_put(dptr.block);
	
	f->nlink--;
	f->dirty = 1;
	
	dir->mtime = clock_time();
	dir->dirty = 1;
	
	fs_putfso(dir);
	fs_putfso(f);
	return 0;
}

int nat_rename(struct fs *fs, const char *oldname, const char *newname)
{
	int plen = strlen(oldname);
	struct dirent_ptr dptr;
	struct fso *odir = NULL;
	struct fso *ndir = NULL;
	struct fso *f = NULL;
	int err;
	
	dptr.block = NULL;
	
	if (!strcmp(oldname, newname))
		return EINVAL;
	
	if (strlen(newname) > plen && newname[plen] == '/' && !memcmp(oldname, newname, plen))
		return EINVAL;
	
	err = nat_lookup_dir(&odir, fs, oldname);
	if (err)
		goto clean;
	
	err = fs_chk_perm(odir, X_OK | W_OK, curr->euid, curr->egid);
	if (err)
		goto clean;
	
	err = nat_lookup_dir(&ndir, fs, newname);
	if (err)
		goto clean;
	
	err = fs_chk_perm(ndir, W_OK, curr->euid, curr->egid);
	if (err)
		goto clean;
	
	err = find_entry(odir, basename(oldname), &dptr);
	if (err)
		goto clean;
	
	err = new_entry(ndir, basename(newname), dptr.dirent->first_block);
	if (err)
	{
		if (err == EEXIST)
		{
			struct dirent_ptr ndptr;
			struct fso *of;
			
			err = find_entry(ndir, basename(newname), &ndptr);
			if (err)
				goto clean;
			err = fs_getfso(&of, fs, ndptr.dirent->first_block);
			if (err)
			{
				blk_put(ndptr.block);
				goto clean;
			}
			if (S_ISDIR(of->mode) && (err = is_empty(of)))
			{
				blk_put(ndptr.block);
				fs_putfso(of);
				goto clean;
			}
			of->nlink--;
			of->dirty = 1;
			fs_putfso(of);
			
			memset(ndptr.dirent, 0, sizeof *ndptr.dirent);
			strcpy(ndptr.dirent->name, basename(newname));
			ndptr.dirent->first_block = dptr.dirent->first_block;
			ndptr.block->dirty = 1;
			blk_put(ndptr.block);
		}
	}
	
	memset(dptr.dirent, 0, sizeof *dptr.dirent);
	dptr.block->dirty = 1;
	
	odir->mtime = ndir->mtime = clock_time();
	odir->dirty = 1;
	ndir->dirty = 1;
	
clean:
	blk_put(dptr.block);
	fs_putfso(odir);
	fs_putfso(ndir);
	fs_putfso(f);
	return err;
}

int nat_chdir(struct fs *fs, const char *name)
{
	struct fso *dir;
	int err;
	
	err = nat_lookup(&dir, fs, name);
	if (err)
		return err;
	
	if (!S_ISDIR(dir->mode))
	{
		fs_putfso(dir);
		return ENOTDIR;
	}
	
	err = fs_chk_perm(dir, X_OK, curr->euid, curr->egid);
	fs_putfso(dir);
	if (err)
		return err;
	
	return 0;
}

int nat_mkdir(struct fs *fs, const char *name, mode_t mode)
{
	struct fso *dir;
	int err;
	
	err = nat_creat(&dir, fs, name, S_IFDIR | (mode & S_IRIGHTS), 0);
	if (err)
		return err;
	
	fs_putfso(dir);
	return 0;
}

int nat_rmdir(struct fs *fs, const char *name)
{
	struct dirent_ptr dptr;
	struct fso *parent;
	struct fso *dir;
	int err;
	
	err = nat_lookup_dir(&parent, fs, name);
	if (err)
		return err;
	
	err = fs_chk_perm(parent, X_OK | W_OK, curr->euid, curr->egid);
	if (err)
	{
		fs_putfso(parent);
		return err;
	}
	
	err = nat_lookup(&dir, fs, name);
	if (err)
	{
		fs_putfso(parent);
		return err;
	}
	
	err = is_empty(dir);
	if (err)
	{
		fs_putfso(parent);
		fs_putfso(dir);
		return err;
	}
	
	err = find_entry(parent, basename(name), &dptr);
	if (err)
	{
		fs_putfso(parent);
		fs_putfso(dir);
		return err;
	}
	
	parent->mtime = clock_time();
	parent->dirty = 1;
	
	memset(dptr.dirent, 0, sizeof *dptr.dirent);
	dptr.block->dirty = 1;
	blk_put(dptr.block);
	
	dir->nlink--;
	dir->dirty = 1;
	
	fs_putfso(parent);
	fs_putfso(dir);
	return 0;
}
