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
#include <kern/natfs.h>
#include <kern/errno.h>
#include <kern/block.h>
#include <kern/task.h>
#include <kern/lib.h>
#include <kern/fs.h>
#include <sys/stat.h>

int nat_read(struct fs_rwreq *req)
{
	struct fso *fso = req->fso;
	char *bp = req->buf;
	uoff_t count;
	uoff_t off;
	int err;
	
	err = fs_chk_perm(fso, R_OK, curr->euid, curr->egid);
	if (err)
		return err;
	
	if (S_ISDIR(fso->mode))
		return EISDIR;
	
	if (!S_ISREG(fso->mode))
		return EINVAL;
	
	if (req->offset < 0)
		return EINVAL;
	if (req->count < 0)
		return EINVAL;
	if (req->offset + req->count < 0)
		return EINVAL;
	
	if (req->offset > fso->size)
	{
		req->count=0;
		return 0;
	}
	
	if (req->offset + req->count > fso->size)
		req->count=fso->size - req->offset;
	
	count = req->count;
	off   = req->offset;
	
	if (!fso->fs->read_only && !fso->fs->no_atime)
	{
		fso->atime = clock_time();
		fso->dirty = 1;
	}
	
	while (count)
	{
		struct block *b;
		unsigned l;
		unsigned s;
		
		s = off % BLK_SIZE;
		
		if (count > BLK_SIZE - s)
			l = BLK_SIZE - s;
		else
			l = count;
		
		err = nat_bmap(fso, off / BLK_SIZE, 0);
		if (err)
			return err;
		
		if (!fso->nat.bmap_phys)
			memset(bp, 0, l);
		else
		{
			err = blk_read(&b, fso->fs->dev, fso->nat.bmap_phys);
			if (err)
				return err;
			
			memcpy(bp, b->data + s, l);
			
			blk_put(b);
			if (err)
				return err;
		}
		
		count -= l;
		off   += l;
		bp    += l;
	}
	
	return 0;
}

int nat_write(struct fs_rwreq *req)
{
	struct fso *fso = req->fso;
	char *bp = req->buf;
	uoff_t count;
	uoff_t off;
	int err;
	
	err = fs_chk_perm(fso, W_OK, curr->euid, curr->egid);
	if (err)
		return err;
	
	if (S_ISDIR(fso->mode))
		return EISDIR;
	
	if (!S_ISREG(fso->mode))
		return EINVAL;
	
	if (req->offset < 0)
		return EINVAL;
	if (req->count < 0)
		return EINVAL;
	if (req->offset + req->count < 0)
		return EINVAL;
	if (req->offset + req->count >= NAT_FILE_SIZE_MAX)
		return EINVAL;
	
	count = req->count;
	off   = req->offset;
	
	fso->mtime = clock_time();
	fso->dirty = 1;
	
	while (count)
	{
		struct block *b;
		unsigned l;
		unsigned s;
		
		s = off % BLK_SIZE;
		
		if (count > BLK_SIZE - s)
			l = BLK_SIZE - s;
		else
			l = count;
		
		err = nat_bmap(fso, off / BLK_SIZE, 1);
		if (err)
			return err;
		
		if (off + l > fso->size)
		{
			fso->size = off + l;
			fso->dirty = 1;
		}
		
		if (l != BLK_SIZE)
			err = blk_read(&b, fso->fs->dev, fso->nat.bmap_phys);
		else
			err = blk_get(&b, fso->fs->dev, fso->nat.bmap_phys);
		if (err)
			return err;
		
		memcpy(b->data + s, bp, l);
		
		b->valid = 1;
		b->dirty = 1;
		blk_put(b);
		if (err)
			return err;
		
		count -= l;
		off   += l;
		bp    += l;
	}
	
	return 0;
}
