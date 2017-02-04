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

#include <kern/printk.h>
#include <kern/task.h>
#include <kern/cio.h>
#include <kern/lib.h>
#include <errno.h>

struct cdev *cio_dev[CIO_MAXDEV];

int cio_install(struct cdev *cd)
{
	int i;
	
	for (i = 0; i < CIO_MAXDEV; i++)
		if (cio_dev[i] == cd)
			panic("cio_install: device already installed");
	
	for (i = 0; i < CIO_MAXDEV; i++)
		if (!cio_dev[i])
		{
			cio_dev[i] = cd;
			return 0;
		}
	
	return ENOMEM;
}

int cio_uinstall(struct cdev *cd)
{
	int i;
	
	for (i = 0; i < CIO_MAXDEV; i++)
		if (cio_dev[i] == cd)
		{
			cio_dev[i] = NULL;
			return 0;
		}
	
	panic("cio_install: device not installed");
}

int cio_find(struct cdev **cd, char *name)
{
	int i;
	
	for (i = 0; i < CIO_MAXDEV; i++)
		if (cio_dev[i] && !strcmp(cio_dev[i]->name, name))
		{
			*cd = cio_dev[i];
			return 0;
		}
	
	return ENODEV;
}

int cio_open(struct cdev *cd)
{
	int err;
	
	cd->refcnt++;
	if (cd->refcnt == 1)
	{
		err = cd->open(cd);
		if (err)
		{
			cd->refcnt--;
			return err;
		}
	}
	return 0;
}

int cio_close(struct cdev *cd)
{
	int err;
	
	if (!cd->refcnt)
		panic("cio_close: device not open");
	if (cd->refcnt == 1)
	{
		err = cd->close(cd);
		cd->refcnt--;
		return err;
	}
	cd->refcnt--;
	return 0;
}

void cio_setstate(struct cdev *cd, int state)
{
	int s;
	
	s = intr_dis();
	cio_state(cd, cd->state | state);
	intr_res(s);
}

void cio_clrstate(struct cdev *cd, int state)
{
	int s;
	
	s = intr_dis();
	cio_state(cd, cd->state & ~state);
	intr_res(s);
}

void cio_state(struct cdev *cd, int state)
{
	int s;
	
	s = intr_dis();
	cd->state = state;
	intr_res(s);
	
	if (cd->fso)
		fs_state(cd->fso, state);
}
