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

#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>

int main(int argc, char **argv)
{
	uint32_t buf[16];
	uint32_t cs;
	int cnt;
	int fd;
	int i;
	
	fd = open(argv[1], O_RDWR);
	if (fd < 0)
		err(1, "%s: open", argv[1]);
	
	if (lseek(fd, 2048 * 0x12, SEEK_SET) < 0)
		err(1, "%s: lseek", argv[1]);
	
	cnt = read(fd, buf, sizeof buf);
	if (cnt != sizeof buf)
	{
		if (cnt >= 0)
			errx(1, "%s: Short read", argv[1]);
		err(1, "%s: read", argv[1]);
	}
	
	for (cs = 0, i = 0; i < sizeof buf / sizeof *buf; i++)
		cs += buf[i];
	buf[0x0e] -= cs;
	
	if (lseek(fd, 2048 * 0x12, SEEK_SET) < 0)
		err(1, "%s: lseek", argv[1]);
	
	if (write(fd, buf, sizeof buf) != sizeof buf)
		err(1, "%s: write", argv[1]);
	
	close(fd);
}
