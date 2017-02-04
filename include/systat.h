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

#ifndef _SYSTAT_H
#define _SYSTAT_H

#include <sys/types.h>
#include <limits.h>

typedef long long memstat_t;

struct systat
{
	int		task_avail;
	int		task_max;
	
	memstat_t	core_avail;
	memstat_t	core_max;
	
	memstat_t	kva_avail;
	memstat_t	kva_max;
	
	int		file_avail;
	int		file_max;
	
	int		fso_avail;
	int		fso_max;
	
	int		blk_dirty;
	int		blk_valid;
	int		blk_avail;
	int		blk_max;
	
	int		sw_freq;
	int		hz;
	
	time_t		uptime;
	
	int		cpu;
	int		cpu_max;
};

struct taskinfo
{
	char		pathname[PATH_MAX];
	char		queue[64];
	uid_t		euid;
	gid_t		egid;
	uid_t		ruid;
	gid_t		rgid;
	pid_t		ppid;
	pid_t		pid;
	memstat_t	size;
	unsigned	maxev;
	int		prio;
	int		cpu;
};

int _systat(struct systat *buf);
int _taskinfo(struct taskinfo *buf);
int _taskmax(void);

#endif
