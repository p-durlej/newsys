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
#include <kern/errno.h>
#include <kern/clock.h>
#include <kern/block.h>
#include <kern/task.h>
#include <kern/lib.h>
#include <kern/fs.h>
#include <os386.h>

#define DEBUG	0

int fs_mount(const char *prefix, const char *bdev_name, const char *fstype, int flags)
{
	struct fs *fs;
	int err;
	int i;
	
	for (i = 0; i < FS_MAXFS; i++)
	{
		fs = &fs_fs[i];
		
		if (!fs->in_use)
			continue;
		
		if (!strcmp(fs->prefix, prefix))
			return EMOUNTED;
		if (fs->dev && !strcmp(fs->dev->name, bdev_name))
			return EMOUNTED;
	}
	
	for (i = 0; i < FS_MAXFS; i++)
	{
		fs = &fs_fs[i];
		
		if (!fs->in_use)
			break;
	}
	
	if (i >= FS_MAXFS)
		return ENOMEM;
	
	fs = &fs_fs[i];
	memset(fs, 0, sizeof *fs);
	
	strcpy(fs->prefix, prefix);
	fs->prefix_l = strlen(prefix);
	
	if (strcmp(bdev_name, "nodev"))
	{
		err = blk_find(&fs->dev, bdev_name);
		if (err)
			return err;
	}
	else
		fs->dev = NULL;
	
	err = fs_find(&fs->type, fstype);
	if (err)
		return err;
	
	if (fs->dev)
	{
		err = blk_open(fs->dev);
		if (err)
			return err;
	}
	
	if (flags & 1)
		fs->read_only = 1;
	if (flags & 2)
		fs->no_atime = 1;
	if (flags & 4)
		fs->removable = 1;
	if (flags & 8)
		fs->insecure = 1;
	fs->in_use = 1;
	
	if (fs->removable)
		return 0;
	
	err = fs->type->mount(fs);
	if (err)
	{
		fs->in_use = 0;
		if (fs->dev)
			blk_close(fs->dev);
		return err;
	}
	
	fs->active = 1;
	return 0;
}

int fs_umount(const char *prefix)
{
	struct fs *fs;
	int i, n;
	int err;
	
	for (i = 0; i < FS_MAXFS; i++)
	{
		fs = &fs_fs[i];
		if (fs->in_use && !strcmp(fs->prefix, prefix))
			break;
	}
	if (i >= FS_MAXFS)
		return ENOENT;
	
	for (n = 0; n < fs_fso_high; n++)
	{
		struct fso *fso = &fs_fso[n];
		
		if (fso->refcnt && fso->fs == fs)
		{
			const char *prefix = fs->prefix;
			if (!*prefix)
				prefix = "/";
			
			if (fs->dev != NULL)
				printk("mount.c: fs_umount: %s: (%s): file %i active, refcnt = %i\n", prefix, fs->dev->name, fso->index, fso->refcnt);
			else
				printk("mount.c: fs_umount: %s: file %i active, refcnt = %i\n", prefix, fso->index, fso->refcnt);
			
			return EBUSY;
		}
	}
	
	if (fs->active)
		err = fs->type->umount(fs);
	else
		err = 0;
	
	if (fs->dev)
	{
		if (!err)
			err = blk_close(fs->dev);
		else
			blk_close(fs->dev);
	}
	
	fs->in_use = 0;
	return err;
}

static void fs_sched_unmount(struct fs *fs)
{
	int s;
	
	if (!fs->removable)
		panic("fs_sched_unount: !fs->removable");
	
	s = intr_dis();
	fs->unmount_time = clock_uptime() + FS_UDELAY;
	intr_res(s);
}

int fs_activate(struct fs *fs)
{
	int err;
	int s;
	
	if (!fs->removable)
		return 0;
	
	if (fs->active)
	{
		fs_sched_unmount(fs);
		return 0;
	}
	
#if DEBUG
	printk("fs_activate: mount \"%s\"\n", fs->prefix);
#endif
	err = fs->type->mount(fs);
	if (err)
		return err;
	
	s = intr_dis();
	
	fs->active = 1;
	
	fs_sched_unmount(fs);
	
	intr_res(s);
	return 0;
}

static void fs_dumount(void *cx)
{
	struct fs *fs = cx;
	int i;
	
	if (!fs->active)
		return;
	
	for (i = 0; i < fs_fso_high; i++)
	{
		struct fso *fso = &fs_fso[i];
		
		if (fso->refcnt && fso->fs == fs)
		{
#if DEBUG
			printk("fs_dumount: busy \"%s\"\n", fs->prefix);
#endif
			fs_sched_unmount(fs);
			return;
		}
	}
	
#if DEBUG
	printk("fs_dumount: unmount \"%s\"\n", fs->prefix);
#endif
	fs->type->umount(fs);
	fs->active = 0;
	
	blk_syncdev(fs->dev, SYNC_WRITE | SYNC_INVALIDATE);
}

void fs_clock(void)
{
	time_t t;
	int i;
	
	t = clock_uptime();
	
	for (i = 0; i < FS_MAXFS; i++)
	{
		struct fs *fs = &fs_fs[i];
		
		if (!fs->in_use || !fs->removable || !fs->active)
			continue;
		
		if (!fs->unmount_time || fs->unmount_time > t)
			continue;
		
#if DEBUG
		printk("fs_clock: unmount \"%s\"\n", fs->prefix);
#endif
		fs->unmount_time = 0;
		task_defer(fs_dumount, fs);
	}
}
