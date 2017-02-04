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

#ifndef _KERN_CONSOLE_H
#define _KERN_CONSOLE_H

#ifndef NULL
#define NULL (void *)0
#endif

#define CONSOLE_BUF	8192

#define CSTATUS_ERROR	1
#define CSTATUS_PANIC	2
#define CSTATUS_HALT	4

struct console
{
	int	width, height;
	int	flags;
	
	void	(*disable)(struct console *con);
	void	(*reset)(struct console *con);
	void	(*clear)(struct console *con);
	void	(*putc)(struct console *con, int c);
	void	(*status)(struct console *con, int flags);
	void	(*gotoxy)(struct console *con, int x, int y);
	void	(*puta)(struct console *con, int x, int y, int c, int a);
};

extern struct console *con_con;

extern char con_buf[CONSOLE_BUF]; /* XXX PC */
extern int  con_ptr;
extern int  vga_con;

void bootcon_init(void);

void con_install(struct console *con);

void con_disable(void);
void con_mute(void);
void con_reset(void);
void con_clear(void);
void putchar(char ch);
void cputs(const char *msg);

void con_sstatus(int flags);
void con_cstatus(int flags);

void panic(const char *msg) __attribute__((noreturn));
void backtrace(void *bp0);
int  sysdump(void);

#endif
