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

#include <kern/console.h>
#include <kern/printk.h>

#include <arch/stdarg.h>

#define ULONG_DIGITS	20
#define ULONG_BITS	64
#define UINT_BITS	32
#define USHORT_BITS	16
#define PTR_BITS	64

#define printk_uint(x)	printk_ulong(x)
#define printk_int(x)	printk_long(x)

static void printk_digit(unsigned value)
{
	if (value < 10)
		putchar('0' + value);
	else
		putchar('a' + value - 10);
}

static void printk_ulong(unsigned long value)
{
	unsigned long digit;
	int pzero = 0;
	int i, n;
	
	for (i = ULONG_DIGITS - 1; i >= 0; i--)
	{
		digit = value;
		for (n = 0; n < i; n++)
			digit /= 10;
		digit %= 10;
		if (pzero || digit || !i)
			putchar('0' + digit);
		if (digit)
			pzero = 1;
	}
}

static void printk_long(long value)
{
	if (value < 0)
	{
		putchar('-');
		printk_ulong(-value);
	}
	else
		printk_ulong(value);
}

static void printk_hex(unsigned long value, int bits)
{
	int i;
	
	for (i = 0; i < bits >> 2; i++)
	{
		printk_digit((value >> (bits - 4)) & 15);
		value <<= 4;
	}
}

static void printk_oct(unsigned long value, int bits)
{
	unsigned digit;
	int pzero = 0;
	int i, n;
	
	for (i = (bits + 2) / 3 - 1; i >= 0; i--)
	{
		digit = value;
		for (n = 0; n < i; n++)
			digit /= 8;
		digit %= 8;
		if (pzero || digit || !i)
			printk_digit(digit);
		if (digit)
			pzero = 1;
	}
}

static void printk_ptr(void *value)
{
	if (!value)
		cputs("(null)");
	else
	{
		cputs("0x");
		printk_hex((unsigned long)value, PTR_BITS);
	}
}

void printk(const char *format, ...)
{
	const char *f = format;
	va_list a;
	
	va_start(a, format);
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
			case 'h':
				f++;
				switch (*f)
				{
				case 'i':
					printk_long(va_arg(a, long));
					break;
				case 'u':
					printk_ulong(va_arg(a, unsigned long));
					break;
				case 'x':
					printk_hex(va_arg(a, unsigned long), USHORT_BITS);
					break;
				case 'o':
					printk_oct(va_arg(a, unsigned long), USHORT_BITS);
					break;
				}
				break;
			case 'l':
				f++;
				switch (*f)
				{
				case 'i':
					printk_long(va_arg(a, long));
					break;
				case 'u':
					printk_ulong(va_arg(a, unsigned long));
					break;
				case 'x':
					printk_hex(va_arg(a, unsigned long), ULONG_BITS);
					break;
				case 'o':
					printk_oct(va_arg(a, unsigned long), ULONG_BITS);
					break;
				}
				break;
			case 'i':
				printk_int(va_arg(a, int));
				break;
			case 'u':
				printk_uint(va_arg(a, unsigned));
				break;
			case 'p':
				printk_ptr(va_arg(a, void *));
				break;
			case 'x':
				printk_hex(va_arg(a, unsigned), UINT_BITS);
				break;
			case 'o':
				printk_oct(va_arg(a, unsigned), USHORT_BITS);
				break;
			case 's':
				cputs(va_arg(a, const char *));
				break;
			case 'c':
				putchar(va_arg(a, int));
				break;
			}
		}
		else
			putchar(*f);
		
		f++;
	}
	va_end(a);
}
