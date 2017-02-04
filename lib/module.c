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

#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <os386.h>
#include <errno.h>

int _mod_load(const char *pathname, const void *data, unsigned data_size)
{
	struct stat st;
	void *image;
	ssize_t cnt;
	int ret;
	int fd;
	
	fd = open(pathname, O_RDONLY);
	if (fd < 0)
		return -1;
	fstat(fd, &st);
	
	if (!S_ISREG(st.st_mode))
	{
		_set_errno(EINVAL);
		close(fd);
		return -1;
	}
	
	image = malloc(st.st_size);
	if (!image)
	{
		close(fd);
		return -1;
	}
	
	cnt = read(fd, image, st.st_size);
	close(fd);
	if (cnt < 0)
	{
		free(image);
		return -1;
	}
	
	if (cnt != st.st_size)
	{
		_set_errno(EINVAL);
		free(image);
		return -1;
	}
	
	ret = _mod_insert(pathname, image, st.st_size, data, data_size);
	free(image);
	return ret;
}
