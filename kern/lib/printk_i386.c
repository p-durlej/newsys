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

#include <kern/console.h>
#include <kern/printk.h>
#include <kern/errno.h>
#include <kern/lib.h>

static void printk_digit(unsigned value)
{
	if (value < 10)
		putchar('0' + value);
	else
		putchar('a' + value - 10);
}

static void printk_uint(unsigned value)
{
	unsigned digit;
	int pzero = 0;
	int i, n;
	
	for (i = 20; i >= 0; i--)
	{
		digit = value;
		for (n = 0; n < i; n++)
			digit /= 10;
		digit %= 10;
		if (pzero || digit || !i)
			printk_digit(digit);
		if (digit)
			pzero = 1;
	}
}

static void printk_int(int value)
{
	if (value < 0)
	{
		putchar('-');
		printk_uint(-value);
	}
	else
		printk_uint(value);
}

static void printk_hex(unsigned value)
{
	printk_digit((value & 0xf0000000) >> 28);
	printk_digit((value & 0xf000000) >> 24);
	printk_digit((value & 0xf00000) >> 20);
	printk_digit((value & 0xf0000) >> 16);
	printk_digit((value & 0xf000) >> 12);
	printk_digit((value & 0xf00) >> 8);
	printk_digit((value & 0xf0) >> 4);
	printk_digit((value & 0xf));
}

static void printk_oct(unsigned value)
{
	unsigned digit;
	int pzero = 0;
	int i, n;
	
	for (i = 20; i >= 0; i--)
	{
		digit = value;
		for (n = 0; n < i; n++)
			digit /= 8;
		digit %= 8;
		if (pzero || digit || !i)
			printk_digit(digit);
		if (digit)
			pzero=1;
	}
}

static void printk_ptr(unsigned value)
{
	if (!value)
		cputs("(null)");
	else
	{
		cputs("0x");
		printk_hex(value);
	}
}

void printk(const char *format, ...)
{
	unsigned *arg = (unsigned *)&format + 1;
	const char *f = format;
	
	while (*f)
	{
		if (*f == '%')
		{
			f++;
			
			switch (*f)
			{
			case 0:
				putchar('%');
				return;
			case '%':
				putchar('%');
				break;
			case 'l':
				f++;
				switch (*f)
				{
				case 'i':
					printk_int(*(arg++));
					break;
				case 'u':
					printk_uint(*(arg++));
					break;
				}
				break;
			case 'i':
				printk_int(*(arg++));
				break;
			case 'u':
				printk_uint(*(arg++));
				break;
			case 'p':
				printk_ptr(*(arg++));
				break;
			case 'x':
				printk_hex(*(arg++));
				break;
			case 'o':
				printk_oct(*(arg++));
				break;
			case 's':
				cputs((void *)*(arg++));
				break;
			case 'c':
				putchar((char)*(arg++));
				break;
			}
		}
		else
			putchar(*f);
		
		f++;
	}
}
