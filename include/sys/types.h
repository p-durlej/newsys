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

#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

#include <arch/archdef.h>
#include <arch/size_t.h>
#include <libdef.h>
#include <stdint.h>

typedef unsigned int	uint;	/* XXX */
typedef unsigned long	ulong;	/* XXX */

typedef int		sig_t;
typedef int		pid_t;
typedef int32_t		uid_t;
typedef uid_t		gid_t;
typedef uint32_t	mode_t;
typedef uint32_t	dev_t;
typedef int64_t		off_t;
typedef uint64_t	uoff_t;
typedef uint32_t	ino_t;
typedef int64_t		time_t;
typedef uint32_t	nlink_t;
typedef uint32_t	blk_t;
typedef blk_t		blkcnt_t;
typedef uint32_t	blksize_t;
typedef uint32_t	useconds_t;

#define makedev(maj, min)	((uint32_t)(maj) << 16 | (uint32_t)(min))
#define major(dev)		((uint32_t)(dev >> 16) & 0xffff)
#define minor(dev)		((uint32_t)(dev	       & 0xffff))

#endif
