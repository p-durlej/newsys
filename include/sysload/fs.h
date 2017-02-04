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

#ifndef _SYSLOAD_FS_H
#define _SYSLOAD_FS_H

#include <priv/natfs.h>

#include <sys/types.h>

struct file;
struct fs;

struct fstype
{
	struct fstype *next;
	const char *name;
	
	int (*mount)(struct fs *fs);
	int (*lookup)(struct file *f, struct fs *fs, const char *pathname);
	int (*load)(struct file *f, void *buf, size_t sz);
};

struct fs
{
	struct fstype *	fstype;
	struct disk *	disk;
	
	struct nat_super sb;
};

struct file
{
	struct fs *	fs;
	
	blk_t		index;
	off_t		size;
};

extern struct fstype *fs_types;

struct fstype *fs_find(const char *name);

int fs_mount(struct fs *fs, struct fstype *fst, struct disk *disk);
int fs_lookup(struct file *f, struct fs *fs, const char *pathname);
int fs_load3(struct file *f, void *buf, size_t sz);
int fs_load(struct file *f, void *buf);

void fs_init(void);

#endif
