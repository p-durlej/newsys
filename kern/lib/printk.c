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

#include <kern/console.h>
#include <kern/printk.h>
#include <kern/errno.h>
#include <kern/lib.h>

char *sys_errlist[] =
{
	[EZERO]		= "Success (EZERO)",
	[ENOENT]	= "No such file or directory (ENOENT)",
	[EACCES]	= "Permission denied (EACCES)",
	[EPERM]		= "Operation not permitted (EPERM)",
	[E2BIG]		= "Argument list too big (E2BIG)",
	[EBUSY]		= "Device busy (EBUSY)",
	[ECHILD]	= "No child processes (ECHILD)",
	[EEXIST]	= "File exists (EEXIST)",
	[EFAULT]	= "Invalid address (EFAULT)",
	[EFBIG]		= "File too big (EFBIG)",
	[EINTR]		= "System call interrupted (EINTR)",
	[EINVAL]	= "Invalid value (EINVAL)",
	[ENOMEM]	= "Not enough memory (ENOMEM)",
	[EMFILE]	= "Too many open files (EMFILE)",
	[EMLINK]	= "Too many links (EMLINK)",
	[EAGAIN]	= "Try again (EAGAIN)",
	[ENAMETOOLONG]	= "File name too long (ENAMETOOLONG)",
	[ENFILE]	= "Too many open files in system (ENFILE)",
	[ENODEV]	= "No such device (ENODEV)",
	[ENOEXEC]	= "Bad file format (ENOEXEC)",
	[EIO]		= "I/O error (EIO)",
	[ENOSPC]	= "No space left on device (ENOSPC)",
	[ENOSYS]	= "System call not implemented (ENOSYS)",
	[ENOTDIR]	= "Not a directory (ENOTDIR)",
	[ENOTEMPTY]	= "Directory not empty (ENOTEMPTY)",
	[ENOTTY]	= "Not a tty (ENOTTY)",
	[EPIPE]		= "Broken pipe (EPIPE)",
	[EROFS]		= "Read only file system (EROFS)",
	[ESPIPE]	= "Illegal seek (ESPIPE)",
	[ESRCH]		= "No such process (ESRCH)",
	[EXDEV]		= "Cross-device link (EXDEV)",
	[ERANGE]	= "Result too large (ERANGE)",
	[EBADF]		= "Bad file descriptor (EBADF)",
	[EISDIR]	= "Is a directory (EISDIR)",
	[EDEADLK]	= "Deadlock avoided (EDEADLK)",
	[ENXIO]		= "No such device (ENXIO)",
	[ETXTBSY]	= "Text file busy (ETXTBSY)",
	
	[EMOUNTED]	= "Filesystem already mounted (EMOUNTED)",
	[ENODESKTOP]	= "Process has no desktop (ENODESKTOP)",
	[ENOKS]		= "No such kernel service (ENOKS)",
	[EBADKSD]	= "Bad kernel service descriptor (EBADKSD)",
	
	[EBADM]		= "Bad module descriptor (EBADM)",
	[ENSYM]		= "Unresolved symbol (ENSYM)",
};

void perror(const char *msg, int errno)
{
	if (errno < 0 || errno >= sizeof sys_errlist / sizeof *sys_errlist)
	{
		printk("perror: msg = \"%s\", errno = %i\n", msg, errno);
		panic("perror: bad errno");
	}
	
	if (msg)
		printk("%s: %s (%i)\n", msg, sys_errlist[errno], errno);
	else
		printk("%s (%i)\n", sys_errlist[errno], errno);
}
