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

#ifndef _KERN_LIB_H
#define _KERN_LIB_H

#ifndef NULL
#define NULL	(void *)0
#endif

#include <kern/machine/machine.h>
#include <stddef.h>
#include <list.h>

#define READ_CACHE	1
#define WRITE_CACHE	2

void *	memcpy(void *d, const void *s, size_t size);
void *	memset(void *p, int c, size_t size);
void *	memmove(void *dest, const void *src, size_t n);

int	memcmp(const void *s1, const void *s2, size_t n);

unsigned inb(unsigned p);
unsigned inw(unsigned p);
unsigned inl(unsigned p);

void	outb(unsigned p, unsigned v);
void	outw(unsigned p, unsigned v);
void	outl(unsigned p, unsigned v);

void	insb(unsigned port, void *buf, unsigned len);
void	insw(unsigned port, void *buf, unsigned len);
void	insl(unsigned port, void *buf, unsigned len);

void	outsb(unsigned port, const void *buf, unsigned len);
void	outsw(unsigned port, const void *buf, unsigned len);
void	outsl(unsigned port, const void *buf, unsigned len);

char *	strcpy(char *dest, const char *src);
int	strcmp(const char *s1, const char *s2);
char *	strcat(char *dest, const char *src);
char *	strchr(const char *s, int c);
char *	strrchr(const char *s, int c);
size_t	strlen(const char *s);

char *	strncpy(char *dest, const char *src, size_t n);
int	strncmp(const char *s1, const char *s2, size_t n);

int	kmalloc(void *buf, unsigned size, const char *name);
int	krealloc(void *buf, void *ptr, unsigned size, const char *name);
void	free(void *ptr);
void *	malloc(unsigned size);
void *	realloc(void *ptr, unsigned size);
void	malloc_boot(void);
void	mnewref(void *ptr);

int	phys_map(void **p, unsigned base, unsigned size, int cache);
int	phys_unmap(void *p, unsigned size);

void *	dma_malloc(unsigned size);
void	dma_free(void *ptr, unsigned size);

#endif
