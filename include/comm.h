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

#ifndef _COMM_H
#define _COMM_H

int ioctl(int fd, int cmd, void *buf);

struct comm_config
{
	unsigned char	word_len;
	unsigned char	parity;
	unsigned char	stop;
	int		speed;
};

struct comm_stat
{
	int	ibu;
	int	obu;
};

#define COMM_E_TRDY	1
#define COMM_E_RRDY	2

#define COMMIO_CHECK	0x6300
#define COMMIO_SETSPEED	0x6301
#define COMMIO_GETSPEED	0x6302
#define COMMIO_SETCONF	0x6303
#define COMMIO_GETCONF	0x6304
#define COMMIO_STAT	0x6305

#define is_comm(fd)			(!ioctl((fd), COMMIO_CHECK, NULL))
#define comm_set_speed(comm_fd, speed)	ioctl((comm_fd), COMMIO_SETSPEED, (speed))
#define comm_get_speed(comm_fd, speed)	ioctl((comm_fd), COMMIO_GETSPEED, (speed))
#define comm_set_conf(comm_fd, conf)	ioctl((comm_fd), COMMIO_SETCONF, (conf))
#define comm_get_conf(comm_fd, conf)	ioctl((comm_fd), COMMIO_GETCONF, (conf))
#define comm_stat(comm_fd, buf)		ioctl((comm_fd), COMMIO_STAT, (buf))

#endif
