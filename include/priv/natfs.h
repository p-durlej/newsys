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

#ifndef _PRIV_NATFS_H
#define _PRIV_NATFS_H

#define NAT_MAGIC		"BE32ULF"

#define NAT_FAT_FREE		0
#define NAT_FAT_EOF		(-1)
#define NAT_FAT_SKIP		(-2)
#define NAT_FAT_BAD		(-3)

#define NAT_NAME_MAX		27
#define NAT_FILE_SIZE_MAX	(0x7fffffff)
#define NAT_DIR_SIZE_MAX	(0x00100000)

#define NAT_D_PER_BLOCK		(BLK_SIZE / sizeof(struct nat_dirent)) /* XXX */

#include <sys/types.h>
#include <stdint.h>

struct nat_super
{
	char	 magic[8];
	uint32_t bam_block;
	uint32_t bam_size;
	uint32_t data_block;
	uint32_t data_size;
	uint32_t root_block;
	uint8_t	 ndirblks;
	uint8_t	 nindirlev;
	int	 dirty;
};

struct nat_dirent
{
	char	 name[NAT_NAME_MAX + 1];
	uint32_t first_block;
};

struct nat_header
{
	uint64_t ctime;
	uint64_t mtime;
	uint64_t atime;
	int32_t	 uid;
	int32_t	 gid;
	uint32_t size;
	uint32_t mode;
	uint32_t nlink;
	uint32_t blocks;
	uint32_t rdev;
	uint8_t	 ndirblks;
	uint8_t	 nindirlev;
	uint16_t padding;
	uint32_t bmap[114];
};

#endif
