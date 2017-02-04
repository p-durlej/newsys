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

#include <sysload/fs.h>

#include <string.h>
#include <errno.h>

struct fstype *fs_types;

void fs_native_init(void);
void fs_boot_init(void);

struct fstype *fs_find(const char *name)
{
	struct fstype *p;
	
	for (p = fs_types; p; p = p->next)
		if (!strcmp(p->name, name))
			return p;
	return NULL;
}

int fs_mount(struct fs *fs, struct fstype *fst, struct disk *disk)
{
	memset(fs, 0, sizeof *fs);
	fs->disk   = disk;
	fs->fstype = fst;
	return fst->mount(fs);
}

int fs_lookup(struct file *f, struct fs *fs, const char *pathname)
{
	while (*pathname == '/')
		pathname++;
	memset(f, 0, sizeof *f);
	f->fs = fs;
	return fs->fstype->lookup(f, fs, pathname);
}

int fs_load3(struct file *f, void *buf, size_t sz)
{
	if (sz > f->size)
		return EINVAL;
	return f->fs->fstype->load(f, buf, sz);
}

int fs_load(struct file *f, void *buf)
{
	return fs_load3(f, buf, f->size);
}

void fs_init(void)
{
	fs_native_init();
	fs_boot_init();
}
