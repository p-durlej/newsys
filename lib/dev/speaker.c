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

#include <dev/speaker.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

static int spk_fd = -1;

int spk_tone(int fd, int freq, int dur)
{
	uint16_t f = freq;
	struct timeval tv0, tv;
	long delay;
	
	if (fd < 0)
	{
		fd = spk_fd;
		if (fd < 0)
		{
			fd = spk_open(NULL);
			if (fd < 0)
				return -1;
			spk_fd = fd;
			
			fcntl(fd, F_SETFD, FD_CLOEXEC);
		}
	}
	
	if (write(fd, &f, sizeof f) != sizeof f)
		return -1;
	if (dur < 0)
		return 0;
	
	gettimeofday(&tv, NULL);
	tv0 = tv;
	tv0.tv_usec += dur * 1000;
	tv0.tv_sec  += tv0.tv_usec / 1000000;
	tv0.tv_usec %= 1000000;
	do
	{
		delay  = (tv0.tv_sec  - tv.tv_sec) * 1000000;
		delay += (tv0.tv_usec - tv.tv_usec);
		if (delay >= 1000000)
			sleep(delay / 1000000);
		else
			usleep(delay);
		gettimeofday(&tv, NULL);
	} while (tv0.tv_sec > tv.tv_sec ||
		 (tv0.tv_sec == tv.tv_sec && tv0.tv_usec > tv.tv_usec));
	
	f = 0;
	if (write(fd, &f, sizeof f) != sizeof f)
		return -1;
	return 0;
}

void spk_close(int fd)
{
	if (fd < 0)
	{
		fd = spk_fd;
		if (fd < 0)
			return;
		spk_fd = -1;
	}
	close(fd);
}

int spk_open(const char *pathname)
{
	if (pathname == NULL)
	{
		pathname = getenv("SPEAKER");
		if (pathname == NULL)
		{
			_set_errno(ENXIO);
			return -1;
		}
	}
	return open(pathname, O_WRONLY);
}
