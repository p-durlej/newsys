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

#include <kern/config.h>
#include <kern/printk.h>
#include <kern/fs.h>
#include <kern/lib.h>
#include <kern/block.h>
#include <kern/natfs.h>
#include <kern/errno.h>

int nat_mount(struct fs *fs)
{
	struct nat_super *sb;
	struct block *sbb;
	int err;
	
	err = blk_read(&sbb, fs->dev, 1);
	if (err)
	{
#if VERBOSE
		perror("nat_mount: blk_read", err);
#endif
		return err;
	}
	
	sb = (void *)sbb->data;
	if (memcmp(sb->magic, NAT_MAGIC, sizeof sb->magic))
	{
#if VERBOSE
		printk("mount.c: nat_mount: bad magic\n");
#endif
		blk_put(sbb);
		return EINVAL;
	}
	
#if VERBOSE
	printk("nat_mount: sb->bam_block  = %i\n", sb->bam_block);
	printk("           sb->bam_size   = %i\n", sb->bam_size);
	printk("           sb->data_block = %i\n", sb->data_block);
	printk("           sb->data_size  = %i\n", sb->data_size);
	printk("           sb->root_block = %i\n", sb->root_block);
	printk("           sb->ndirblks   = %i\n", sb->ndirblks);
	printk("           sb->nindirlev  = %i\n", sb->nindirlev);
#endif
	
	fs->nat.bam_block  = sb->bam_block;
	fs->nat.bam_size   = sb->bam_size;
	fs->nat.data_block = sb->data_block;
	fs->nat.data_size  = sb->data_size;
	fs->nat.root_block = sb->root_block;
	
	fs->nat.free_block = sb->data_block;
	fs->nat.bam_curr   = 0;
	fs->nat.bam_dirty  = 0;
	
	fs->nat.ndirblks   = sb->ndirblks;
	fs->nat.nindirlev  = sb->nindirlev;
	
	blk_put(sbb);
	return 0;
}

int nat_umount(struct fs *fs)
{
	nat_sync_bam(fs);
	return 0;
}
