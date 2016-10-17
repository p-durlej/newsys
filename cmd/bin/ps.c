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

#include <sys/signal.h>
#include <systat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <os386.h>
#include <stdio.h>
#include <errno.h>

static struct pty_size	pty_size;

static struct taskinfo *task;
static int		task_max;
static int		task_cnt;
static int		cpu_max;

static int		qflag;
static int		eflag;
static int		bigP;

static void gptysize(void)
{
	if (ioctl(0, PTY_GSIZE, &pty_size))
	{
		pty_size.w = 80;
		pty_size.h = 24;
	}
}

static void pcpu(int cpu)
{
	if (cpu_max)
		printf("%3u%% ", 100L * cpu / cpu_max);
	else
		printf("%4u ", cpu);
}

static void ptasks(int max)
{
	char *p;
	int len;
	int i;
	
	printf("PID   PPID  EUID  EGID  RUID  RGID  SIZE   CPU ");
	if (qflag)
		printf("QUEUE     ");
	if (eflag)
		printf("MAXEV ");
	if (bigP)
		printf("PRIO ");
	printf("COMM\n");
	
	for (i = 0; i < task_cnt && max; i++)
	{
		if (!task[i].pid)
			continue;
		max--;
		
		len = pty_size.w - 47;
		
		printf("%-5i ", (int)task[i].pid);
		printf("%-5i ", (int)task[i].ppid);
		printf("%-5i ", (int)task[i].euid);
		printf("%-5i ", (int)task[i].egid);
		printf("%-5i ", (int)task[i].ruid);
		printf("%-5i ", (int)task[i].rgid);
		printf("%-5lli ", (long long)task[i].size / 1024);
		pcpu(task[i].cpu);
		
		if (qflag)
		{
			printf("%-9s ", task[i].queue);
			len -= 10;
		}
		
		if (eflag)
		{
			printf("%-5u ", task[i].maxev);
			len -= 6;
		}
		
		if (bigP)
		{
			printf("%-4i ", task[i].prio);
			len -= 5;
		}
		
		p = task[i].pathname;
		while (*p && len-- > 0)
		{
			if (*p >= 32 && *p <= 127)
				fputc(*p, stdout);
			else
				fputc('.', stdout);
			p++;
		}
		fputc('\n', stdout);
	}
}

static void phead(void)
{
	struct systat st;
	
	if (_systat(&st))
		return;
	
	printf("files %7ju  used %7ju  avail %7ju  total\n",
		(uintmax_t)(st.file_max - st.file_avail),
		(uintmax_t) st.file_avail,
		(uintmax_t) st.file_max);
	
	printf("fsobs %7ju  used %7ju  avail %7ju  total\n",
		(uintmax_t)(st.fso_max - st.fso_avail),
		(uintmax_t) st.fso_avail,
		(uintmax_t) st.fso_max);
	
	printf("tasks %7ju  used %7ju  avail %7ju  total\n",
		(uintmax_t)(st.task_max - st.task_avail),
		(uintmax_t) st.task_avail,
		(uintmax_t) st.task_max);
	
	printf(" core %7juK used %7juK avail %7juK total\n\n",
		(uintmax_t)(st.core_max - st.core_avail) / 1024,
		(uintmax_t) st.core_avail		 / 1024,
		(uintmax_t) st.core_max			 / 1024);
}

static void sig_int(int nr)
{
#define CLEAR	"\033[H\033[J"
	signal(SIGINT, sig_int);
	write(0, CLEAR, sizeof CLEAR - 1);
	_exit(0);
#undef CLEAR
}

static int tcmp(const void *p1, const void *p2)
{
	const struct taskinfo *t1 = p1;
	const struct taskinfo *t2 = p2;
	
	return t2->cpu - t1->cpu;
}

static void top(void)
{
	signal(SIGINT, sig_int);
	
	for (;;)
	{
		task_cnt = _taskinfo(task);
		
		qsort(task, task_cnt, sizeof *task, tcmp);
		
		gptysize();
		
		printf("\033[H\033[J");
		phead();
		ptasks(pty_size.h - 7);
		sleep(1);
	}
}

static void usage(void)
{
	printf("Usage: top [-eqP]\n"
	       "       ps [-eqP]\n"
	       "Task information.\n\n"
	       "  -e show print event high water values\n"
	       "  -q show queue information\n"
	       "  -P show task priorities\n\n");
}

int main(int argc, char **argv)
{
	struct systat st;
	char *p;
	int c;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		usage();
		return 0;
	}
	
	while (c = getopt(argc, argv, "eqP"), c > 0)
		switch (c)
		{
		case 'q':
			qflag = 1;
			break;
		case 'e':
			eflag = 1;
			break;
		case 'P':
			bigP = 1;
			break;
		default:
			return 1;
		}
	
	if (!_systat(&st))
		cpu_max = st.cpu_max;
	
	task_max = _taskmax();
	task = malloc(sizeof(struct taskinfo) * task_max);
	if (!task)
	{
		perror("ps: malloc");
		return errno;
	}
	task_cnt = _taskinfo(task);
	
	p = strrchr(argv[0], '/');
	if (p == NULL)
		p = argv[0];
	else
		p++;
	
	if (!strcmp(p, "top"))
		top();
	else
	{
		gptysize();
		ptasks(INT_MAX);
	}
	
	return 0;
}
