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

#ifndef _SYSLOAD_KPARAM_H
#define _SYSLOAD_KPARAM_H

#include <stdint.h>

#define KMAPENT_RSVD	0
#define KMAPENT_MEM	1
#define KMAPENT_SYSTEM	2

#define KFB_TEXT	1
#define KFB_GRAPH	2
#define KFB_VGA		3

struct kmodimg
{
	struct kmodimg *next;
	
	void *	 image;
	void *	 args;
	uint32_t args_size;
	char	 name[32];
};

struct kmapent
{
	int	 type;
	uint64_t base;
	uint64_t size;
};

struct kfbmode
{
	int xres, yres;
	int type;
	int bpp;
};

struct kfb
{
	struct kfb *	next;
	
	int		bytes_per_line;
	int		xres, yres;
	int		type;
	int		bpp;
	
	uint64_t	io_base;
	uint64_t	base;
	
	struct kfbmode *modes;
	int		mode_cnt;
};

struct kparam
{
	uint32_t	conv_mem_size;	/* i386 only; must be the first member */
	struct kmodimg *mod_list;	/* must be the second member */
	struct kmapent *mem_map;
	int		mem_cnt;
	char		root_dev[16];
	char		root_type[16];
	uint32_t	root_flags;
	uint32_t	boot_flags;
	uint64_t	rd_base;
	uint32_t	rd_blocks;
	struct kfb *	fb;
};

struct khead
{
	char	 code[16];
	uint32_t base;
	uint32_t size;
};

#endif
