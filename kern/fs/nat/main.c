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

#include <kern/printk.h>
#include <kern/block.h>
#include <kern/natfs.h>
#include <kern/errno.h>
#include <kern/stat.h>
#include <kern/task.h>
#include <kern/umem.h>
#include <kern/fs.h>
#include <kern/hw.h>

#include <sys/types.h>
#include <sys/ioctl.h>

static struct fstype fstype =
{
	.name		= "native",
	.mount		= nat_mount,
	.umount		= nat_umount,
	.lookup		= nat_lookup,
	.creat		= nat_creat,
	.getfso		= nat_getfso,
	.putfso		= nat_putfso,
	.syncfso	= nat_syncfso,
	.readdir	= nat_readdir,
	.ioctl		= nat_ioctl,
	.read		= nat_read,
	.write		= nat_write,
	.trunc		= nat_trunc,
	.chk_perm	= nat_chk_perm,
	.link		= nat_link,
	.unlink		= nat_unlink,
	.rename		= nat_rename,
	.chdir		= nat_chdir,
	.mkdir		= nat_mkdir,
	.rmdir		= nat_rmdir,
	.statfs		= nat_statfs,
	.sync		= nat_sync,
};

void nat_init()
{
	if (fs_install(&fstype))
		panic("natfs_init: fs_install failed");
};

int nat_chk_perm(struct fso *fso, int mode, uid_t uid, gid_t gid)
{
	return 0;
}

int nat_sync(struct fs *fs)
{
	return nat_sync_bam(fs);
}

int nat_ioctl(struct fso *fso, int cmd, void *buf)
{
	blk_t blk;
	int err;
	
	if (cmd != FIOCBMAP)
		return ENOTTY;
	
	err = fs_chk_perm(fso, R_OK, curr->euid, curr->egid);
	if (err)
		return err;
	
	err = fucpy(&blk, buf, sizeof blk);
	if (err)
		return err;
	
	err = nat_bmap(fso, blk, 0);
	if (err)
		return err;
	
	blk = fso->nat.bmap_phys;
	return tucpy(buf, &blk, sizeof blk);
}

static int popcnt(uint32_t v)
{
	v = (v & 0x55555555) + ((v & 0xaaaaaaaa) >> 1);
	v = (v & 0x33333333) + ((v & 0xcccccccc) >> 2);
	v = (v & 0x0f0f0f0f) + ((v & 0xf0f0f0f0) >> 4);
	v = (v & 0x00ff00ff) + ((v & 0xff00ff00) >> 8);
	v = (v & 0x0000ffff) + ((v & 0xffff0000) >> 16);
	return v;
}

int nat_statfs(struct fs *fs, struct statfs *st)
{
	uint32_t *ebam = (void *)(fs->nat.bam_buf + BLK_SIZE);
	uint32_t *bamp;
	blk_t i;
	int err;
	
	st->blk_total = fs->nat.data_size;
	st->blk_free  = 0;
	
	for (i = 0; i < fs->nat.bam_size; i++)
	{
		err = nat_switch_bam(fs, fs->nat.bam_block + i);
		if (err)
			return err;
		
		for (bamp = (void *)fs->nat.bam_buf; bamp < ebam; bamp++)
		{
			if (*bamp == 0xffffffff)
				continue;
			st->blk_free += popcnt(~*bamp);
		}
	}
	return 0;
}
