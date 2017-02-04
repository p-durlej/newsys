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

#define BLK_SIZE	512	/* XXX */

#include <sysload/console.h>
#include <sysload/disk.h>
#include <sysload/fs.h>
#include <priv/natfs.h>
#include <kern/stat.h>

#include <string.h>
#include <errno.h>

#define min(a, b)	((a) < (b) ? (a) : (b))

static int fs_native_mount(struct fs *fs);
static int fs_native_lookup(struct file *f, struct fs *fs, const char *pathname);
static int fs_native_load(struct file *f, void *buf, size_t sz);

static struct fs_native_bce
{
	char		buf[BLK_SIZE];
	blk_t		block;
	struct disk *	disk;
} fs_native_cache[4];

static struct fstype fs_native_fst =
{
	.name	= "native",
	.mount	= fs_native_mount,
	.lookup	= fs_native_lookup,
	.load	= fs_native_load,
};

static int fs_native_mount(struct fs *fs)
{
	char buf[512];
	int err;
	
	err = disk_read(fs->disk, 1, buf);
	if (err)
		return err;
	fs->sb = *(struct nat_super *)buf;
	if (memcmp(&fs->sb.magic, NAT_MAGIC, sizeof fs->sb.magic))
		return EINVAL;
	return 0;
}

static int fs_native_read_bmap(struct disk *disk, blk_t block, int level, void *bufp)
{
	struct fs_native_bce *bce = &fs_native_cache[level % 4];
	int err;
	
	if (bce->disk == disk && bce->block == block)
		goto fini;
	
	err = disk_read(disk, block, bce->buf);
	if (err)
		return err;
	
	bce->block = block;
	bce->disk  = disk;
fini:
	*(void **)bufp = bce->buf;
	return 0;
}

static int fs_native_bmap(struct file *f, blk_t log, blk_t *phys)
{
	struct nat_header *hd;
	struct fs *fs = f->fs;
	uint32_t *buf;
	blk_t bn;
	int shift;
	int indl;
	int err;
	int il;
	int i;
	
	err = fs_native_read_bmap(fs->disk, f->index, 0, &hd);
	if (err)
		return err;
	
	if (log < hd->ndirblks)
	{
		*phys = hd->bmap[log];
		return 0;
	}
	
	shift  = 7 * hd->nindirlev;
	i      = hd->ndirblks + (log >> shift);
	bn     = hd->bmap[i];
	shift -= 7;
	il     = 1;
	
	while (bn && shift >= 0)
	{
		err = fs_native_read_bmap(fs->disk, bn, il++, &buf);
		if (err)
			return err;
		
		bn = buf[(log >> shift) & 127];
		shift -= 7;
	}
	
	*phys = bn;
	return 0;
}

static int fs_native_find_entry(struct file *f, blk_t *bp, const char *name)
{
	union
	{
		struct nat_dirent de[NAT_D_PER_BLOCK];
		struct nat_header hd;
	} u;
	struct fs *fs = f->fs;
	blk_t phys = *bp;
	blk_t size;
	blk_t log;
	int err;
	int i;
	
	err = disk_read(fs->disk, phys, &u.hd);
	if (err)
		return err;
	if (!S_ISDIR(u.hd.mode))
		return ENOTDIR;
	size = u.hd.size >> 9;
	
	for (log = 0; log < size; log++)
	{
		err = fs_native_bmap(f, log, &phys);
		if (err)
			return err;
		if (!phys)
			continue;
		
		err = disk_read(fs->disk, phys, u.de);
		if (err)
			return err;
		
		for (i = 0; i < NAT_D_PER_BLOCK; i++)
		{
			if (u.de[i].name[NAT_NAME_MAX])
				return EINVAL;
			if (!strcmp(u.de[i].name, name))
			{
				*bp = u.de[i].first_block;
				return 0;
			}
		}
	}
	return ENOENT;
}

static int fs_native_lookup(struct file *f, struct fs *fs, const char *pathname)
{
	union { char buf[512]; struct nat_header hd; } u;
	char name[NAT_NAME_MAX + 1];
	const char *p0, *p1;
	struct file dir;
	int err;
	blk_t b;
	
	dir.fs    = fs;
	dir.index = fs->sb.root_block;
	dir.size  = -1;
	
	b  = fs->sb.root_block;
	p0 = pathname;
	for (;;)
	{
		while (*p0 == '/')
			p0++;
		if (!*p0)
			break;
		
		p1 = strchr(p0, '/');
		if (!p1)
			p1 = strchr(p0, 0);
		
		if (p1 - p0 > sizeof name)
			return ENAMETOOLONG;
		
		memcpy(name, p0, p1 - p0);
		name[p1 - p0] = 0;
		p0 = p1;
		
		err = fs_native_find_entry(&dir, &b, name);
		if (err)
			return err;
		dir.index = b;
	}
	
	err = disk_read(fs->disk, b, u.buf);
	if (err)
		return err;
	
	f->size	 = u.hd.size;
	f->index = b;
	f->fs	 = fs;
	return 0;
}

static int fs_native_load(struct file *f, void *buf, size_t sz)
{
	char lbuf[512];
	off_t resid;
	blk_t phys;
	blk_t log;
	char *p;
	int err;
	int cl;
	
	resid = sz;
	p     = buf;
	log   = 0;
	while (resid)
	{
		err = fs_native_bmap(f, log, &phys);
		if (err)
			return err;
		
		if (phys)
		{
			err = disk_read(f->fs->disk, phys, lbuf);
			if (err)
				return err;
			
			cl = min(resid, 512);
			memcpy(p, lbuf, cl);
		}
		else
			memset(p, 0, min(resid, 512));
		
		resid -= cl;
		p += cl;
		log++;
	}
	return 0;
}

void fs_native_init(void)
{
	fs_native_fst.next = fs_types;
	fs_types = &fs_native_fst;
}
