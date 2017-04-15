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

#include <arch/archdef.h>

#include <priv/libc.h>

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#define SIZE_HH		-2
#define SIZE_H		-1
#define SIZE_DEFAULT	0
#define SIZE_L		1
#define SIZE_LL		2
#define SIZE_MAX	3
#define SIZE_PTRDIFF	4
#define SIZE_PTR	5

struct format
{
	int alt;
	int sign;
	int zero;
	int left;
	int spc;
	
	int signedval;
	int precision;
	int width;
	int _size;
	int base;
};

static int putn(uintmax_t v, int base, FILE *f)
{
	int z = 0;
	int i;
	
	for (i = 32; i >= 0; i--)
	{
		uintmax_t digit;
		char ch;
		int c;
		
		digit = v;
		c     = i;
		while (c--)
			digit /= (uintmax_t)base;
		digit %= (uintmax_t)base;
		
		if (!digit && !z)
			continue;
		
		if (digit <= 9)
			ch = '0' + digit;
		else
			ch = 'a' + digit - 10;
		
		if (fputc(ch, f) == EOF)
			return -1;
		z = 1;
	}
	return 0;
}

static int nr_digits(uintmax_t v, int base)
{
	int cnt;
	
	for (cnt = 0; v; cnt++)
		v /= base;
	return cnt;
}

static int fmtputs(FILE *f, struct format *fmt, const char *str)
{
	int nspc = 0;
	int len;
	
	if (!str)
		str = "(null)";
	
	len = strlen(str);
	if (fmt->precision > 0 && len > fmt->precision)
		len = fmt->precision;
	
	if (len < fmt->width)
		nspc = fmt->width - len;
	
	if (!fmt->left)
		while(nspc--)
			if (fputc(' ', f) == EOF)
				return -1;
	while (len--)
		if (fputc(*str++, f) == EOF)
			return -1;
	
	if (fmt->left)
		while(nspc--)
			if (fputc(' ', f) == EOF)
				return -1;
	return 0;
}

static int fmtputn(FILE *f, struct format *fmt, uintmax_t v, int neg)
{
	char sgn = 0;
	int pcnt = 0;
	int prec = 1;
	int dcnt;
	
	if (fmt->precision >= 0)
		prec = fmt->precision;
	
	if (neg)
		sgn = '-';
	else
	{
		if (fmt->sign)
			sgn = '+';
		else if (fmt->spc)
			sgn = ' ';
	}
	
	dcnt = nr_digits(v, fmt->base);
	pcnt = fmt->width - dcnt;
	if (prec > dcnt)
		pcnt = fmt->width - prec;
	if (sgn)
		pcnt--;
	if (pcnt < 0)
		pcnt = 0;
	
	if (!fmt->left && !fmt->zero)
		while (pcnt--)
			if (fputc(' ', f) == EOF)
				return -1;
	if (sgn)
		if (fputc(sgn, f) == EOF)
			return -1;
	if (!fmt->left && fmt->zero)
		while (pcnt--)
			if (fputc('0', f) == EOF)
				return -1;
	
	while (prec > dcnt)
	{
		fputc('0', f);
		dcnt++;
	}
	
	if (putn(v, fmt->base, f))
		return -1;
	
	if (fmt->left)
		while (pcnt--)
			if (fputc(' ', f) == EOF)
				return -1;
	return 0;
}

