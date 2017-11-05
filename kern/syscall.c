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

#include <sysload/kparam.h>

#include <arch/archdef.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <bioctl.h>
#include <event.h>
#include <time.h>

#include <kern/machine/machine.h>
#include <kern/start.h>
#include <kern/shutdown.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/config.h>
#include <kern/module.h>
#include <kern/signal.h>
#include <kern/printk.h>
#include <kern/block.h>
#include <kern/errno.h>
#include <kern/sched.h>
#include <kern/clock.h>
#include <kern/page.h>
#include <kern/umem.h>
#include <kern/intr.h>
#include <kern/task.h>
#include <kern/fork.h>
#include <kern/lib.h>
#include <kern/cpu.h>
#include <kern/fs.h>

#include "syslist/systab.h"

void malloc_dump(void);

int sys__sysmesg(const char *msg)
{
	int err;
	
	if (!msg)
	{
		cputs("(null)");
		return 0;
	}
	
	err = usa(&msg, 1024, ERANGE);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	cputs(msg);
	return 0;
}

int sys__panic(const char *msg)
{
	int err;
	
	if (!msg)
	{
		panic("(null)");
		return 0;
	}
	
	err = usa(&msg, 1024, ERANGE);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	panic(msg);
	return 0;
}

static int sys__debug_ptasks(void)
{
	const struct task *t;
	const char *p, *e;
	int i;
	
	for (i = 0; i < TASK_MAX; i++)
	{
		t = task[i];
		
		if (t == NULL)
			continue;
		
		e = t->k_stack + sizeof t->k_stack;
		p = t->k_stack;
		
		while (p < e && *p == 0x55555555) // XXX
			p++;
		
		printk("i = %i, pid = %i, exec_name = \"%s\", "
		       "stack: %i\n",
		       i, (int)t->pid, t->exec_name, (int)(e - p));
	}
	return 0;
}

int sys__debug(int cmd)
{
	printk("sys__debug: cmd = %i\n", cmd);
	
	switch (cmd)
	{
	case DEBUG_PTASKS:
		return sys__debug_ptasks();
	case DEBUG_MDUMP:
		malloc_dump();
		return 0;
	case DEBUG_BLOCK:
		blk_dump();
		return 0;
	default:
		uerr(ENOSYS);
		return -1;
	}
}

#if defined __ARCH_I386__
int sys__iopl(int level)
{
	if (level < 0 || level > 3)
	{
		uerr(EINVAL);
		return -1;
	}
	
	ureg->eflags &= 0xffffcfff;
	ureg->eflags |= level << 12;
	return 0;
}
#elif defined __ARCH_AMD64__
int sys__iopl(int level)
{
	if (level < 0 || level > 3)
	{
		uerr(EINVAL);
		return -1;
	}
	
	ureg->rflags &= 0xffffffffffffcfff;
	ureg->rflags |= level << 12;
	return 0;
}
#else
int sys__iopl(int level)
{
	uerr(ENOSYS);
	return -1;
}
#endif

