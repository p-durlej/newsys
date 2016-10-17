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

#include <priv/libc_globals.h>
#include <priv/libc.h>
#include <priv/sys.h>
#include <sys/types.h>
#include <findexec.h>
#include <newtask.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

static const char * const null_env[1] = { NULL };

size_t __libc_mkarg(struct arg_buf **buf, char *const *argv, char *const *envp)
{
	struct arg_buf *arg;
	char *const *sp;
	char *dp;
	size_t size;
	size_t len;
	int ac;
	int ec;
	
	if (!envp)
		envp = (void *)null_env;
	
	size = 0;
	for (ac = 0, sp = argv; *sp; sp++, ac++)
		size += strlen(*sp) + 1;
	for (ec = 0, sp = envp; *sp; sp++, ec++)
		size += strlen(*sp) + 1;
	size += sizeof(struct arg_head);
	
	if (size > ARG_MAX)
	{
		_set_errno(E2BIG);
		return -1;
	}
	
	arg = malloc(size);
	if (!arg)
		return -1;
	memset(arg, 0, size);
	dp = arg->buf;
	*buf = arg;
	
	arg->hd.umask = __libc_umask;
	arg->hd.argc  = ac;
	arg->hd.envc  = ec;
	
	for (sp = argv; *sp; sp++)
	{
		len = strlen(*sp) + 1;
		memcpy(dp, *sp, len);
		dp += len;
	}
	for (sp = envp; *sp; sp++)
	{
		len = strlen(*sp) + 1;
		memcpy(dp, *sp, len);
		dp += len;
	}
	
	return size;
}

int _newtaskve(const char *path, char *const *argv, char *const *envp)
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
	
	r  = _newtask(path, arg, size);
	se = _get_errno();
	free(arg);
	_set_errno(se);
	return r;
}

int _newtaskv(const char *path, char *const *argv)
{
	return _newtaskve(path, argv, _get_environ());
}

int _newtaskl(const char *path, ...)
{
	char buf[256];
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
	
	return _newtaskv(path, argv);
}

int _newtaskle(const char *path, ...)
{
	char buf[256];
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
	
	return _newtaskve(path, argv, envp);
}

int _newtaskvp(const char *path, char *const *argv)
{
	const char *exec;
	
	exec = _findexec(path, NULL);
	if (exec == NULL)
		return -1;
	return _newtaskv(exec, argv);
}

int _newtasklp(const char *path, ...)
{
	char buf[256];
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
	
	return _newtaskvp(path, argv);
}
