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

#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>

#include "../include/sysload/head.h"

static struct
{
	const char *	name;
	int		val;
} modes[] =
{
	{ "disk",	BOOT_TYPE_DISK	},
	{ "pxe",	BOOT_TYPE_PXE	},
	{ "cdrom",	BOOT_TYPE_CDROM	},
};

int main(int argc, char **argv)
{
	uint32_t buf;
	int fd;
	int i;
	
	if (argc != 3)
		errx(1, "usage: psysload SYSLOAD {disk|pxe|cdrom}");
	
	for (i = 0; i < sizeof modes / sizeof *modes; i++)
		if (!strcmp(argv[2], modes[i].name))
		{
			buf = modes[i].val;
			break;
		}
	
	if (i >= sizeof modes / sizeof *modes)
		errx(1, "%s: bad mode", argv[2]);
	
	fd = open(argv[1], O_WRONLY);
	if (fd < 0)
		err(1, "%s: open", argv[1]);
	
	if (lseek(fd, BOOT_TYPE_OFFSET, SEEK_SET) < 0)
		err(1, "%s: lseek", argv[1]);
	
	errno = EINVAL;
	if (write(fd, &buf, sizeof buf) != sizeof buf)
		err(1, "%s: write", argv[1]);
	
	if (close(fd))
		err(1, "%s: close", argv[1]);
	
	return 0;
}
