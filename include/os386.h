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

#ifndef _OS386_H
#define _OS386_H

#include <kern/intr_regs.h>
#include <sysload/flags.h>
#include <sys/types.h>
#include <limits.h>

#define PTY_PTSNAME	0x5000
#define PTY_UNLOCK	0x5001
#define PTY_SSIZE	0x5002
#define PTY_GSIZE	0x5003
#define PTY_SHUP	0x5004

struct pty_size
{
	int w, h;
};

#define DMESG_BUF	8192

#define SYNC_WRITE	1
#define SYNC_INVALIDATE	2

#define DEBUG_PTASKS	1
#define DEBUG_MDUMP	2
#define DEBUG_BLOCK	3

int _pg_alloc(unsigned start, unsigned end);
int _pg_free(unsigned start, unsigned end);
int _csync(void *p, size_t size);

int _boot_flags(void); /* XXX */
int _dmesg(char *buf, int size);
int _sysmesg(const char *msg);
int _panic(char *msg);
int _debug(int cmd);
int _sync1(int flags);

int _mod_insert(const char *pathname, const void *image, unsigned image_size,
		const void *data, unsigned data_size);
int _mod_load(const char *pathname, const void *data, unsigned data_size);
int _mod_unload(int md);

int _iopl(int level);

int _ctty(const char *pathname);

#define CHDIR_OBSERVE	1

int _chdir(const char *path, int flags);

#define SHUTDOWN_HALT	0
#define SHUTDOWN_REBOOT	1
#define SHUTDOWN_POWER	2

int _shutdown(int type);

struct core_head
{
	char		 pathname[PATH_MAX];
	struct intr_regs regs;
	int		 status;
};

struct core_page_head
{
	uint32_t size;
	uint32_t addr;
};

int _cprintf(const char *fmt, ...);

ssize_t _read(int fd, void *buf, size_t count);
ssize_t _write(int fd, const void *buf, size_t count);

struct fxstat
{
	int refcnt;
	int state;
};

#endif
