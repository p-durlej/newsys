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

#ifndef _KERN_FS_H
#define _KERN_FS_H

#include <kern/task_queue.h>
#include <kern/limits.h>
#include <kern/block.h>
#include <kern/mutex.h>
#include <sys/types.h>
#include <stddef.h>
#include <statfs.h>

#define FS_MAXFSTYPE	8
#define FS_MAXFS	16
#define FS_MAXFSO	256
#define FS_MAXFILE	256

#define R_OK		4
#define W_OK		2
#define X_OK		1
#define F_OK		0

struct task;

struct fs_file
{
	int		refcnt;
	struct fso *	fso;
	uoff_t		offset;
	int		omode;
};

struct fs_desc
{
	struct fs_file *file;
	int		close_on_exec;
};

struct fs_rwreq
{
	void *		buf;
	uoff_t		offset;
	size_t		count;
	struct fso *	fso;
	int		no_delay;
};

struct fso
{
	int			no_seek;
	int			refcnt;
	int			dirty;
	
	struct fs *		fs;
	ino_t			index;
	
	nlink_t			nlink;
	
	uid_t			uid;
	gid_t			gid;
	
	time_t			atime;
	time_t			ctime;
	time_t			mtime;
	
	blkcnt_t		blocks;
	mode_t			mode;
	size_t			size;
	dev_t			rdev;
	
	int			reader_count;
	int			writer_count;
	
	struct task *volatile	polling;
	volatile int		pollcnt;
	volatile int		state;
	
	void *			extra;
	
	union
	{
		struct
		{
			int	bmap_valid;
			blk_t	bmap_log;
			blk_t	bmap_phys;
			blk_t	bmap[114];
			int	ndirblks;
			int	nindirlev;
		} nat;
		
		struct
		{
			struct task_queue readers;
			struct task_queue writers;
			int		  write_p;
			int		  read_p;
			char *		  buf;
		} pipe;
	};
};

struct fs_dirent
{
	ino_t	index;
	char	name[NAME_MAX + 1];
};

struct fs
{
	char		prefix[PATH_MAX];
	int		prefix_l;
	
	int		removable;
	int		read_only;
	int		no_atime;
	int		insecure;
	int		active;
	int		in_use;
	
	time_t		unmount_time;
	
	struct fstype *	type;
	struct bdev *	dev;
	
	void *		extra;
	
	union
	{
		struct
		{
			blk_t	bam_block;
			blk_t	bam_size;
			blk_t	data_block;
			blk_t	data_size;
			blk_t	root_block;
			blk_t	free_block;
			
			char	bam_buf[BLK_SIZE];
			blk_t	bam_curr;
			int	bam_dirty;
			int	bam_busy;
			
			int	ndirblks;
			int	nindirlev;
		} nat;
		
		struct
		{
			time_t	mount_time;
		} bfs;
	};
};

struct fstype
{
	const char *name;
	
	int (*mount)(struct fs *fs);
	int (*umount)(struct fs *fs);
	
	int (*lookup)(struct fso **f, struct fs *fs, const char *name);
	int (*creat)(struct fso **f, struct fs *fs, const char *name, mode_t mode, dev_t rdev);
	
	int (*getfso)(struct fso *f);
	int (*putfso)(struct fso *f);
	int (*syncfso)(struct fso *f);
	
	int (*readdir)(struct fso *dir, struct fs_dirent *de, int index);
	
	int (*ioctl)(struct fso *fso, int cmd, void *buf);
	int (*read)(struct fs_rwreq *req);
	int (*write)(struct fs_rwreq *req);
	int (*trunc)(struct fso *fso);
	
	int (*chk_perm)(struct fso *f, int mode, uid_t uid, gid_t gid);
	int (*link)(struct fso *f, const char *name);
	int (*unlink)(struct fs *fs, const char *name);
	int (*rename)(struct fs *fs, const char *oldname, const char *newname);
	
	int (*chdir)(struct fs *fs, const char *name);
	int (*mkdir)(struct fs *fs, const char *name, mode_t mode);
	int (*rmdir)(struct fs *fs, const char *name);
	
	int (*statfs)(struct fs *fs, struct statfs *st);
	int (*sync)(struct fs *fs);
	
	int (*ttyname)(struct fso *f, char *buf, int buf_size);
};

extern struct fs	fs_fs[FS_MAXFS];
extern struct fstype *	fs_fstype[FS_MAXFSTYPE];
extern struct fso *	fs_fso;
extern int		fs_fso_high;

extern struct fs_file	fs_file[FS_MAXFILE];

extern struct task_queue fs_pollq;

void fs_init(void);

void fs_clock(void);

void fs_newtask(struct task *p);
void fs_clean(void);

int  fs_fqname(char *buf, const char *path);
int  fs_pathfs(struct fs **fs, int *prefix_l, const char *pathname);
int  fs_parsename(struct fs **fs, char *buf, const char *pathname);
int  fs_parsename4(struct fs **fs, char *buf, char *fqbuf, const char *pathname);

int  fs_find(struct fstype **fstype, const char *name);
int  fs_install(struct fstype *fstype);
int  fs_uinstall(struct fstype *fstype);

int  fs_mount(const char *prefix, const char *bdev_name, const char *fstype, int flags);
int  fs_umount(const char *prefix);
int  fs_activate(struct fs *fs);

int  fs_getfso(struct fso **fso, struct fs *fs, ino_t index);
int  fs_lookup(struct fso **fso, const char *pathname);
int  fs_putfso(struct fso *fso);
void fs_setstate(struct fso *fso, int state);
void fs_clrstate(struct fso *fso, int state);
void fs_state(struct fso *fso, int state);

int  fs_creat(struct fso **fso, const char *pathname, mode_t mode, dev_t rdev);
int  fs_chk_perm(struct fso *fso, int mode, uid_t uid, gid_t gid);
int  fs_access(const char *pathname, int mode);
int  fs_link(const char *oldname, const char *newname);
int  fs_unlink(const char *pathname);
int  fs_rename(const char *oldname, const char *newname);

int  fs_chdir(const char *name);
int  fs_mkdir(const char *name, mode_t mode);
int  fs_rmdir(const char *name);

int  fs_revoke(const char *pathname);

int  fs_sync(struct fs *fs);
int  fs_syncall(void);

int  fs_getfile(struct fs_file **file, struct fso *fso, int omode);
int  fs_putfile(struct fs_file *file);

int  fs_getfd(unsigned *fd, struct fs_file *file);
int  fs_putfd(unsigned fd);
int  fs_fdaccess(unsigned fd, int mode);

void fs_notify(const char *path);

#endif
