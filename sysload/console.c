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

#include <sysload/console.h>

void con_putn(int v)
{
	int z = 0;
	int i, n;
	int d;
	
	if (v < 0)
	{
		con_putc('-');
		v = -v;
	}
	
	for (i = 9; i >= 0; i--)
	{
		for (d = v, n = 0; n < i; n++)
			d /= 10;
		d %= 10;
		if (!d && !z && i)
			continue;
		con_putc('0' + d);
		z = 1;
	}
}

void con_putx32(int v)
{
	int i;
	
	for (i = 0; i < 8; i++, v <<= 4)
		con_putc("0123456789abcdef"[(v >> 28) & 15]);
}

void con_putx16(int v)
{
	int i;
	
	for (i = 0; i < 4; i++, v <<= 4)
		con_putc("0123456789abcdef"[(v >> 12) & 15]);
}

void con_putx8(int v)
{
	con_putc("0123456789abcdef"[(v >> 4) & 15]);
	con_putc("0123456789abcdef"[ v       & 15]);
}

void con_putx4(int v)
{
	con_putc("0123456789abcdef"[v & 15]);
}

void con_trace(const char *file, const char *func, int line)
{
	con_gotoxy(0, 23);
	con_puts(file);
	con_puts(": ");
	con_puts(func);
	con_puts(": ");
	con_putn(line);
	con_putc('\n');
}
