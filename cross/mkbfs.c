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

#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#define VERBOSE		0

#define BFS_PATH_MAX	499
#define BFS_NAME_MAX	31

#define BFS_D_PER_BLOCK	(BLK_SIZE / sizeof(struct bfs_dirent))

struct bfs_head
{
	unsigned mode;
	unsigned size;
	unsigned next;
	char name[BFS_PATH_MAX + 1];
};

struct bfs_dirent
{
	char name[BFS_NAME_MAX + 1];
};

int dev_fd;
char *dev_path;
unsigned offset;

unsigned curr_blk;

void mkhead(struct bfs_head *hd, char *name, mode_t mode, unsigned size)
{
	memset(hd, 0, sizeof *hd);
	
	hd->size = size;
	hd->next = (hd->size + 511) / 512 + curr_blk + 1;
	hd->mode = 0;
	
	strcpy(hd->name, name);
	
	switch (mode & S_IFMT)
	{
		case S_IFREG:
			hd->mode = 0;
			break;
		case S_IFDIR:
			hd->mode = 030000;
			break;
	}
	
	if (mode & S_IRUSR)
		hd->mode |= 0400;
	if (mode & S_IWUSR)
		hd->mode |= 0200;
	if (mode & S_IXUSR)
		hd->mode |= 0100;
}

void add_block(void *data)
{
	int cnt;
	
	if (lseek(dev_fd, (offset + curr_blk) * 512, SEEK_SET) < 0)
	{
		perror(dev_path);
		exit(errno);
	}
	
	cnt = write(dev_fd, data, 512);
	if (cnt < 0)
	{
		perror(dev_path);
		exit(errno);
	}
	if (cnt != 512)
	{
		fprintf(stderr, "%s: partial write\n", dev_path);
		exit(255);
	}
	
	curr_blk++;
}

void add_reg(char *spath, char *dpath)
{
	int fd = open(spath, O_RDONLY);
	struct bfs_head hd;
	struct stat st;
	char buf[512];
	int cnt;
	
#if VERBOSE
	printf("adding \"%s\" (reg)\n", spath);
#endif
	
	if (fd < 0 || fstat(fd, &st))
	{
		perror(spath);
		exit(errno);
	}
	
	mkhead(&hd, dpath, st.st_mode, st.st_size);
	add_block(&hd);
	
	while (curr_blk < hd.next)
	{
		memset(buf, 0, 512);
		cnt = read(fd, buf, 512);
		if (cnt < 0)
		{
			perror(spath);
			exit(errno);
		}
		add_block(buf);
	}
	
	close(fd);
}

void add_dir(char *dir_path, char *prefix, int last)
{
	struct bfs_dirent bde;
	struct bfs_head hd;
	struct dirent *de;
	struct stat st;
	unsigned nblk;
	int items = 0;
	DIR *dir;
	
	dir = opendir(dir_path);
	if (!dir)
	{
		perror(dir_path);
		exit(errno);
	}
	
	while (errno=0, readdir(dir))
	{
		if (errno)
		{
			perror(dir_path);
			exit(errno);
		}
		items++;
	}
	
#if VERBOSE
	printf("adding \"%s\" (dir), %i items\n", dir_path, items);
#endif
	
	rewinddir(dir);
	while (errno=0, de=readdir(dir))
	{
		char sub_dpath[BFS_PATH_MAX + 1];
		char sub_spath[PATH_MAX];
		
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;
		
		strcpy(sub_spath, dir_path);
		strcat(sub_spath, "/");
		strcat(sub_spath, de->d_name);
		
		if (*prefix)
		{
			strcpy(sub_dpath, prefix);
			strcat(sub_dpath, "/");
			strcat(sub_dpath, de->d_name);
		}
		else
			strcpy(sub_dpath, de->d_name);
		
		if (errno)
		{
			perror(dir_path);
			exit(errno);
		}
		
		if (stat(sub_spath, &st))
		{
			perror(sub_spath);
			exit(errno);
		}
		
		switch (st.st_mode & S_IFMT)
		{
			case S_IFDIR:
				add_dir(sub_spath, sub_dpath, 0);
				break;
			case S_IFREG:
				add_reg(sub_spath, sub_dpath);
				break;
			default:
				fprintf(stderr, "%s: inode type unsupported by BFS\n", sub_spath);
				exit(255);
		}
	}
	
	if (stat(dir_path, &st))
	{
		perror(dir_path);
		exit(errno);
	}
	mkhead(&hd, prefix, st.st_mode, sizeof bde * items);
	
	nblk = hd.next;
	if (last)
		hd.next = 0;
	add_block(&hd);
	curr_blk = nblk;
	
	rewinddir(dir);
	while (errno=0, de=readdir(dir))
	{
		if (errno)
		{
			perror(dir_path);
			exit(errno);
		}
		
		memset(&bde, 0, sizeof bde);
		strncpy(bde.name, de->d_name, BFS_NAME_MAX + 1);
		if (write(dev_fd, &bde, sizeof bde) != sizeof bde)
		{
			if (errno)
			{
				perror(dev_path);
				exit(errno);
			}
			fprintf(stderr, "%s: partial write\n", dev_path);
			exit(255);
		}
	}
	closedir(dir);
}

void align_sz(void)
{
	struct stat st;
	off_t sz;
	
	sz  = offset + curr_blk;
	sz *= 512;
	
	if (fstat(dev_fd, &st))
	{
		perror(dev_path);
		exit(errno);
	}
	if (st.st_size >= sz)
		return;
	if (ftruncate(dev_fd, sz))
	{
		perror(dev_path);
		exit(errno);
	}
}

int main(int argc, char **argv)
{
	if (argc != 5)
	{
		fprintf(stderr, "usage: mkbfs DEVICE OFFSET FIRST_BLOCK TEMPLATE\n");
		return 255;
	}
	
	dev_path = argv[1];
	dev_fd = open(dev_path, O_WRONLY | O_CREAT, 0666);
	if (dev_fd < 0)
	{
		perror(dev_path);
		return errno;
	}
	offset   = strtoul(argv[2], NULL, 0);
	curr_blk = strtoul(argv[3], NULL, 0);
	
	add_dir(argv[4], "", 1);
	align_sz();
	return 0;
}
