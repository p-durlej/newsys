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

#ifndef _KERN_CIO_H
#define _KERN_CIO_H

#include <sys/types.h>

#define CIO_MAXDEV	64

#define R_OK		4
#define W_OK		2

struct cio_request;
struct fso;

struct cdev
{
	char *	name;
	int	unit;
	int	no_seek;
	int	(*open)(struct cdev *cd);
	int	(*close)(struct cdev *cd);
	int	(*read)(struct cio_request *rq);
	int	(*write)(struct cio_request *rq);
	int	(*ioctl)(struct cdev *cd, int cmd, void *buf);
	
	struct fso *fso;
	int	state;
	int	refcnt;
};

struct cio_request
{
	struct cdev *cdev;
	int	no_delay;
	uoff_t	offset;
	size_t	count;
	void *	buf;
};

extern struct cdev *cio_dev[CIO_MAXDEV];

void cio_update(void);

int cio_install(struct cdev *cd);
int cio_uinstall(struct cdev *cd);
int cio_find(struct cdev **cd, char *name);

int cio_open(struct cdev *cd);
int cio_close(struct cdev *cd);

void cio_setstate(struct cdev *cd, int state);
void cio_clrstate(struct cdev *cd, int state);
void cio_state(struct cdev *cd, int state);

#endif
