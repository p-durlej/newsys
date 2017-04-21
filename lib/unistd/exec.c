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

#include <priv/libc.h>
#include <priv/sys.h>
#include <findexec.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

int execve(const char *path, char *const argv[], char *const envp[])
{
	struct arg_buf *arg;
	size_t size;
	int se;
	int r;
	
	if (access(path, X_OK))
		return -1;
	
	size = __libc_mkarg(&arg, argv, envp);
	if (size < 0)
		return -1;
	
	r  = _exec(path, arg, size);
	se = _get_errno();
	free(arg);
	_set_errno(se);
	return r;
}

int execv(const char *path, char *const argv[])
{
	return execve(path, argv, _get_environ());
}

int execl(const char *path, ...)
{
	va_list ap;
	int cnt;
	int i;
	
	va_start(ap, path);
	cnt = 1;
	while (va_arg(ap, char *))
		cnt++;
	va_end(ap);
	
	char *argv[cnt];
	
	va_start(ap, path);
	for (i = 0; i < cnt; i++)
		argv[i] = va_arg(ap, char *);
	va_end(ap);
	
	return execv(path, argv);
}

int execle(const char *path, ...)
{
	va_list ap;
	void *envp;
	int cnt;
	int i;
	
	va_start(ap, path);
	cnt = 1;
	while (va_arg(ap, char *))
		cnt++;
	va_end(ap);
	
	char *argv[cnt];
	
	va_start(ap, path);
	for (i = 0; i < cnt; i++)
		argv[i] = va_arg(ap, char *);
	
	envp = va_arg(ap, char **);
	
	va_end(ap);
	
	return execve(path, argv, envp);
}

int execvp(const char *path, char *const argv[])
{
	const char *exec;
	
	exec = _findexec(path, NULL);
	if (exec == NULL)
		return -1;
	return execv(exec, argv);
}

int execlp(const char *path, ...)
{
	va_list ap;
	int cnt;
	int i;
	
	va_start(ap, path);
	cnt = 1;
	while (va_arg(ap, char *))
		cnt++;
	va_end(ap);
	
	char *argv[cnt];
	
	va_start(ap, path);
	for (i = 0; i < cnt; i++)
		argv[i] = va_arg(ap, char *);
	
	va_end(ap);
	
	return execvp(path, argv);
}
