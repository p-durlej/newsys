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

#include <sys/types.h>
#include <stdint.h>

#define ELO_PAGE_SIZE	4096
#define ELO_PAGE_SHIFT	12

typedef uint32_t elo_caddr_t;

struct elo_data
{
	int		absolute;
	char *		pathname;
	int		fd;
	
	char *		image;
	elo_caddr_t	entry;
	elo_caddr_t	base;
	elo_caddr_t	end;
};

struct elf_head;

void	elo_reloc(struct elo_data *ed, void *addr, int type);
void	elo_check(struct elf_head *hd);

void	elo_fail(const char *msg) __attribute__((noreturn));
void	elo_mesg(const char *msg);

int	elo_read(int fd, void *buf, size_t sz);
int	elo_seek(int fd, off_t off);

int	elo_pgalloc(unsigned start, unsigned end);
void *	elo_malloc(size_t sz);
void	elo_free(void *p);

void	elo_csync(void *p, size_t sz);

void	elo_load(struct elo_data *ed);
