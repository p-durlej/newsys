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

#include <kern/config.h>
#include <kern/printk.h>
#include <kern/signal.h>
#include <kern/errno.h>
#include <kern/event.h>
#include <kern/task.h>
#include <kern/umem.h>
#include <kern/lib.h>

#include <arch/archdef.h>
#include <sys/signal.h>

int signal_send(struct task *p, sig_t sig)
{
	if (!p)
		panic("signal_send: !p");
	
	if (curr->euid && curr->euid != p->euid)
		return EPERM;
	
	return signal_send_k(p, sig);
}

int signal_fatal(struct task *p, sig_t sig)
{
	if (!p)
		panic("signal_fatal: !p");
	
	if (sig < 0 || sig > SIGMAX)
		return EINVAL;
	
	if (!sig)
		return 0;
	
	if (p->signal_handler[sig - 1] == SIG_IGN)
		p->signal_handler[sig - 1] = SIG_DFL;
	
	return signal_send_k(p, sig);
}

int signal_send_k(struct task *p, sig_t sig)
{
	if (!p)
		panic("signal_send_k: !p");
	
	if (sig < 0 || sig > SIGMAX)
		return EINVAL;
	
	if (!sig)
		return 0;
	
	if (p->exited || p->signal_handler[sig - 1] == SIG_IGN)
		return 0;
	
	if (sig == SIGCONT || sig == SIGKILL)
	{
		p->stopped = 0;
		if (p->paused == WAIT_INTR)
			task_resume(p);
	}
	
	if (sig == SIGSTOP)
		p->stopped = 1;
	
#if SIGNAL_DEBUG
	printk("signal_send_k: sending %i to %s(%i)\n", sig, p->exec_name, p->pid);
#endif
	p->signal_received[sig - 1] = 1;
	p->signal_pending = 1;
	if (p->paused == WAIT_INTR)
		task_resume(p);
	return 0;
}

int signal_raise(sig_t sig)
{
	if (!curr)
		panic("signal_raise: !curr");
	
#if SIGNAL_DEBUG
	printk("signal_raise: %i, %s received signal %i\n",
		curr->pid, curr->exec_name, sig);
#endif
	return signal_send_k(curr, sig);
}

int signal_raise_fatal(sig_t sig)
{
	if (!curr)
		panic("signal_raise_fatal: !curr");
	
#if SIGNAL_DEBUG
	printk("%i, %s received fatal signal %i\n", curr->pid, curr->exec_name, sig);
#endif
	return signal_fatal(curr, sig);
}

static void signal_default(int i)
{
#if SIGNAL_DEBUG
	printk("signal_default: signal %i\n", i);
#endif
	switch (i)
	{
	case SIGSTOP:
	case SIGCONT:
	case SIGCHLD:
	case SIGEVT:
		break;
	case SIGQUIT:
	case SIGILL:
	case SIGTRAP:
	case SIGABRT:
	case SIGBUS:
	case SIGFPE:
	case SIGSEGV:
	case SIGSTK:
	case SIGOOM:
		dump_core(i | 0x80);
		task_exit(i | 0x80);
		break;
	default:
		task_exit(i);
	}
}

static void signal_push(int nr)
{
#ifdef __ARCH_I386__
	upush(ureg->eip);
	upush(nr);
	upush((unsigned)curr->signal_handler[nr - 1]);
	ureg->eip = (unsigned)curr->signal_entry;
#elif defined __ARCH_AMD64__
	ureg->rsp -= 128;
	upush(ureg->rip);
	upush(nr);
	upush((uintptr_t)curr->signal_handler[nr - 1]);
	ureg->rip = (uintptr_t)curr->signal_entry;
#else
#error Unknown arch
#endif
}

static void signal_dispatch(int nr)
{
	struct event e;
	
	if (curr->signal_use_event[nr - 1])
	{
		memset(&e, 0, sizeof e);
		e.type = E_SIGNAL;
		e.sig  = nr;
		e.proc = curr->signal_handler[nr - 1];
		send_event(curr, &e);
	}
	else
		signal_push(nr);
	
	curr->signal_handler[nr - 1] = SIG_DFL;
}

void signal_handle(void)
{
	int i;
	
	if (!curr)
		panic("signal_handle: !curr");
	
	while (curr->signal_pending)
	{
		curr->signal_pending = 0;
		
		for (i = 1; i <= SIGMAX; i++)
			if (curr->signal_received[i - 1])
			{
				curr->signal_received[i - 1] = 0;
				
				switch ((intptr_t)curr->signal_handler[i - 1])
				{
				case (intptr_t)SIG_IGN:
					break;
				case (intptr_t)SIG_DFL:
					signal_default(i);
					break;
				default:
					signal_dispatch(i);
				}
			}
	}
}
