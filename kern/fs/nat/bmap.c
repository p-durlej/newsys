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
#include <kern/sched.h>
#include <kern/umem.h>
#include <kern/natfs.h>
#include <kern/errno.h>
#include <kern/lib.h>
#include <sys/stat.h>

#define BLK_PER_BLK	(BLK_SIZE / sizeof(blk_t))

int nat_switch_bam(struct fs *fs, blk_t blk)
{
	int err;
	
	if (blk == fs->nat.bam_curr)
		return 0;
	
	nat_sync_bam(fs);
	err = blk_pread(fs->dev, blk, 0, BLK_SIZE, fs->nat.bam_buf);
	if (err)
	{
		fs->nat.bam_curr = 0;
		return err;
	}
	fs->nat.bam_curr = blk;
	return 0;
}

int nat_read_bam(struct fs *fs, blk_t blk, int *bused)
{
	int mask, i;
	int err;
	
	err = nat_switch_bam(fs, fs->nat.bam_block + blk / BLK_SIZE / 8);
	if (err)
		return err;
	
	mask = (1 << (blk & 7));
	i    = (blk / 8) % BLK_SIZE;
	
	*bused = !!(fs->nat.bam_buf[i] & mask);
	return 0;
}

int nat_write_bam(struct fs *fs, blk_t blk, int bused)
{
	int mask, val, i;
	int err;
	
	err = nat_switch_bam(fs, fs->nat.bam_block + blk / BLK_SIZE / 8);
	if (err)
		return err;
	
	mask =   ~(1 << (blk & 7));
	val  = bused << (blk & 7);
	i    = (blk / 8) % BLK_SIZE;
	
	fs->nat.bam_buf[i] &= mask;
	fs->nat.bam_buf[i] |= val;
	fs->nat.bam_dirty = 1;
	return 0;
}

int nat_sync_bam(struct fs *fs)
{
	if (!fs->nat.bam_dirty)
		return 0;
	fs->nat.bam_dirty = 0;
	
	return blk_pwrite(fs->dev, fs->nat.bam_curr, 0, BLK_SIZE, fs->nat.bam_buf);
}

int nat_balloc(struct fs *fs, blk_t *blk)
{
	uint32_t *bamp, *ebam = (void *)(fs->nat.bam_buf + BLK_SIZE);
	uint32_t bamw;
	blk_t bn;
	blk_t i;
	int err;
	
	i = fs->nat.data_block / BLK_SIZE / 8;
	for (; i < fs->nat.bam_size; i++)
	{
		err = nat_switch_bam(fs, fs->nat.bam_block + i);
		if (err)
			return err;
		
		for (bamp = (void *)fs->nat.bam_buf; bamp < ebam; bamp++)
		{
			if (*bamp == 0xffffffff)
				continue;
			bamw = *bamp;
			break;
		}
		if (bamp == ebam)
			continue;
		
		bn = 0;
		while (bamw & 1)
		{
			bamw >>= 1;
			bn++;
		}
		*bamp |= 1 << (bn & 31);
		fs->nat.bam_dirty = 1;
		
		bn += ((char *)bamp - fs->nat.bam_buf) * 8;
		bn += i * BLK_SIZE * 8;
		
		*blk = bn;
		return 0;
	}
	
	printk("nat_balloc: no space left on device %s\n", fs->dev->name);
	return ENOSPC;
}

int nat_bfree(struct fs *fs, blk_t blk)
{
	if (fs->nat.free_block > blk)
		fs->nat.free_block = blk;
	return nat_write_bam(fs, blk, 0);
}

