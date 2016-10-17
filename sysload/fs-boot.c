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

#include <sysload/disk.h>
#include <sysload/fs.h>
#include <priv/bootfs.h>

#include <string.h>
#include <errno.h>

#define min(a, b)	((a) < (b) ? (a) : (b))

static int fs_boot_mount(struct fs *fs);
static int fs_boot_lookup(struct file *f, struct fs *fs, const char *pathname);
static int fs_boot_load(struct file *f, void *buf, size_t sz);

static struct fstype fs_boot_fst =
{
	.name	= "boot",
	.mount	= fs_boot_mount,
	.lookup	= fs_boot_lookup,
	.load	= fs_boot_load,
};

static int fs_boot_mount(struct fs *fs)
{
	return 0;
}

static int fs_boot_lookup(struct file *f, struct fs *fs, const char *pathname)
{
	union { struct bfs_head h; char buf[512]; } u;
	blk_t blk = 128;
	int err;
	
	do
	{
		err = disk_read(f->fs->disk, blk, &u.buf);
		if (err)
			return err;
		
		if (u.h.name[sizeof u.h.name - 1])
			return EINVAL;
		if (!strcmp(u.h.name, pathname))
		{
			f->index = blk;
			f->size	 = u.h.size;
			return 0;
		}
		blk = u.h.next;
	} while (blk);
	
	return ENOENT;
}

static int fs_boot_load(struct file *f, void *buf, size_t sz)
{
	char lbuf[512];
	off_t resid;
	blk_t blk;
	char *p;
	int err;
	int cl;
	
	resid = sz;
	blk   = f->index + 1;
	p     = buf;
	while (resid)
	{
		err = disk_read(f->fs->disk, blk, lbuf);
		if (err)
			return err;
		cl = min(resid, 512);
		memcpy(p, lbuf, cl);
		
		resid -= cl;
		p += cl;
		blk++;
	}
	return 0;
}

void fs_boot_init(void)
{
	fs_boot_fst.next = fs_types;
	fs_types = &fs_boot_fst;
}
