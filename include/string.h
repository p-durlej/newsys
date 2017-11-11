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

#include <libdef.h>
#include <sys/types.h>

void *	memccpy(void *dest, const void *src, int c, size_t n);
void *	memchr(const void *s, int c, size_t n);
int	memcmp(const void *s1, const void *s2, size_t n);
void *	memcpy(void *dest, const void *src, size_t n);
void *	memmove(void *dest, const void *src, size_t n);
void *	memset(void *s, int c, size_t n);

char *	strcat(char *dest, const char *src);
char *	strchr(const char *s, int c);
int	strcmp(const char *s1, const char *s2);
char *	strcpy(char *dest, const char *src);
size_t	strcspn(const char *s, const char *reject);
int	strlen(const char *s);
char *	strncat(char *dest, const char *src, size_t n);
int	strncmp(const char *s1, const char *s2, size_t n);
char *	strncpy(char *dest, const char *src, size_t n);
char *	strpbrk(const char *s, const char *accept);
char *	strrchr(const char *s, int c);
size_t	strspn(const char *s, const char *accept);
char *	strstr(const char *s1, const char *s2);
char *	strtok(char *s, const char *delim);

const char *strsignal(int sig);
const char *strerror(int errno);

char *strdup(const char *s);

void bcopy(const void *s, void *d, size_t len);
void bzero(void *p, size_t len);
