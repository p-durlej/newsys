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

#ifndef NULL
#define NULL (void *)0
#endif

#ifndef _KERN_PROC_H
#define _KERN_PROC_H

#include <kern/machine/machine.h>
#include <kern/machine/task.h>
#include <kern/wingui.h>
#include <kern/limits.h>
#include <kern/intr.h>
#include <kern/page.h>
#include <kern/cio.h>
#include <kern/lib.h> /* XXX */
#include <kern/fs.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <os386.h>
#include <event.h>

#define TASK_MAX	64

#define PID_RESTART	1024
#define PID_MAX		16383

#define WAIT_NOINTR	1
#define WAIT_INTR	2

#define PAGES_PER_TASK	((sizeof(struct task) + PAGE_SIZE - 1) / PAGE_SIZE)

typedef void task_dproc(void *cx);

struct task_queue;

struct task
{
	char k_stack[32760];	/* offset must be 0 */
	void *k_sp;		/* offset must be 32760 */
				/* special values of k_sp: (see sched.c)
				 *   0 - call new_task_entry
				 *   1 - call after_fork
				 */
	
#ifdef __ARCH_I386__
	unsigned __attribute__((aligned(4096)))
				pg_dir[1024];
#elif defined __ARCH_AMD64__
	vpage __attribute__((aligned(4096)))
				pg_dir[512];
#else
#error Unknown arch
#endif
	unsigned		pg_count;
	
	struct mach_task	mach_task;
	
	unsigned char __attribute__((aligned(16)))
				fpu_state[512];
	int			fpu_saved;
	struct task *		parent;
	
	struct task_queue *	queue;
	struct task *		prev;
	struct task *		next;
	
	volatile int		cputime_cnt;
	volatile int		cputime;
	
	uid_t			euid;
	gid_t			egid;
	uid_t			ruid;
	gid_t			rgid;
	pid_t			pid;
	
	char			exec_name[PATH_MAX];
	char			exec_arg[ARG_MAX];
	int			exec_arg_size;
	int			exec;
	
	unsigned		exit_status;
	int			exited;
	
	volatile int		time_slice;
	volatile int		priority;
	volatile int		paused;
	int			stopped;
	
	struct fs_desc		file_desc[OPEN_MAX];
	char			cwd[PATH_MAX];
	char			tty[PATH_MAX];
	int			multipoll;
	int			ocwd;
	
	struct win_task		win_task;
	
	void *			signal_entry;
	void *			errno;
	
	unsigned		signal_received[NSIG + 1];
	void *			signal_handler[NSIG + 1];
	int			signal_use_event[NSIG + 1];
	int			signal_pending;
	int			signal_held;
	
	unsigned		alarm_repeat;
	unsigned		alarm_ticks;
	long			alarm;
	
	volatile struct event	event[EVT_MAX];
	volatile int		unseen_events;
	volatile int		event_count;
	volatile int		event_high;
	int			first_event;
	volatile int		last_event;
};

struct task_uargs
{
	char *	name;
	void *	args;
	size_t	size;
};

extern struct task *	task[TASK_MAX];
extern struct task *	curr;
extern struct task *	init;

extern int		task_count;
extern int		task_pcount;

void	task_init(void);

pid_t	newpid(void);

int	task_find(struct task **tp, pid_t pid);

int	task_getslot(int *p);
void	task_putslot(int i);

int	task_suspend(struct task_queue *tq, int type);
void	task_resume(struct task *task);

int	task_creat(const char *name, const void *arg, int arg_size, pid_t *pid);
int	task_exec(const char *name, const void *arg, int arg_size);
void	task_exit(int status);

void	task_set_prio(int prio);

void	task_defer(task_dproc *proc, void *cx);
void	task_rundp(void);

#endif
