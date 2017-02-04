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

#ifndef _KERN_NATFS_H
#define _KERN_NATFS_H

#include <priv/natfs.h>
#include <sys/types.h>
#include <kern/fs.h>

void nat_init(void);

int nat_mount(struct fs *fs);
int nat_umount(struct fs *fs);

int nat_lookup(struct fso **f, struct fs *fs, const char *pathname);
int nat_creat(struct fso **f, struct fs *fs, const char *pathname, mode_t mode, dev_t rdev);
int nat_getfso(struct fso *f);
int nat_putfso(struct fso *f);
int nat_syncfso(struct fso *f);

int nat_readdir(struct fso *dir, struct fs_dirent *de, int index);

int nat_ioctl(struct fso *fso, int cmd, void *buf);
int nat_k_read(struct fs_rwreq *req);
int nat_k_write(struct fs_rwreq *req);
int nat_read(struct fs_rwreq *req);
int nat_write(struct fs_rwreq *req);
int nat_trunc(struct fso *fso);
int nat_poll(struct fso *f);

int nat_chk_perm(struct fso *f, int mode, uid_t uid, gid_t gid);
int nat_link(struct fso *f, const char *name);
int nat_unlink(struct fs *fs, const char *name);
int nat_rename(struct fs *fs, const char *oldname, const char *newname);

int nat_statfs(struct fs *fs, struct statfs *st);
int nat_sync(struct fs *fs);

int nat_switch_bam(struct fs *fs, blk_t blk);
int nat_read_bam(struct fs *fs, blk_t blk, int *bused);
int nat_write_bam(struct fs *fs, blk_t blk, int bused);
int nat_sync_bam(struct fs *fs);

int nat_balloc(struct fs *fs, blk_t *blk);
int nat_bfree(struct fs *fs, blk_t blk);
int nat_bmap(struct fso *fso, blk_t log, int alloc);

int nat_chdir(struct fs *fs, const char *name);
int nat_mkdir(struct fs *fs, const char *name, mode_t mode);
int nat_rmdir(struct fs *fs, const char *name);

#endif