int nat_bmap_dir(struct fso *fso, blk_t log, int alloc, blk_t *blk)
{
	struct block *bb;
	blk_t bn;
	int err;
	
	if (log >= sizeof fso->nat.bmap / sizeof *fso->nat.bmap)
		return EFBIG;
	if (alloc && !fso->nat.bmap[log])
	{
		err = nat_balloc(fso->fs, &bn);
		if (err)
			return err;
		
		err = blk_get(&bb, fso->fs->dev, bn);
		if (err)
			return err;
		
		memset(bb->data, 0, BLK_SIZE);
		bb->dirty = 1;
		bb->valid = 1;
		blk_put(bb);
		
		fso->nat.bmap[log] = bn;
		fso->blocks++;
		fso->dirty	   = 1;
	}
	*blk = fso->nat.bmap[log];
	return 0;
}

int nat_bmap(struct fso *fso, blk_t log, int alloc)
{
	struct block *bb = NULL, *bb1 = NULL;
	uint32_t *imap;
	blk_t bn;
	int shift;
	int indl;
	int err;
	
	if (fso->nat.bmap_valid && fso->nat.bmap_log == log)
		return 0;
	
	if (log < fso->nat.ndirblks)
	{
		err = nat_bmap_dir(fso, log, alloc, &bn);
		if (err)
			return err;
		
		fso->nat.bmap_phys  = bn;
		fso->nat.bmap_log   = log;
		fso->nat.bmap_valid = 1;
		return 0;
	}
	
	indl  = fso->nat.nindirlev;
	shift = 7 * indl;
	
	err = nat_bmap_dir(fso, fso->nat.ndirblks + (log >> shift), alloc, &bn);
	if (err)
		return err;
	shift -= 7;
	
	while (bn && indl)
	{
		err = blk_read(&bb, fso->fs->dev, bn);
		if (err)
			return err;
		imap = (void *)bb->data;
		
		bn = imap[(log >> shift) & 127];
		if (!bn)
		{
			if (!alloc)
				break;
			
			err = nat_balloc(fso->fs, &bn);
			if (err)
				goto err;
			
			err = blk_get(&bb1, fso->fs->dev, bn);
			if (err)
				goto err;
			memset(bb1->data, 0, BLK_SIZE);
			bb1->valid = 1;
			bb1->dirty = 1;
			blk_put(bb1);
			bb1 = NULL;
			
			imap[(log >> shift) & 127] = bn;
			bb->dirty = 1;
			fso->blocks++;
			fso->dirty = 1;
		}
		blk_put(bb);
		bb = NULL;
		shift -= 7;
		indl--;
	}
	
	fso->nat.bmap_phys  = bn;
	fso->nat.bmap_log   = log;
	fso->nat.bmap_valid = 1;
err:
	if (bb1 != NULL)
		blk_put(bb1);
	if (bb != NULL)
		blk_put(bb);
	return err;
}

static int nat_trunc_indir(struct fs *fs, blk_t bn, int indl)
{
	struct block *bb;
	uint32_t *imap;
	int err1;
	int err;
	int i;
	
	err = blk_read(&bb, fs->dev, bn);
	if (err)
		return err;
	imap = (void *)bb->data;
	
	for (i = 0; i < BLK_SIZE / sizeof *imap; i++)
		if (imap[i])
		{
			if (indl)
			{
				err1 = nat_trunc_indir(fs, imap[i], indl - 1);
				if (!err)
					err = err1;
			}
			
			err1 = nat_bfree(fs, imap[i]);
			if (!err)
				err = err1;
		}
	
	blk_put(bb);
	return err;
}

int nat_trunc(struct fso *fso)
{
	blk_t bn;
	int indl;
	int i;
	
	indl = fso->nat.nindirlev;
	if (indl < 1 || indl > 8)
		return EINVAL;
	
	for (i = 0; i < sizeof fso->nat.bmap / sizeof *fso->nat.bmap; i++)
	{
		bn = fso->nat.bmap[i];
		if (!bn)
			continue;
		
		if (i >= fso->nat.ndirblks)
			nat_trunc_indir(fso->fs, bn, indl - 1);
		nat_bfree(fso->fs, bn);
		
		fso->nat.bmap[i] = 0;
	}
	fso->dirty  = 1;
	fso->blocks = 1;
	fso->size   = 0;
	return 0;
}
