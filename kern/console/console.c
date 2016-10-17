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

#include <priv/copyright.h>

#include <kern/console.h>
#include <kern/config.h>
#include <kern/printk.h>
#include <kern/wingui.h>
#include <kern/intr.h>
#include <kern/task.h>

struct console *con_con;

char con_buf[CONSOLE_BUF];
int  con_ptr;

static int con_disabled	= 0;
static int con_muted	= 0;
static int con_status	= 0;

void putchar(char ch)
{
#if CONSOLE_E9
	outb(CONSOLE_E9_PORT, ch);
#endif
	con_buf[con_ptr++] = ch;
	con_ptr %= CONSOLE_BUF;
	
	if (con_con != NULL && !con_muted)
		con_con->putc(con_con, ch);
}

void cputs(const char *msg)
{
	while (*msg)
		putchar(*(msg++));
}

void con_install(struct console *con)
{
	if (con_con != NULL)
		con_con->disable(con_con);
	
	con_con = con;
	con_reset();
}

void con_disable(void)
{
	con_disabled = 1;
	con_muted = 1;
	
	if (con_con != NULL)
		con_con->disable(con_con);
}

void con_mute(void)
{
	con_muted = 1;
}

void con_reset(void)
{
	int i;
	
	if (con_con == NULL)
		return;
	
	con_con->reset(con_con);
	con_disabled = 0;
	con_muted = 0;
	
	for (i = con_ptr; i < sizeof con_buf; i++)
		if (con_buf[i])
			con_con->putc(con_con, con_buf[i]);

	for (i = 0; i < con_ptr; i++)
		if (con_buf[i])
			con_con->putc(con_con, con_buf[i]);
}

void con_clear(void)
{
	if (con_con != NULL && !con_muted)
		con_con->clear(con_con);
}

void con_sstatus(int flags)
{
	int s;
	
	s = intr_dis();
	con_status |= flags;
	if (con_con != NULL)
		con_con->status(con_con, con_status);
	intr_res(s);
}

void con_cstatus(int flags)
{
	int s;
	
	s = intr_dis();
	con_status &= ~flags;
	if (con_con != NULL)
		con_con->status(con_con, con_status);
	intr_res(s);
}
