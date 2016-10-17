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

#include <kern/task_queue.h>
#include <kern/task.h>
#include <kern/printk.h>
#include <kern/config.h>
#include <kern/errno.h>
#include <kern/block.h>
#include <kern/umem.h>
#include <kern/lib.h>
#include <os386.h>
#include <list.h>

#define BLK_NR_LISTS	64

#define SYNC_PWRITE	0

struct bdev *blk_dev[BLK_MAXDEV];

static struct list blk_lists[BLK_NR_LISTS];
static struct list blk_lru;

static struct block **blk_blk;
static int blk_count;

void blk_stat(struct blk_stat *buf)
{
	int i;
	
	buf->total = blk_count;
	buf->dirty = 0;
	buf->valid = 0;
	buf->free  = 0;
	
	for (i = 0; i < blk_count; i++)
	{
		if (blk_blk[i]->dirty)
			buf->dirty++;
		if (blk_blk[i]->valid)
			buf->valid++;
		if (!blk_blk[i]->refcnt)
			buf->free++;
	}
}

#if BLK_CHECK
static void blk_check(void)
{
	struct block *b;
	struct list *l;
	int i;
	
	for (i = 0; i < BLK_NR_LISTS; i++)
	{
		l = &blk_lists[i];
		for (b = list_first(l); b; b = list_next(l, b))
			if (i != b->nr % BLK_NR_LISTS)
				panic("blk_check: i != b->nr % BLK_NR_LISTS");
	}
}
#else
#define blk_check()
#endif

void blk_init(void)
{
	struct block *p;
	int err;
	int i;
	
	err = kmalloc(&blk_blk, sizeof(struct block *) * BLK_NBLK, "blk_blk");
	if (err)
		panic("blk_init: could not allocate blk_blk");
	
	err = kmalloc(&p, sizeof(struct block) * BLK_NBLK, "block");
	if (err)
		panic("blk_init: could not allocate memory for disk cache");
	memset(p, 0, sizeof(struct block) * BLK_NBLK);
	
	for (i = 0; i < BLK_NR_LISTS; i++)
		list_init(&blk_lists[i], struct block, list_item);
	list_init(&blk_lru, struct block, lru_item);
	
	for (i = 0; i < BLK_NBLK; i++, p++)
	{
		list_app(&blk_lists[0], p); /* XXX */
		list_app(&blk_lru, p);
		blk_blk[i] = p;
	}
	blk_count = BLK_NBLK;
	blk_check();
}

void blk_dump(void)
{
	struct block *b;
	int i;
	
	for (i = 0; i < blk_count; i++)
	{
		b = blk_blk[i];
		
		if (b->refcnt)
			printk("blk_dump: %i: valid = %i, dirty = %i, refcnt = %i, nr = %li\n",
				i, b->valid, b->dirty, b->refcnt, (long)b->nr);
	}
}

int blk_install(struct bdev *dev)
{
	int i;
	
	for (i = 0; i < BLK_MAXDEV; i++)
		if (blk_dev[i] && !strcmp(blk_dev[i]->name, dev->name))
			return EEXIST;
	
	for (i = 0; i < BLK_MAXDEV; i++)
		if (!blk_dev[i])
		{
			dev->write_cnt = dev->read_cnt = dev->error_cnt = 0;
			dev->refcnt = 0;
			
			blk_dev[i] = dev;
			return 0;
		}
	
	return ENOMEM;
}

int blk_uinstall(struct bdev *dev)
{
	int i;
	
	if (dev->refcnt)
		panic("blk_uinstall: device in use");
	
	for (i = 0; i < BLK_MAXDEV; i++)
		if (blk_dev[i] == dev)
		{
			blk_dev[i] = NULL;
			return 0;
		}
	
	panic("blk_uinstall: device not installed");
}

int blk_find(struct bdev **dev, const char *name)
{
	int i;
	
	for (i = 0; i < BLK_MAXDEV; i++)
		if (blk_dev[i] && !strcmp(blk_dev[i]->name, name))
		{
			*dev = blk_dev[i];
			return 0;
		}
	
	return ENODEV;
}

int blk_get(struct block **blkp, struct bdev *dev, blk_t nr)
{
	struct block *b;
	struct list *l;
	int loop_det;
	
	blk_check();
	
	loop_det = blk_count;
	l = &blk_lists[nr % BLK_NR_LISTS];
	for (b = list_first(l); b; b = list_next(l, b))
	{
		if (!loop_det--)
			panic("blk_get: queue loop\n");
		
		if ((b->refcnt || b->valid) && b->nr == nr && b->dev == dev)
		{
			if (!b->refcnt)
				list_rm(&blk_lru, b);
			
			b->refcnt++;
			*blkp = b;
			blk_check();
			return 0;
		}
	}
	
	b = list_first(&blk_lru);
	if (!b)
	{
		printk("blk_get: out of disk buffers\n");
		return ENOMEM;
	}
	list_rm(&blk_lists[b->nr % BLK_NR_LISTS], b);
	list_rm(&blk_lru, b);
	
	if (b->dirty)
		blk_write(b);
	
	b->refcnt = 1;
	b->dev	  = dev;
	b->nr	  = nr;
	b->valid  = 0;
	
	list_pre(l, b);
	
	*blkp = b;
	blk_check();
	return 0;
}

