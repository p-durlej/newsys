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

#ifndef _DEV_CONSOLE_H
#define _DEV_CONSOLE_H

#define CF_COLOR	1
#define CF_ADDR		2

#define CIOGINFO	0x4301
#define CIOGOTO		0x4302
#define CIOPUTA		0x4303
#define CIODISABLE	0x4304
#define CIOMUTE		0x4305

struct cioinfo
{
	int	flags;
	int	w, h;
};

struct ciogoto
{
	int x, y;
};

struct cioputa
{
	const char *	data;
	int		len;
	int		x, y;
};

#ifndef _KERN_

#define con_info(d, buf)			ioctl((d), CIOGINFO, (buf))
#define con_goto(d, x_, y_)			ioctl((d), CIOGOTO, &(struct ciogoto){ .x = (x_), .y = (y_) })
#define con_puta(d, x_, y_, data_, len_)	ioctl((d), CIOPUTA, &(struct cioputa){ .x = (x_), .y = (y_), .data = (data_), .len = (len_) })
#define con_disable(d)				ioctl((d), CIODISABLE, 0)
#define con_mute(d)				ioctl((d), CIOMUTE, 0)

#endif

#endif
