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

#ifndef _STDLIB_H
#define _STDLIB_H

#ifndef NULL
#define NULL	0
#endif

#include <libdef.h>
#include <sys/types.h>
#include <stdint.h> /* XXX inttypes.h? */

#define RAND_MAX	0x7fff

typedef struct
{
	intmax_t quot;
	intmax_t rem;
} imaxdiv_t;

typedef struct
{
	long long quot;
	long long rem;
} lldiv_t;

typedef struct
{
	long quot;
	long rem;
} ldiv_t;

typedef struct
{
	int quot;
	int rem;
} div_t;

int		atexit(void (*func)(void));

void		exit(int status) __attribute__((noreturn));
void		_Exit(int st) __attribute__((noreturn));
void		abort(void) __attribute__((noreturn));

void *		malloc(size_t size);
void		free(void *ptr);
void *		realloc(void *ptr, size_t size);
void *		calloc(size_t n, size_t m);

unsigned long long
		strtoull(const char *nptr, char **endptr, int base);
long long	strtoll(const char *nptr, char **endptr, int base);
unsigned long	strtoul(const char *nptr, char **endptr, int base);
long		strtol(const char *nptr, char **endptr, int base);
long		atoll(const char *nptr);
long		atol(const char *nptr);
int		atoi(const char *nptr);

intmax_t	imaxabs(intmax_t v);
long long	llabs(long long v);
long		labs(long v);
int		abs(int v);

imaxdiv_t	imaxdiv(intmax_t num, intmax_t den);
lldiv_t		lldiv(long long num, long long den);
ldiv_t		ldiv(long num, long den);
div_t		div(int num, int den);

char *		getenv(const char *n);
int		setenv(const char *n, const char *v, int o);
void		unsetenv(const char *n);
int		putenv(char *s);
int		clearenv(void);

int		system(const char *comm);

void *		bsearch(void *key, void *data, size_t cnt, size_t size, int (*cmp)(const void *a, const void *b));
void		qsort(void *data, size_t cnt, size_t size, int (*cmp)(const void *a, const void *b));

void		srand(unsigned seed);
int		rand(void);

#endif
