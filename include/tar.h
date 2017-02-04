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

#include <sys/types.h>
#include <sys/stat.h>

#define TAR_LT_REG	'0'
#define TAR_LT_HARD	'1'
#define TAR_LT_SYMBOLIC	'2'
#define TAR_LT_CHR	'3'
#define TAR_LT_BLK	'4'
#define TAR_LT_DIR	'5'
#define TAR_LT_FIFO	'6'

struct tar_hdr
{
	char pathname[100];
	char mode[8];
	char owner[8];
	char group[8];
	char size[12];
	char mtime[12];
	char cksum[8];
	char link_type;
	char link[100];
};

struct tar_file
{
	const char *	pathname;
	mode_t		mode;
	uid_t		owner;
	gid_t		group;
	off_t		size;
	time_t		mtime;
	const char *	link;
	int		link_type;
	
	off_t		offset;
};

struct tar
{
	struct tar_file *files;
	int 	file_cnt;
	
	off_t	append_offset;
	int	fd;
};

struct tar *tar_openw(const char *pathname);
struct tar *tar_open(const char *pathname);
void tar_close(struct tar *tar);

ssize_t tar_read(const struct tar *tar, const struct tar_file *file, void *buf, size_t len, off_t off);
