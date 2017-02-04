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

#ifndef _STDIO_H
#define _STDIO_H

#include <sys/types.h>
#include <stdarg.h>	/* XXX */
#include <limits.h>

#include <seekdef.h>
#include <libdef.h>

#define BUFSIZ			64

#define _IONBF			0
#define _IOLBF			1
#define _IOFBF			2
#define __LIBC_IOBF_STROUT	3

#define EOF			(-1)

#define __LIBC_FMODE_R		1
#define __LIBC_FMODE_W		2
#define __LIBC_FMODE_A		4

typedef off_t fpos_t;

typedef struct __libc_file
{
	int fd;
	int tty;
	int mode;
	
	int ungotc;
	int eof;
	int err;
	
	int buf_mode;
	
	int (*fputc)(int c, struct __libc_file *f);
	int (*fgetc)(struct __libc_file *f);
	
	union
	{
		struct
		{
			size_t cap;
			size_t pos;
			char *str;
		} strout;
		
		struct
		{
			char *buf;
			int buf_size;
			int buf_pos;
			int buf_insize;
			int buf_write;
			int buf_malloc;
		};
	};
} FILE;

extern FILE * const stdin;
extern FILE * const stdout;
extern FILE * const stderr;

int	__libc_putc_str(int c, FILE *f);
int	__libc_putc_nbf(int c, FILE *f);
int	__libc_putc_fbf(int c, FILE *f);
int	__libc_getc_nbf(FILE *f);
int	__libc_getc_fbf(FILE *f);

FILE *	_get_stdin(void);
FILE *	_get_stdout(void);
FILE *	_get_stderr(void);

int	printf(const char *format, ...);
int	fprintf(FILE *f, const char *format, ...);
int	sprintf(char *str, const char *format, ...);
int	snprintf(char *str, size_t n, const char *format, ...);
int	asprintf(char **str, const char *format, ...);

int	vprintf(const char *format, va_list ap);
int	vfprintf(FILE *f, const char *format, va_list ap);
int	vsprintf(char *str, const char *format, va_list ap);
int	vsnprintf(char *str, size_t size, const char *format, va_list ap);

int	rename(const char *name1, const char *name2);

int	remove(const char *pathname);

FILE *	fopen(const char *path, const char *mstr);
FILE *	fdopen(int fd, const char *mstr);
FILE *	freopen(const char *path, const char *mstr, FILE *f);
int	fclose(FILE *f);

void	clearerr(FILE *f);
int	feof(FILE *f);
int	ferror(FILE *f);
int	fileno(FILE *f);

int	fputc(int c, FILE *f);
int	putc(int c, FILE *f);
int	putchar(int c);
int	fgetc(FILE *f);
int	getc(FILE *f);
int	getchar(void);
int	ungetc(int ch, FILE *f);
size_t	fread(void *ptr, size_t size, size_t nmemb, FILE *f);
size_t	fwrite(const void *ptr, size_t size, size_t nmemb, FILE *f);
int	fflush(FILE *f);

int	fseek(FILE *f, long off, int whence);
long	ftell(FILE *f);
void	rewind(FILE *f);

char *	fgets(char *buf, int size, FILE *f);
int	fputs(const char *s, FILE *f);
int	puts(const char *s);

void	setbuf(FILE *f, char *buf);
void	setbuffer(FILE *f, char *buf, size_t size);
void	setlinebuf(FILE *f);
int	setvbuf(FILE *f, char *buf, int mode, size_t size);

void	perror(const char *s);

#define clearerr(f)	do { (f)->err = 0; (f)->eof = 0; } while (0)
#define feof(f)		((f)->eof)
#define ferror(f)	((f)->err)
#define fileno(f)	((f)->fd)

#define putc(c, f)	fputc(c, f)
#define getc(f)		fgetc(f)

#endif
