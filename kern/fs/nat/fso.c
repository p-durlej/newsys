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

#include <kern/errno.h>
#include <kern/natfs.h>
#include <kern/stat.h>
#include <kern/lib.h>
#include <kern/fs.h>

static mode_t nat_imode(mode_t mode)
{
	mode_t n;
	
	n  = mode & S_IFMT;
	n |= 0444;
	
	if (mode & 0222)
		n |= 0222;
	if (mode & 0111)
		n |= 0111;
	
	return n;
}

int nat_putfso(struct fso *fso)
{
	if (!fso->nlink)
	{
		nat_trunc(fso);
		nat_bfree(fso->fs, fso->index);
		return 0;
	}
	else
		return nat_syncfso(fso);
}

int nat_getfso(struct fso *fso)
{
	struct nat_header *hd;
	struct block *b;
	int err;
	
	err = blk_read(&b, fso->fs->dev, fso->index);
	if (err)
		return err;
	hd = (void *)b->data;
	
	fso->blocks	= hd->blocks;
	fso->size	= hd->size;
	fso->mode	= hd->mode;
	fso->rdev	= hd->rdev;
	
	fso->uid	= hd->uid;
	fso->gid	= hd->gid;
	
	fso->atime	= hd->atime;
	fso->ctime	= hd->ctime;
	fso->mtime	= hd->mtime;
	
	fso->nlink	= hd->nlink;
	
	if (fso->fs->insecure)
	{
		fso->mode = nat_imode(fso->mode);
		fso->uid  = 0;
		fso->gid  = 0;
	}
	
	memcpy(&fso->nat.bmap, hd->bmap, sizeof fso->nat.bmap);
	fso->nat.ndirblks  = hd->ndirblks;
	fso->nat.nindirlev = hd->nindirlev;
	
	fs_state(fso, R_OK | W_OK);
	blk_put(b);
	return 0;
}

int nat_syncfso(struct fso *fso)
{
	struct nat_header *hd;
	struct block *b;
	int err;
	
	if (!fso->dirty)
		return 0;
	
	err = blk_get(&b, fso->fs->dev, fso->index);
	if (err)
		return err;
	
	memset(b->data, 0, BLK_SIZE);
	hd = (void *)b->data;
	
	hd->blocks	= fso->blocks;
	hd->size	= fso->size;
	hd->mode	= fso->mode;
	
	hd->uid		= fso->uid;
	hd->gid		= fso->gid;
	
	hd->atime	= fso->atime;
	hd->ctime	= fso->ctime;
	hd->mtime	= fso->mtime;
	
	hd->nlink	= fso->nlink;
	
	if (fso->fs->insecure)
	{
		hd->mode = nat_imode(hd->mode);
		hd->uid  = 0;
		hd->gid  = 0;
	}
	
	memcpy(hd->bmap, &fso->nat.bmap, sizeof fso->nat.bmap);
	hd->ndirblks  = fso->nat.ndirblks;
	hd->nindirlev = fso->nat.nindirlev;
	
	fso->dirty	= 0;
	b->valid	= 1;
	b->dirty	= 1;
	blk_put(b);
	return 0;
}