int sys__sync1(int flags)
{
	int err;
	
	err = fs_syncall();
	if (err)
	{
		blk_syncall(flags);
		uerr(err);
		return -1;
	}
	err = blk_syncall(flags);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys__blk_add(int count)
{
	int err;

	err = blk_add(count);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys__boot_flags(void)
{
	return kparam.boot_flags;
}

int sys__set_errno(int errno)
{
	tucpy(curr->errno, &errno, sizeof errno);
	return 0;
}

int sys__get_errno(void)
{
	int errno;
	
	fucpy(&errno, curr->errno, sizeof errno);
	return errno;
}

int sys__set_errno_ptr(int *errno_p)
{
	curr->errno = errno_p;
	return 0;
}

void *sys__get_errno_ptr()
{
	return curr->errno;
}

int sys__set_signal_entry(void *proc)
{
	curr->signal_entry = proc;
	return 0;
}

pid_t sys_fork(void)
{
	pid_t pid;
	int err;

	err = do_fork(&pid);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return pid;
}

int sys__exit(int status)
{
	task_exit((status & 0xff) << 8);
	return 0;
}

int sys__exec(char *pathname, char *arg, int arg_size)
{
	char p[PATH_MAX];
	char a[ARG_MAX]; /* XXX large */
	int err;
	
	if (arg_size > ARG_MAX)
	{
		uerr(E2BIG);
		return -1;
	}
	
	err = fustr(p, pathname, sizeof p);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = fucpy(a, arg, arg_size);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = task_exec(p, a, arg_size);
	if (err)
	{
		curr->signal_handler[SIGBUS - 1] = SIG_DFL;
		signal_raise(SIGBUS);
		uerr(err);
		return -1;
	}
	
	return 0; /* current process will be replaced */
}

int sys__newtask(const char *pathname, char *arg, int arg_size)
{
	pid_t pid;
	int err;
	
	if (arg_size < 0 || arg_size > ARG_MAX)
	{
		uerr(E2BIG);
		return -1;
	}
	
	err = usa(&pathname, PATH_MAX - 1, ENAMETOOLONG);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = uga(&arg, arg_size, UA_READ);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = task_creat(pathname, arg, arg_size, &pid);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return pid;
}

int sys_kill(pid_t pid, sig_t sig)
{
	int err = ESRCH;
	int i;
	
	if (!pid)
		err = signal_send(curr, sig);
	else
		for (i = 0; i < TASK_MAX; i++)
			if (task[i] && (task[i]->pid == pid || pid == -1))
			{
				if (pid == -1)
				{
					if (task[i] == curr)
						continue;
					if (task[i] == init)
						continue;
				}
				
				if (!err || err == ESRCH)
					err = signal_send(task[i], sig);
				else
					signal_send(task[i], sig);
			}
	
	if (err)
	{
		uerr(err);
		return -1;
	}
	return 0;
}

static void setup_chld(void *handler)
{
	int i;
	
	switch ((intptr_t)handler)
	{
	case (intptr_t)SIG_IGN:
		if (curr == init)
			break;
		for (i = 0; i < TASK_MAX; i++)
			if (task[i] && task[i]->parent == curr && task[i]->exited)
			{
				task[i]->parent = init;
				signal_send_k(init, SIGCHLD);
			}
		break;
	case (intptr_t)SIG_DFL:
		break;
	default:
		for (i = 0; i < TASK_MAX; i++)
			if (task[i] && task[i]->parent == curr && task[i]->exited)
				signal_raise(SIGCHLD);
		break;
	}
}

void *sys_signal(sig_t sig, void *handler)
{
	void *prev;
	
	if (sig < 0 || sig > SIGMAX)
	{
		uerr(EINVAL);
		return (void *)-1;
	}
	
	if ((sig == SIGKILL || sig == SIGSTOP) && handler != SIG_DFL)
	{
		uerr(EINVAL);
		return (void *)-1;
	}
	if (sig == SIGEVT && handler == SIG_IGN)
		handler = SIG_DFL;
	if (sig == SIGCONT && handler == SIG_IGN)
		handler = SIG_DFL;
	
	prev = curr->signal_handler[sig - 1];
	curr->signal_handler[sig - 1]	= handler;
	curr->signal_use_event[sig - 1]	= 0;
	
	if (sig == SIGCHLD)
		setup_chld(handler);
	
	return prev;
}

int sys___xwait(pid_t pid, struct _xstatus *status, int options)
{
	static struct task_queue tq = { .name = "wait" };
	
	struct _xstatus lstatus;
	int child_exist = 0;
	int err;
	int i;
	
	for (i = 0; i < TASK_MAX; i++)
	{
		if (!task[i] || task[i]->parent != curr)
			continue;
		
		if (pid > 0 && task[i]->pid != pid)
			continue;
		
		child_exist = 1;
		
		if (task[i]->exited)
		{
			memset(&lstatus, 0, sizeof lstatus);
			
			strcpy(lstatus.exec_name, task[i]->exec_name);
			lstatus.status = task[i]->exit_status;
			lstatus.pid = task[i]->pid;
			
			err = tucpy(status, &lstatus, sizeof lstatus);
			if (err)
			{
				uerr(err);
				return -1;
			}
			
			task_putslot(i);
			return 0;
		}
	}
	
	if (child_exist)
	{
		if (options & WNOHANG)
		{
			err = uset(status, 0, sizeof lstatus);
			if (err)
			{
				uerr(err);
				return -1;
			}
			return 0;
		}
		task_suspend(&tq, WAIT_INTR);
		uerr(EINTR);
		return -1;
	}
	
	uerr(ECHILD);
	return -1;
}

int sys_nice(int inc)
{
	int old = curr->priority;
	int prio;
	
	if (inc < 0 && curr->euid)
	{
		uerr(EPERM);
		return -1;
	}
	
	prio = curr->priority + inc;
	if (prio > TASK_PRIO_LOW)
		prio = TASK_PRIO_LOW;
	else if (prio < 0)
		prio = 0;
	
	task_set_prio(prio);
	return old;
}

pid_t sys_getpid()
{
	return curr->pid;
}

pid_t sys_getppid()
{
	return curr->parent->pid;
}

uid_t sys_getuid()
{
	return curr->ruid;
}

uid_t sys_geteuid()
{
	return curr->euid;
}

gid_t sys_getgid()
{
	return curr->rgid;
}

gid_t sys_getegid()
{
	return curr->egid;
}

int sys_setuid(uid_t uid)
{
	if (curr->euid && curr->ruid != uid)
	{
		uerr(EPERM);
		return -1;
	}
	
	curr->euid = uid;
	curr->ruid = uid;
	return 0;
}

int sys_setgid(gid_t gid)
{
	if (curr->euid && curr->rgid != gid)
	{
		uerr(EPERM);
		return -1;
	}
	
	curr->egid = gid;
	curr->rgid = gid;
	return 0;
}

unsigned sys_alarm(unsigned delay)
{
	unsigned ret = 0;
	unsigned tc;
	long tm;
	int s;
	
	s = intr_dis();
	tc = ticks;
	tm = time;
	intr_res(s);
	
	if (curr->alarm)
	{
		ret = curr->alarm - tm;
		curr->alarm = 0;
	}
	
	if (ret < 0)
		ret = 0;
	
	if (!delay)
	{
		s = intr_dis();
		curr->alarm_ticks = 0;
		curr->alarm	  = 0;
		intr_res(s);
		return ret;
	}
	
	s = intr_dis();
	curr->alarm = tm + delay;
	curr->alarm_ticks = tc;
	set_alarm(curr->alarm, curr->alarm_ticks);
	intr_res(s);
	
	return ret;
}

unsigned sys_ualarm(unsigned u_delay, unsigned repeat)
{
	unsigned ret = 0;
	unsigned tc, atc;
	time_t tm, atm;
	int hz = clock_hz();
	int s;
	
	if (u_delay > 1000000)
	{
		uerr(EINVAL);
		return -1;
	}
	
	s = intr_dis();
	tc = ticks;
	tm = time;
	intr_res(s);
	
	if (curr->alarm)
	{
		ret = curr->alarm_ticks - tc + (curr->alarm - tm) * 1000000;
		curr->alarm = 0;
	}
	
	if (ret < 0)
		ret = 0;
	
	if (!u_delay)
	{
		s = intr_dis();
		
		curr->alarm_ticks = 0;
		curr->alarm = 0;
		
		intr_res(s);
		return ret;
	}
	
	if (repeat)
		curr->alarm_repeat = (u_delay * hz + hz - 1) / 1000000;
	else
		curr->alarm_repeat = 0;
	
	atc = tc + (u_delay * hz) / 1000000;
	atm = tm;
	if (atc >= hz)
	{
		atm += atc / hz;
		atc %= hz;
	}
	
	s = intr_dis();
	
	curr->alarm_ticks = atc;
	curr->alarm = atm;
	
	intr_res(s);
	
	set_alarm(curr->alarm, curr->alarm_ticks);
	
	return ret;
}

int sys_pause()
{
	task_suspend(NULL, WAIT_INTR);
	uerr(EINTR);
	return -1;
}

int sys__pg_alloc(unsigned start, unsigned end)
{
	int err;

	err = u_alloc(start, end);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys__pg_free(unsigned start, unsigned end)
{
	int err;
	
	err = u_free(start, end);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys__csync(void *p, size_t size)
{
	csync(p, size);
	return 0;
}

int sys__mod_insert(const char *pathname, void *image, unsigned image_size, void *data, unsigned data_size)
{
	struct module *m = NULL;
	void *kimage = NULL;
	int err;
	
	err = usa(&pathname, PATH_MAX - 1, ENAMETOOLONG);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = kmalloc(&kimage, image_size, "module");
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	err = uga(&data, data_size, UA_READ);
	if (err)
	{
		free(kimage);
		uerr(err);
		return -1;
	}
	
	err = fucpy(kimage, image, image_size);
	if (err)
	{
		free(kimage);
		uerr(err);
		return -1;
	}
	
	err = mod_getslot(&m);
	if (err)
	{
		free(kimage);
		uerr(err);
		return -1;
	}
	
	err = mod_init(m, pathname, kimage, data, data_size);
	if (!err)
	{
		m->malloced = 1;
		return mod_getid(m);
	}
	if (m)
		m->in_use = 0;
	
	free(kimage);
	uerr(err);
	return -1;
}

int sys__mod_unload(int md)
{
	int err;
	
	err = mod_unload((unsigned)md);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys__systat(struct systat *buf)
{
	struct blk_stat bs;
	struct systat lbuf;
	int err;
	int i;
	
	memset(&lbuf, 0, sizeof lbuf);
	lbuf.task_max = TASK_MAX;
	lbuf.file_max = FS_MAXFILE;
	lbuf.fso_max  = FS_MAXFSO;
	lbuf.sw_freq  = switch_freq;
	lbuf.hz	      = clock_hz();
	lbuf.uptime   = clock_uptime();
	lbuf.cpu      = clock_cputime();
	lbuf.cpu_max  = lbuf.hz;
	
	blk_stat(&bs);
	lbuf.blk_dirty = bs.dirty;
	lbuf.blk_valid = bs.valid;
	lbuf.blk_avail = bs.free;
	lbuf.blk_max   = bs.total;
	
	lbuf.task_avail = 0;
	for (i = 0; i < TASK_MAX; i++)
		if (!task[i])
			lbuf.task_avail++;
	
	lbuf.core_avail	= (memstat_t)(pg_nfree + pg_nfree_dma) * PAGE_SIZE;
	lbuf.core_max	= (memstat_t) pg_total		       * PAGE_SIZE;
	lbuf.kva_avail	= (memstat_t) pg_vfree		       * PAGE_SIZE;
	lbuf.kva_max	= (memstat_t) PAGE_DYN_COUNT	       * PAGE_SIZE;
	
	lbuf.fso_avail = FS_MAXFSO;
	for (i = 0; i < fs_fso_high; i++)
		if (fs_fso[i].refcnt)
			lbuf.fso_avail--;
	
	lbuf.file_avail = 0;
	for (i = 0; i < FS_MAXFILE; i++)
		if (!fs_file[i].refcnt)
			lbuf.file_avail++;
	
	err = tucpy(buf, &lbuf, sizeof lbuf);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	return 0;
}

int sys__taskinfo(struct taskinfo *buf)
{
	struct task_queue *q;
	struct taskinfo lbuf;
	int err;
	int cnt;
	int s;
	int i;
	
	for (i = cnt = 0; i < TASK_MAX; i++)
	{
		memset(&lbuf, 0, sizeof lbuf);
		
		if (task[i] && (!curr->euid || task[i]->ruid == curr->euid))
		{
			intr_dis();
			q = task[i]->queue;
			intr_ena();
			
			lbuf.pid   = task[i]->pid;
			lbuf.ppid  = task[i]->parent->pid;
			lbuf.euid  = task[i]->euid;
			lbuf.egid  = task[i]->egid;
			lbuf.ruid  = task[i]->ruid;
			lbuf.rgid  = task[i]->rgid;
			lbuf.size  = task[i]->pg_count + PAGES_PER_TASK;
			lbuf.size *= PAGE_SIZE;
			lbuf.maxev = task[i]->event_high;
			lbuf.prio  = task[i]->priority;
			s = intr_dis();
			lbuf.cpu   = task[i]->cputime;
			intr_res(s);
			strcpy(lbuf.pathname, task[i]->exec_name);
			
			if (q != NULL && q->name != NULL)
			{
				strncpy(lbuf.queue, q->name, sizeof lbuf.queue);
				lbuf.queue[sizeof lbuf.queue - 1] = 0;
			}
			else if (task[i] == curr)
				strcpy(lbuf.queue, "RUNNING");
			
			err = tucpy(&buf[cnt++], &lbuf, sizeof lbuf);
			if (err)
			{
				uerr(err);
				return -1;
			}
		}
		
	}
	
	return cnt;
}

int sys__taskmax(void)
{
	return TASK_MAX;
}

int sys__modinfo(struct modinfo *buf)
{
	struct modinfo *p;
	struct module *m;
	int err;
	
	err = uga(&buf, sizeof *buf * MODULE_MAX, UA_WRITE);
	if (err)
	{
		uerr(err);
		return -1;
	}
	
	p = buf;
	for (m = module; m < module + MODULE_MAX; m++)
		if (m->in_use)
		{
			strcpy(p->name, m->name);
			p->md	= m - module;
			p->base	= m->code;
			p++;
		}
	
	return p - buf;
}

int sys__modmax(void)
{
	return MODULE_MAX;
}

int sys_gettimeofday(struct timeval *tv, struct timezone *tz)
{
	struct timeval ltv;
	int err;
	int s;
	
	memset(&ltv, 0, sizeof ltv);
	s = intr_dis();
	ltv.tv_sec   = time;
	ltv.tv_usec  = 1000000L * ticks;
	ltv.tv_usec /= clock_hz();
	intr_res(s);
	
	if (tv && (err = tucpy(tv, &ltv, sizeof ltv)))
	{
		uerr(err);
		return -1;
	}
	if (tz && (err = uset(tz, 0, sizeof *tz)))
	{
		uerr(err);
		return -1;
	}
	return 0;
}

int sys_settimeofday(struct timeval *tv, struct timezone *tz)
{
	int err;
	int s;
	
	if (tv)
	{
		err = uga(&tv, sizeof *tv, UA_READ);
		if (err)
		{
			uerr(err);
			return -1;
		}
		
		s = intr_dis();
		time  =  tv->tv_sec;
		ticks = (tv->tv_usec * clock_hz()) / 1000000L;
		intr_res(s);
	}
	return 0;
}

int sys_evt_signal(int sig, void *handler)
{
	if (sig < 1 || sig > SIGMAX)
	{
		uerr(EINVAL);
		return -1;
	}
	
	if ((sig == SIGKILL || sig == SIGSTOP) && handler != SIG_DFL)
		return 0;
	
	if (sig == SIGCONT && handler == SIG_IGN)
		handler = SIG_DFL;
	
	curr->signal_handler[sig - 1]	= handler;
	curr->signal_use_event[sig - 1] = 1;
	
	if (sig == SIGCHLD)
		setup_chld(handler);
	return 0;
}

int sys_evt_count(void)
{
	curr->unseen_events = 0; /* XXX */
	return curr->event_count;
}

int sys__evt_wait(struct event *event)
{
	static struct task_queue tq = { .name = "evtwait" };
	
	struct event *e;
	int err;
	int cnt;
	int i;
	int s;
	
	s = intr_dis();
	while (!curr->event_count)
	{
		if (curr->event_count)
			break;
		if (curr->signal_pending)
		{
			intr_res(s);
			uerr(EINTR);
			return -1;
		}
		task_suspend(&tq, WAIT_INTR);
	}
	intr_res(s);
	e = (struct event *)&curr->event[curr->first_event];
	
	if (e->type == E_WINGUI && e->win.type == WIN_E_PTR_MOVE)
	{
		s = intr_dis();
		cnt = curr->event_count - 1;
		intr_res(s);
		
		i = curr->first_event + 1;
		i %= EVT_MAX;
		
		while (cnt--)
		{
			if (curr->event[i].win.type == WIN_E_PTR_MOVE)
			{
				e->win.more = 1;
				break;
			}
			
			i++;
			i %= EVT_MAX;
		}
	}
	
	err = tucpy(event, e, sizeof *event);
	if (err)
	{
		uerr(err);
		return -1;
	}
	curr->first_event++;
	curr->first_event %= EVT_MAX;
	s = intr_dis();
	curr->unseen_events = 0;
	curr->event_count--;
	intr_res(s);
	return 0;
}

int sys__shutdown(int type)
{
	if (type != SHUTDOWN_HALT && type != SHUTDOWN_REBOOT && type != SHUTDOWN_POWER)
	{
		uerr(EINVAL);
		return -1;
	}
	
	fs_syncall();
	blk_syncall(SYNC_WRITE | SYNC_INVALIDATE);
	call_shutdown(type);
	
	switch (type)
	{
	case SHUTDOWN_HALT:
		con_sstatus(CSTATUS_HALT);
		con_reset();
		
		halt();
		
		printk("System halted.\n");
		for (;;)
			asm("hlt");
	case SHUTDOWN_REBOOT:
		printk("Please stand by while the system is being restarted.\n");
		reboot();
	case SHUTDOWN_POWER:
		printk("Safe to switch off.\n");
		power_down();
		for (;;)
			asm("hlt");
	}
	return 0;
}

int sys__dmesg(char *buf, int size)
{
	int err;
	
	if (size != sizeof con_buf)
	{
		uerr(EINVAL);
		return -1;
	}
	err = tucpy(buf, con_buf, sizeof con_buf);
	if (err)
	{
		uerr(err);
		return -1;
	}
	return con_ptr;
}

int sys__bdev_stat(struct bdev_stat *buf)
{
	int err;
	int i;
	
	err = uga(&buf, sizeof *buf * BLK_MAXDEV, UA_WRITE);
	if (err)
	{
		uerr(err);
		return -1;
	}
	memset(buf, 0, sizeof *buf * BLK_MAXDEV);
	
	for (i = 0; i < BLK_MAXDEV; i++)
	{
		struct bdev *bd = blk_dev[i];
		
		if (!bd)
			continue;
		
		strcpy(buf->name, bd->name);
		buf->read_cnt  = bd->read_cnt;
		buf->write_cnt = bd->write_cnt;
		buf->error_cnt = bd->error_cnt;
		buf++;
	}
	
	return 0;
}

int sys__bdev_max(void)
{
	return BLK_MAXDEV;
}