int blk_read(struct block **blkp, struct bdev *dev, blk_t nr)
{
	struct block *b = NULL;
	int err;
	
	err = blk_get(&b, dev, nr);
	if (err)
		return err;
	
	if (b->valid)
		goto fini;
	
	err = dev->read(dev->unit, b->nr, b->data);
	if (err)
	{
		dev->error_cnt++;
		blk_put(b);
		return err;
	}
	
	dev->read_cnt++;
	b->valid = 1;
fini:
	*blkp = b;
	return 0;
}

int blk_put(struct block *blk)
{
	if (!blk)
		return 0;
	
	blk_check();
	
	if (!blk->refcnt)
		panic("blk_put: !blk->refcnt");
	blk->refcnt--;
	if (!blk->refcnt)
		list_app(&blk_lru, blk);
	blk_check();
	return 0;
}

int blk_write(struct block *b)
{
	int err = 0;
	
	if (!b->valid)
		panic("blk_write: !b->valid");
	
	if (!b->dirty)
		return 0;
	
	b->dirty = 0;
	
	err = b->dev->write(b->dev->unit, b->nr, b->data);
	
	if (err)
		b->dev->error_cnt++;
	else
		b->dev->write_cnt++;
	
	return err;
}

int blk_open(struct bdev *dev)
{
	int err;
	
	if (!dev->refcnt)
	{
		err = dev->open(dev->unit);
		if (err)
			return err;
	}
	
	dev->refcnt++;
	return 0;
}

int blk_close(struct bdev *dev)
{
	int err;
	
	if (!dev->refcnt)
		panic("blk_close: device not open");
	
	if (dev->refcnt == 1)
	{
		blk_syncdev(dev, SYNC_WRITE | SYNC_INVALIDATE);
		err = dev->close(dev->unit);
		if (err)
			return err;
	}
	
	dev->refcnt--;
	return 0;
}

int blk_pread(struct bdev *dev, blk_t nr, unsigned off, unsigned len, void *buf)
{
	struct block *b;
	int err;
	
	err = blk_read(&b, dev, nr);
	if (err)
		return err;
	
	memcpy(buf, b->data + off, len);
	
	return blk_put(b);
}

int blk_pwrite(struct bdev *dev, blk_t nr, unsigned off, unsigned len, void *buf)
{
	struct block *b;
	int err;
	
	if (len != BLK_SIZE)
		err = blk_read(&b, dev, nr);
	else
		err = blk_get(&b, dev, nr);
	
	if (err)
		return err;
	
	memcpy(b->data + off, buf, len);
	b->dirty = 1;
	b->valid = 1;
	
#if SYNC_PWRITE
	err = blk_write(b);
	blk_put(b);
	return err;
#else
	blk_put(b);
	return 0;
#endif
}

int blk_syncdev(struct bdev *dev, int flags)
{
	struct block *b;
	
	for (b = list_first(&blk_lru); b; b = list_next(&blk_lru, b)) /* XXX */
		if (b->dev == dev)
		{
			if ((flags & SYNC_WRITE) && b->dirty)
				blk_write(b);
			if ((flags & SYNC_INVALIDATE) && !b->dirty)
				b->valid = 0;
		}
	return 0;
}

int blk_syncall(int flags)
{
	struct block *b;
	
	for (b = list_first(&blk_lru); b; b = list_next(&blk_lru, b)) /* XXX */
	{
		if ((flags & SYNC_WRITE) && b->dirty)
			blk_write(b);
		if ((flags & SYNC_INVALIDATE) && !b->dirty)
			b->valid = 0;
	}
	return 0;
}

int blk_add(int count)
{
	struct block *b;
	void *p;
	int err;
	int i;
	
	if (count < blk_count)
		return EINVAL;
	
	if (count == blk_count)
		return 0;
	
	err = krealloc(&p, blk_blk, sizeof *blk_blk * count, "blk_blk");
	if (err)
		return err;
	blk_blk = p;
	
	err = kmalloc(&b, sizeof *b * (count - blk_count), "block");
	if (err)
		return err;
	memset(b, 0, sizeof *b * (count - blk_count));
	
	for (i = blk_count; i < count; i++, b++)
	{
		list_pre(&blk_lru, b);
		blk_blk[i] = b;
	}
	blk_count = count;
	
#if KVERBOSE
	printk("blk_add: blk_count is now %i\n", blk_count);
#endif
	blk_check();
	return 0;
}
