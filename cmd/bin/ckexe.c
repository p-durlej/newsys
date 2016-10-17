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
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

struct exehdr
{
	char magic[4];
	unsigned os_major;
	unsigned os_minor;
	unsigned cksum;
};

int update;

int ckexe(char *pathname)
{
	static unsigned buf[8192];
	
	struct exehdr xhdr;
	unsigned cksum = 0;
	unsigned *p;
	int cnt;
	int fd;
	
	fd = open(pathname, update ? O_RDWR : O_RDONLY);
	if (fd < 0)
		return -1;
	
	while ((cnt = read(fd, buf, sizeof buf)))
	{
		if (cnt < 0)
		{
			close(fd);
			return -1;
		}
		
		memset((char *)buf + cnt, 0, sizeof buf - cnt);
		
		cnt = sizeof buf / sizeof *buf;
		p   = buf;
		while (cnt--)
			cksum += *p++;
	}
	
	if (!update)
	{
		close(fd);
		if (cksum)
		{
			errno = ENOEXEC;
			return -1;
		}
		return 0;
	}
	
	if (lseek(fd, 0L, SEEK_SET))
	{
		close(fd);
		return -1;
	}
	errno = ENOEXEC;
	cnt = read(fd, &xhdr, sizeof xhdr);
	if (cnt != sizeof xhdr)
	{
		close(fd);
		return -1;
	}
	if (memcmp(xhdr.magic, "\0EXE", 4))
	{
		close(fd);
		errno = ENOEXEC;
		return -1;
	}
	
	xhdr.cksum -= cksum;
	
	if (lseek(fd, 0L, SEEK_SET))
	{
		close(fd);
		return -1;
	}
	cnt = write(fd, &xhdr, sizeof xhdr);
	if (cnt != sizeof xhdr)
	{
		close(fd);
		return -1;
	}
	
	close(fd);
	return 0;
}

int main(int argc, char **argv)
{
	int err = 0;
	int i;
	
	for (i = 1; i < argc; i++)
	{
		if (*argv[i] != '-')
			break;
		
		if (!strcmp(argv[i], "-w"))
			update = 1;
	}
	
	for (; i < argc; i++)
		if (ckexe(argv[i]))
		{
			perror(argv[i]);
			err = 1;
		}
	return err;
}
