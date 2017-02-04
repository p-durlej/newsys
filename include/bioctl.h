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

#ifndef _BIOCTL_H
#define _BIOCTL_H

#include <limits.h>

struct bdev_stat
{
	char		name[NAME_MAX + 1];
	uint64_t	read_cnt;
	uint64_t	write_cnt;
	uint64_t	error_cnt;
};

struct bio_info
{
	int	type;
	int	os;
	
	size_t	offset;
	size_t	size;
	
	int	ncyl;
	int	nhead;
	int	nsect;
};

#define BIO_TYPE_FD		0
#define BIO_TYPE_HD		1
#define BIO_TYPE_HD_SECTION	2
#define BIO_TYPE_CD		3
#define BIO_TYPE_SSD		4

#define BIO_OS_SELF		0xcc /* XXX */

#define BIO_INFO		0x6201

#ifndef _KERN_

#include <sys/ioctl.h>
#include <sys/types.h>
#include <stdint.h>

int _bdev_stat(struct bdev_stat *buf);
int _bdev_max(void);

int _blk_add(int count);

#endif

#endif
