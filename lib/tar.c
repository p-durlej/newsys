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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "tar.h"

static void tar_parse_hdr(struct tar_file *file, struct tar_hdr *hd)
{
	file->pathname	= strdup(hd->pathname);
	file->link	= strdup(hd->link);
	
	file->mode	= strtoul(hd->mode,  NULL, 8);
	file->owner	= strtoul(hd->owner, NULL, 8);
	file->group	= strtoul(hd->group, NULL, 8);
	file->size	= strtoul(hd->size,  NULL, 8);
	file->mtime	= strtoul(hd->mtime, NULL, 8);
	file->link_type	= hd->link_type;
}

struct tar *tar_open2(const char *pathname, int wr)
{
	union
	{
		struct tar_hdr hd;
		char buf[512];
	} u;
	struct tar *tar;
	ssize_t rcnt;
	off_t off;
	int fcnt;
	int zcnt;
	int i;
	
	tar = calloc(1, sizeof *tar);
	if (tar == NULL)
		goto fail;
	
	tar->fd = open(pathname, wr ? O_RDWR : O_RDONLY);
	if (tar->fd < 0)
		goto fail;
	
	memset(u.buf, 0, sizeof u.buf);
	fcnt = 0;
	zcnt = 0;
	while (rcnt = read(tar->fd, &u.buf, sizeof u.buf), rcnt == sizeof u.buf)
	{
		off_t size;
		
		if (!*u.hd.pathname && ++zcnt >= 2)
			break;
		
		size  = strtoul(u.hd.size, NULL, 8);
		size +=  511;
		size &= ~511;
		
		lseek(tar->fd, size, SEEK_CUR);
		
		if (*u.hd.pathname)
			fcnt++;
	}
	
	tar->files = calloc(fcnt, sizeof *tar->files);
	tar->file_cnt = fcnt;
	
	if (!fcnt)
		return tar;
	
	lseek(tar->fd, 0L, SEEK_SET);
	
	off = 0;
	i = 0;
	while (i < fcnt)
	{
		off_t size;
		
		rcnt = read(tar->fd, &u.buf, sizeof u.buf);
		
		if (rcnt < sizeof u.buf)
		{
			tar_close(tar);
			_set_errno(ENOEXEC);
			return NULL;
		}
		
		if (!*u.hd.pathname)
		{
			off += 512;
			continue;
		}
		
		tar_parse_hdr(&tar->files[i], &u.hd);
		tar->files[i++].offset = off + 512;
		
		size  = strtoul(u.hd.size, NULL, 8);
		size +=  511;
		size &= ~511;
		
		lseek(tar->fd, size, SEEK_CUR);
		off += size + 512;
		
		tar->append_offset = off;
	}
	
	return tar;
fail:
	if (tar != NULL)
	{
		if (tar->fd >= 0)
			close(tar->fd);
		free(tar);
	}
}

struct tar *tar_open(const char *pathname)
{
	return tar_open2(pathname, 0);
}

struct tar *tar_openw(const char *pathname)
{
	return tar_open2(pathname, 1);
}

void tar_close(struct tar *tar)
{
	if (tar == NULL)
		return;
	
	close(tar->fd);
	
	free(tar->files);
	free(tar);
}

ssize_t tar_read(const struct tar *tar, const struct tar_file *file, void *buf, size_t len, off_t off)
{
	size_t isz;
	
	if (off > file->size)
		return 0;
	
	if (off + len > file->size)
		isz = file->size - off;
	else
		isz = len;
	
	if (lseek(tar->fd, file->offset + off, SEEK_SET) < 0)
		return -1;
	
	return read(tar->fd, buf, isz);
}
