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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#define timezone	0

#define isleap(y)	(!((y) % 4) && (((y) % 100) || !(((y) + 300) % 400)))
#define yearsecs(y)	((365 + isleap((y))) * 86400)
#define monthdays(y, m)	((isleap((y)) && (m) == 1) ? __libc_monthlen[(m)] + 1 : __libc_monthlen[(m)])
#define monthsecs(y, m)	(monthdays((y), (m)) * 86400)

#define YEAR_MIN	-147
#define YEAR_MAX	8099

static char * const __libc_full_wday[7]=
{
	"Sunday",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday"
};

static char * const __libc_full_month[12]=
{
	"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December"
};

static char * const __libc_wday[7]=
{
	"Sun",
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat"
};

static char * const __libc_month[12]=
{
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"
};

static const int __libc_monthlen[12] = { 31, 28, 31, 30,  31,  30,  31,  31,  30,  31,  30,  31 };
static const int __libc_monthacc[12] = {  0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

void tzset(void)
{
	/* timezone  = 0;
	tzname[0] = "";
	tzname[1] = ""; */
}

char *asctime(const struct tm *time)
{
	static char buf[64];
	
	sprintf(buf, "%s %s %2i %02i:%02i:%02i %i\n",
		__libc_wday[time->tm_wday],
		__libc_month[time->tm_mon],
		time->tm_mday,
		time->tm_hour,
		time->tm_min,
		time->tm_sec,
		time->tm_year + 1900);
	
	return buf;
}

char *ctime(const time_t *time)
{
	const struct tm *tm;
	
	tm = localtime(time);
	if (!tm)
		return NULL;
	return asctime(tm);
}

struct tm *gmtime(const time_t *time)
{
	static struct tm tm;
	
	time_t t = *time;
	time_t t7;
	
	t7 = t % (86400 * 7);
	if (t7 < 0)
		t7 += 86400 * 7;
	
	tm.tm_wday = (t7 / 86400 + 4) % 7;
	tm.tm_sec  =  t7	 % 60;
	tm.tm_min  = (t7 / 60)	 % 60;
	tm.tm_hour = (t7 / 3600) % 24;
	tm.tm_year = 70;
	tm.tm_mon  = 0;
	
	while (t < 0)
	{
		t += yearsecs(tm.tm_year - 1);
		if (--tm.tm_year < YEAR_MIN) /* XXX */
			return NULL;
	}
	while (t >= yearsecs(tm.tm_year))
	{
		t -= yearsecs(tm.tm_year);
		if (++tm.tm_year > YEAR_MAX) /* XXX */
			return NULL;
	}
	tm.tm_yday = t / 86400;
	while (t >= monthsecs(tm.tm_year, tm.tm_mon))
	{
		t -= monthsecs(tm.tm_year, tm.tm_mon);
		tm.tm_mon++;
	}
	tm.tm_mday  = t / 86400 + 1;
	tm.tm_isdst = -1;
	return &tm;
}

struct tm *localtime(const time_t *time)
{
	time_t t;
	
	tzset();
	t = timezone + *time;
	return gmtime(&t);
}

static time_t divrd(time_t a, time_t b)
{
	if (a < 0)
		return (a - b + 1) / b;
	return a / b;
}

time_t mktime(const struct tm *time)
{
	struct tm tm;
	time_t t;
	int ly;
	
	if (time->tm_year < YEAR_MIN || time->tm_mon < 0 || time->tm_mday < 0 ||
	    time->tm_hour < 0	     || time->tm_min < 0 || time->tm_sec  < 0)
		return -1;
	tzset();
	tm = *time;
	tm.tm_year += tm.tm_mon / 12;
	tm.tm_mon  %= 12;
	t  = (tm.tm_mday - 1) * 86400;
	t +=  tm.tm_hour * 3600;
	t +=  tm.tm_min * 60;
	t +=  tm.tm_sec;
	t += __libc_monthacc[tm.tm_mon] * 86400;
	
	ly = tm.tm_year + (tm.tm_mon > 1);
	t += 86400 * 365 * (time_t)(tm.tm_year - 70);
	t += 86400 * divrd(ly - 69, 4);
	t -= 86400 * divrd(ly - 1,  100);
	t += 86400 * divrd(ly + 299, 400);
	
	return t - timezone;
}

time_t time(time_t *t)
{
	struct timeval tv;
	
	if (gettimeofday(&tv, NULL))
		return -1;
	if (t)
		*t = tv.tv_sec;
	return tv.tv_sec;
}

int stime(const time_t *t)
{
	struct timeval tv;
	
	tv.tv_sec  = *t;
	tv.tv_usec = 0;
	return settimeofday(&tv, NULL);
}

static int clamp(int v, int min, int max)
{
	if (v < min)
		return min;
	if (v > max)
		return max;
	return v;
}

void cltime(struct tm *tm)
{
	if (tm->tm_year > YEAR_MAX)
	{
		tm->tm_year = YEAR_MAX;
		tm->tm_mon  = 11;
		tm->tm_mday = monthdays(tm->tm_year, tm->tm_mon);
		tm->tm_hour = 23;
		tm->tm_min  = 59;
		tm->tm_sec  = 59;
		return;
	}
	else if (tm->tm_year < YEAR_MIN)
	{
		memset(tm, 0, sizeof *tm);
		tm->tm_year = YEAR_MIN;
		tm->tm_mday = 1;
		return;
	}
	
	if (tm->tm_mon > 11)
	{
		tm->tm_mon  = 11;
		tm->tm_mday = monthdays(tm->tm_year, tm->tm_mon);
		tm->tm_hour = 23;
		tm->tm_min  = 59;
		tm->tm_sec  = 59;
		return;
	}
	if (tm->tm_mon < 0)
	{
		tm->tm_mon  = 0;
		tm->tm_mday = 1;
		tm->tm_hour = 0;
		tm->tm_min  = 0;
		tm->tm_sec  = 0;
		return;
	}
	
	if (tm->tm_mday > monthdays(tm->tm_year, tm->tm_mon))
	{
		tm->tm_mday = monthdays(tm->tm_year, tm->tm_mon);
		tm->tm_hour = 23;
		tm->tm_min  = 59;
		tm->tm_sec  = 59;
		return;
	}
	if (tm->tm_mday < 1)
	{
		tm->tm_mday = 1;
		tm->tm_hour = 0;
		tm->tm_min  = 0;
		tm->tm_sec  = 0;
		return;
	}
	
	if (tm->tm_hour > 23)
	{
		tm->tm_hour = 23;
		tm->tm_min  = 59;
		tm->tm_sec  = 59;
		return;
	}
	if (tm->tm_hour < 0)
	{
		tm->tm_hour = 0;
		tm->tm_min  = 0;
		tm->tm_sec  = 0;
		return;
	}
	
	if (tm->tm_min > 59)
	{
		tm->tm_min  = 59;
		tm->tm_sec  = 59;
		return;
	}
	if (tm->tm_min < 0)
	{
		tm->tm_min  = 0;
		tm->tm_sec  = 0;
		return;
	}
	
	if (tm->tm_sec > 59)
		tm->tm_sec  = 59;
	else if (tm->tm_sec < 0)
		tm->tm_sec  = 0;
}

double difftime(time_t t1, time_t t0)
{
	return (double)t1 - (double)t0;
}

clock_t clock(void)
{
	_set_errno(ENOSYS);
	return -1;
}

size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tm)
{
#define PUTC(c)	do				\
		{				\
			if (++cnt < max)	\
				*p++ = (c);	\
			else			\
				return 0;	\
		} while (0)

#define PUTDNZ(n) do					\
		  {					\
			if (n)				\
				PUTC('0' + (n));	\
			else				\
				PUTC(' ');		\
		  } while(0)

#define PUTD(n)	PUTC('0' + (n))

#define PUTS(s)	do				\
		{				\
			char *_p = (s);		\
						\
			while (*_p)		\
				PUTC(*_p++);	\
		} while (0)

#define LOCALE_DATE	do						\
			{						\
				PUTD((tm->tm_year + 1900) / 1000 % 10);	\
				PUTD((tm->tm_year + 1900) / 100  % 10);	\
				PUTD((tm->tm_year + 1900) / 10	 % 10);	\
				PUTD((tm->tm_year + 1900)	 % 10);	\
				PUTC('-');				\
				PUTD((tm->tm_mon  + 1) / 10);		\
				PUTD((tm->tm_mon  + 1) % 10);		\
				PUTC('-');				\
				PUTD((tm->tm_mday) / 10);		\
				PUTD((tm->tm_mday) % 10);		\
			} while (0)

#define LOCALE_TIME	do				\
			{				\
				PUTD(tm->tm_hour / 10);	\
				PUTD(tm->tm_hour % 10);	\
				PUTC(':');		\
				PUTD(tm->tm_min / 10);	\
				PUTD(tm->tm_min % 10);	\
				PUTC(':');		\
				PUTD(tm->tm_sec / 10);	\
				PUTD(tm->tm_sec % 10);	\
			} while (0)

#define WEEK_FUNC	PUTS("??"); /* XXX */

	char buf[64]; /* XXX */
	char *p = s;
	size_t cnt = 0;
	int h12;
	
	while (*fmt)
	{
		if (*fmt == '%')
		{
alt:
			switch (*++fmt)
			{
			case 'O':
			case 'E':
				goto alt;
			case '%':
				PUTC('%');
				break;
			
			case 'A':
				PUTS(__libc_full_wday[tm->tm_wday]);
				break;
			case 'a':
				PUTS(__libc_wday[tm->tm_wday]);
				break;
			case 'B':
				PUTS(__libc_full_month[tm->tm_mon]);
				break;
			case 'b':
			case 'h':
				PUTS(__libc_month[tm->tm_mon]);
				break;
			case 'c':
				LOCALE_DATE;
				PUTC(' ');
				LOCALE_TIME;
				break;
			case 'C':
				PUTD((tm->tm_year + 1900) / 1000 % 10);
				PUTD((tm->tm_year + 1900) / 100  % 10);
				break;
			case 'd':
				PUTD((tm->tm_mday) / 10);
				PUTD((tm->tm_mday) % 10);
				break;
			case 'D': /* XXX locale */
				PUTD((tm->tm_mon + 1) / 10);
				PUTD((tm->tm_mon + 1) % 10);
				PUTC('/');
				PUTD((tm->tm_mday) / 10);
				PUTD((tm->tm_mday) % 10);
				PUTC('/');
				PUTD((tm->tm_year + 1900) / 10 % 10);
				PUTD((tm->tm_year + 1900)      % 10);
				break;
			case 'e':
				PUTDNZ((tm->tm_mday) / 10);
				PUTD  ((tm->tm_mday) % 10);
				break;
			case 'F':
				PUTD((tm->tm_year + 1900) / 1000 % 10);
				PUTD((tm->tm_year + 1900) / 100  % 10);
				PUTD((tm->tm_year + 1900) / 10	 % 10);
				PUTD((tm->tm_year + 1900)	 % 10);
				PUTC('-');
				PUTD((tm->tm_mon  + 1) / 10);
				PUTD((tm->tm_mon  + 1) % 10);
				PUTC('-');
				PUTD((tm->tm_mday) / 10);
				PUTD((tm->tm_mday) % 10);
				break;
			case 'G':
				PUTD((tm->tm_year + 1900) / 1000 % 10);
				PUTD((tm->tm_year + 1900) / 100  % 10);
				PUTD((tm->tm_year + 1900) / 10	 % 10);
				PUTD((tm->tm_year + 1900)	 % 10);
				break;
			case 'g':
				PUTD((tm->tm_year + 1900) / 10	 % 10);
				PUTD((tm->tm_year + 1900)	 % 10);
				break;
			case 'H':
				PUTD(tm->tm_hour / 10);
				PUTD(tm->tm_hour % 10);
				break;
			case 'I':
				h12 = (tm->tm_hour + 11) % 12 + 1;
				
				PUTD(h12 / 10);
				PUTD(h12 % 10);
				break;
			case 'j':
				PUTD((tm->tm_yday + 1) / 100);
				PUTD((tm->tm_yday + 1) / 10 % 10);
				PUTD((tm->tm_yday + 1)	    % 10);
				break;
			case 'k':
				PUTDNZ(tm->tm_hour / 10);
				PUTD  (tm->tm_hour % 10);
				break;
			case 'l':
				h12 = (tm->tm_hour + 11) % 12 + 1;
				
				PUTDNZ(h12 / 10);
				PUTD  (h12 % 10);
				break;
			case 'm':
				PUTD((tm->tm_mon + 1) / 10);
				PUTD((tm->tm_mon + 1) % 10);
				break;
			case 'M':
				PUTD(tm->tm_min / 10);
				PUTD(tm->tm_min % 10);
				break;
			case 'n':
				PUTC('\n');
				break;
			case 'p':
				if (tm->tm_hour < 12)
					PUTS("AM");
				else
					PUTS("PM");
				break;
			case 'P': /* XXX locale */
				if (tm->tm_hour < 12)
					PUTS("am");
				else
					PUTS("pm");
				break;
			case 'r':
				h12 = (tm->tm_hour + 11) % 12 + 1;
				
				PUTD(h12 / 10);
				PUTD(h12 % 10);
				PUTC(':');
				PUTD(tm->tm_min / 10);
				PUTD(tm->tm_min % 10);
				PUTC(':');
				PUTD(tm->tm_sec / 10);
				PUTD(tm->tm_sec % 10);
				PUTC(' ');
				
				if (tm->tm_hour < 12)
					PUTS("AM");
				else
					PUTS("PM");
				break;
			case 'R':
				PUTD(tm->tm_hour / 10);
				PUTD(tm->tm_hour % 10);
				PUTC(':');
				PUTD(tm->tm_min / 10);
				PUTD(tm->tm_min % 10);
				break;
			case 's':
				sprintf(buf, "%ji", (intmax_t)mktime(tm));
				PUTS(buf);
				break;
			case 'S':
				PUTD(tm->tm_sec / 10);
				PUTD(tm->tm_sec % 10);
				break;
			case 't':
				PUTC('\t');
				break;
			case 'T':
				PUTD(tm->tm_hour / 10);
				PUTD(tm->tm_hour % 10);
				PUTC(':');
				PUTD(tm->tm_min / 10);
				PUTD(tm->tm_min % 10);
				PUTC(':');
				PUTD(tm->tm_sec / 10);
				PUTD(tm->tm_sec % 10);
				break;
			case 'u':
				PUTC('1' + (tm->tm_wday + 6) % 7);
				break;
			case 'U':
				WEEK_FUNC;
				break;
			case 'V':
				WEEK_FUNC;
				break;
			case 'w':
				PUTC('0' + tm->tm_wday);
				break;
			case 'W':
				WEEK_FUNC;
				break;
			case 'x':
				LOCALE_DATE;
				break;
			case 'X':
				LOCALE_TIME;
				break;
			case 'y':
				PUTD((tm->tm_year + 1900) / 10 % 10);
				PUTD((tm->tm_year + 1900)      % 10);
				break;
			case 'Y':
				PUTD((tm->tm_year + 1900) / 1000 % 10);
				PUTD((tm->tm_year + 1900) / 100  % 10);
				PUTD((tm->tm_year + 1900) / 10	 % 10);
				PUTD((tm->tm_year + 1900)	 % 10);
				break;
			case 'z': /* XXX timezone */
				PUTC('+');
				PUTC('0');
				PUTC('0');
				break;
			case 'Z': /* XXX timezone */
				PUTC('G');
				PUTC('M');
				PUTC('T');
				break;
			default:
				PUTC(*fmt);
			}
		}
		else
			PUTC(*fmt);
		fmt++;
	}
	
	*p = 0;
#undef PUTC
#undef PUTS
}
