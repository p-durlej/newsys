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

#ifndef _KERN_BLOCK_H
#define _KERN_BLOCK_H

#include <sys/types.h>
#include <list.h>

#define BLK_RQLEN	1024
#define BLK_READ	1
#define BLK_WRITE	2

#define BLK_MAXDEV	32
#define BLK_NBLK	1024

#define BLK_SIZE	512

#define BLK_MAGIC	0xd15cb10c /* disk block */

struct bdev
{
	int	refcnt;
	char *	name;
	
	void *	data;
	int	unit;
	
	int	(*open)(int unit);
	int	(*close)(int unit);
	int	(*ioctl)(int unit, int cmd, void *buf);
	int	(*read)(int unit, blk_t blk, void *buf);
	int	(*write)(int unit, blk_t blk, const void *buf);
	
	uint64_t	read_cnt;
	uint64_t	write_cnt;
	uint64_t	error_cnt;
};

struct block
{
	struct list_item queue_item;
	struct list_item list_item;
	struct list_item lru_item;
	
	struct task *	reqowner;
	int		reqtype;
	int		err;
	
	int		refcnt;
	int		valid;
	int		dirty;
	
	struct bdev *	dev;
	blk_t		nr;
	
	char		data[BLK_SIZE];
};

struct blk_stat
{
	int dirty;
	int valid;
	int total;
	int free;
};

extern struct bdev *blk_dev[BLK_MAXDEV];

void blk_init(void);
void blk_dump(void);

int blk_find(struct bdev **dev, const char *name);
int blk_install(struct bdev *dev);
int blk_uinstall(struct bdev *dev);

int blk_open(struct bdev *dev);
int blk_close(struct bdev *dev);

int blk_get(struct block **blkp, struct bdev *dev, blk_t nr);
int blk_read(struct block **blkp, struct bdev *dev, blk_t nr);
int blk_put(struct block *blk);

int blk_pread(struct bdev *dev, blk_t nr, unsigned off, unsigned len, void *buf);
int blk_pwrite(struct bdev *dev, blk_t nr, unsigned off, unsigned len, void *buf);

int blk_write(struct block *blk);

int blk_syncdev(struct bdev *dev, int flags);
int blk_syncall(int flags);

void blk_stat(struct blk_stat *buf);
int blk_add(int count);

#endif
