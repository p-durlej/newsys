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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static char ***environ_ptr;

void _set_environ_ptr(char ***env_p)
{
	char **old_env = NULL;
	
	if (environ_ptr)
		old_env = *environ_ptr;
	
	environ_ptr  = env_p;
	*environ_ptr = old_env;
}

void _set_environ(char **env)
{
	if (!environ_ptr)
		abort();
	*environ_ptr = env;
}

char **_get_environ(void)
{
	if (!environ_ptr)
		return NULL;
	return *environ_ptr;
}

static int env_find(const char *n, int p)
{
	int l = p ? (strchr(n, '=') - n) : strlen(n);
	int i;
	char **env;
	
	if (!environ_ptr)
		return -1;
	env = _get_environ();
	if (!env)
		return -1;
	
	for (i = 0; env[i]; i++)
		if (!strncmp(env[i], n, l))
			return i;
	return -1;
}

char *getenv(const char *n)
{
	int i;
	
	i = env_find(n, 0);
	if (i < 0)
		return NULL;
	return strchr((*environ_ptr)[i] + 1, '=') + 1;
}

int setenv(const char *n, const char *v, int o)
{
	char *s;
	int i;
	
	i = env_find(n, 0);
	if (i >= 0 && !o)
		return 0;
	
	s = malloc(strlen(n) + strlen(v) + 2);
	if (!s)
		return -1;
	
	strcpy(s, n);
	strcat(s, "=");
	strcat(s, v);
	return putenv(s);
}

void unsetenv(const char *n)
{
	int i = env_find(n, 0);
	char **env;
	
	if (i < 0)
		return;
	
	env = _get_environ();
	while(env[i++])
		env[i - 1] = env[i];
}

int putenv(char *s)
{
	char **env = *environ_ptr;
	int i;
	
	if (!strchr(s, '='))
	{
		unsetenv(s);
		return 0;
	}
	
	i = env_find(s, 1);
	if (i < 0)
	{
		char **nenv;
		
		i = 0;
		if (env)
			while (env[i++]);
		nenv = realloc(env, sizeof *nenv * (i + 1));
		if (!nenv)
			return -1;
		_set_environ(env = nenv);
		env[i] = NULL;
		i--;
	}
	
	env[i] = s;
	return 0;
}

int clearenv(void)
{
	free(_get_environ());
	_set_environ(NULL);
	return 0;
}