int vfprintf(FILE *f, const char *format, va_list ap)
{
	struct format fmt;
	char ch;
	int num;
	
	while (*format)
	{
		if (*format == '%')
		{
			format++;
			
			if (*format == '%')
			{
				if (fputc('%', f) == EOF)
					return -1;
				format++;
				continue;
			}
			
			fmt.spc		= 0;
			fmt.alt		= 0;
			fmt.sign	= 0;
			fmt.zero	= 0;
			fmt.precision	= -1;
			fmt.width	= 0;
			fmt.left	= 0;
			fmt._size	= SIZE_DEFAULT;
			fmt.signedval	= 1;
			
			num = 0;
			
			while ((ch = *format))
			{
				switch (ch)
				{
				case '0':
					fmt.zero = 1;
					break;
				case '-':
					fmt.left = 1;
					break;
				case '#':
					fmt.alt = 1;
					break;
				case ' ':
					fmt.sign = 0;
					fmt.spc	 = 1;
					break;
				case '+':
					fmt.sign = 1;
					fmt.spc	 = 0;
					break;
				default:
					goto getwidth;
				}
				
				format++;
			}
			
getwidth:
			while (isdigit(*format))
			{
				fmt.width *= 10;
				fmt.width += *format - '0';
				format++;
			}
			
			if (*format == '.')
			{
				format++;
				
				fmt.precision = 0;
				if (*format == '-')
				{
					format++;
					while (isdigit(*format))
						format++;
				}
				else
					while (isdigit(*format))
					{
						fmt.precision *= 10;
						fmt.precision += *format - '0';
						format++;
					}
			}
			
			while ((ch = *format))
			{
				switch (ch)
				{
				case 'L': /* for LF */
				case 'l':
					fmt._size++;
					break;
				case 'h':
					fmt._size--;
					break;
				case 't':
					fmt._size = SIZE_PTRDIFF;
					break;
				case 'j':
					fmt._size = SIZE_MAX;
					break;
				default:
					goto convert;
				}
				format++;
			}
			
convert:
			switch (*format)
			{
			case 'm':
				if (fmtputs(f, &fmt, strerror(_get_errno())))
					return -1;
				break;
			case 's':
				if (fmtputs(f, &fmt, va_arg(ap, char *)))
					return -1;
				break;
			case 'c':
				if (fputc(va_arg(ap, int), f) == EOF)
					return -1;
				break;
			case 'd':
			case 'i':
				fmt.base	= 10;
				fmt.signedval	= 1;
				num		= 1;
				break;
			case 'u':
				fmt.base	= 10;
				fmt.signedval	= 0;
				num		= 1;
				break;
			case 'o':
				fmt.base	= 8;
				fmt.signedval	= 0;
				num		= 1;
				break;
			case 'p': /* XXX */
				fmt._size	= SIZE_PTR;
			case 'x':
			case 'X':
				fmt.base	= 16;
				fmt.signedval	= 0;
				num		= 1;
				break;
			default:
				if (fputc('%', f) == EOF)
					return -1;
				if (fputc(*format, f) == EOF)
					return -1;
			}
			
			if (num)
			{
				uintmax_t val;
				int neg = 0;
				
				if (fmt.signedval)
				{
					intmax_t arg;
					
					switch (fmt._size)
					{
					case SIZE_HH:
						arg = va_arg(ap, int);
						break;
					case SIZE_H:
						arg = va_arg(ap, int);
						break;
					case SIZE_DEFAULT:
						arg = va_arg(ap, int);
						break;
					case SIZE_L:
						arg = va_arg(ap, long);
						break;
					case SIZE_LL:
						arg = va_arg(ap, long long);
						break;
					case SIZE_MAX:
						arg = va_arg(ap, intmax_t);
						break;
					case SIZE_PTRDIFF:
						arg = va_arg(ap, ptrdiff_t);
						break;
					case SIZE_PTR:
						arg = (intptr_t)va_arg(ap, void *);
						break;
					default:
						abort();
					}
					
					if (arg < 0)
					{
						val = -arg;
						neg = 1;
					}
					else
						val = arg;
				}
				else
					switch (fmt._size)
					{
					case SIZE_HH:
						val = va_arg(ap, unsigned int);
						break;
					case SIZE_H:
						val = va_arg(ap, unsigned int);
						break;
					case SIZE_DEFAULT:
						val = va_arg(ap, unsigned int);
						break;
					case SIZE_L:
						val = va_arg(ap, unsigned long);
						break;
					case SIZE_LL:
						val = va_arg(ap, unsigned long long);
						break;
					case SIZE_MAX:
						val = va_arg(ap, uintmax_t);
						break;
					case SIZE_PTRDIFF:
						val = va_arg(ap, ptrdiff_t);
						break;
					case SIZE_PTR:
						val = (uintptr_t)va_arg(ap, void *);
						break;
					default:
						abort();
					}
				
				if (fmtputn(f, &fmt, val, neg))
					return -1;
			}
		}
		else
			if (fputc(*format, f) == EOF)
				return -1;
		
		format++;
	}
	return 0;
}
