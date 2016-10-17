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

#include <kern/limits.h>
#include <kern/printk.h>
#include <kern/errno.h>
#include <kern/task.h>
#include <kern/lib.h>
#include <kern/fs.h>
#include <fcntl.h>

struct fs_file fs_file[FS_MAXFILE];

int fs_getfile(struct fs_file **file, struct fso *fso, int omode)
{
	int i;
	
	for (i = 0; i < FS_MAXFILE; i++)
		if (!fs_file[i].refcnt)
		{
			memset(&fs_file[i], 0, sizeof fs_file[i]);
			
			fs_file[i].refcnt = 1;
			fs_file[i].fso	  = fso;
			fs_file[i].omode  = omode;
			
			if (fso)
				switch (omode & 7)
				{
				case O_RDONLY:
					fso->reader_count++;
					break;
				case O_WRONLY:
					fso->writer_count++;
					break;
				case O_RDWR:
					fso->reader_count++;
					fso->writer_count++;
					break;
				}
			
			*file = &fs_file[i];
			return 0;
		}
	
	return ENFILE;
}

int fs_putfile(struct fs_file *file)
{
	int err;
	
	if (!file->refcnt)
		panic("fdesc.c: fs_putfile: !file->refcnt");
	
	if (file->refcnt == 1)
	{
		switch (file->omode & 7)
		{
		case O_RDONLY:
			file->fso->reader_count--;
			break;
		case O_WRONLY:
			file->fso->writer_count--;
			break;
		case O_RDWR:
			file->fso->reader_count--;
			file->fso->writer_count--;
			break;
		}
		err = fs_putfso(file->fso);
		file->refcnt--;
		return err;
	}
	file->refcnt--;
	return 0;
}

int fs_getfd(unsigned *fd, struct fs_file *file)
{
	int i;
	
	for (i = 0; i < OPEN_MAX; i++)
		if (!curr->file_desc[i].file)
		{
			curr->file_desc[i].file		 = file;
			curr->file_desc[i].close_on_exec = 0;
			
			*fd = i;
			return 0;
		}
	
	return EMFILE;
}

int fs_putfd(unsigned fd)
{
	if (fd >= OPEN_MAX)
		return EBADF;
	
	if (!curr->file_desc[fd].file)
		return EBADF;
	
	fs_putfile(curr->file_desc[fd].file);
	memset(&curr->file_desc[fd], 0, sizeof(struct fs_desc));
	return 0;
}

int fs_fdaccess(unsigned fd, int mode)
{
	struct fs_file *f;
	
	if (fd >= OPEN_MAX)
		return EBADF;
	
	if (!curr->file_desc[fd].file)
		return EBADF;
	
	f = curr->file_desc[fd].file;
	
	if (mode & R_OK)
	{
		switch (f->omode & 7)
		{
		case O_RDONLY:
			break;
		case O_RDWR:
			break;
		default:
			return EBADF;
		}
	}
	
	if (mode & W_OK)
	{
		switch (f->omode & 7)
		{
		case O_WRONLY:
			break;
		case O_RDWR:
			break;
		default:
			return EBADF;
		}
	}
	
	if (mode & X_OK)
		return EINVAL;
	
	if (!f->fso)
		return 0;
	return fs_chk_perm(f->fso, mode, curr->euid, curr->egid);
}
