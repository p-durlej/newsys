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

#include <config/kern.h>

#include <kern/arch/selector.h>
#include <kern/console.h>
#include <kern/syscall.h>
#include <kern/printk.h>
#include <kern/signal.h>
#include <kern/sched.h>
#include <kern/umem.h>
#include <kern/intr.h>
#include <kern/task.h>
#include <kern/page.h>
#include <kern/lib.h>
#include <kern/hw.h>

#include <sys/signal.h>

void asm_spurious();
void asm_exc_0();
void asm_exc_1();
void asm_exc_2();
void asm_exc_3();
void asm_exc_4();
void asm_exc_5();
void asm_exc_6();
void asm_exc_7();
void asm_exc_8();
void asm_exc_9();
void asm_exc_10();
void asm_exc_11();
void asm_exc_12();
void asm_exc_13();
void asm_exc_14();
void asm_exc_15();
void asm_exc_16();
void asm_exc_17();
void asm_exc_18();

void asm_syscall();

static const char *const exc_desc[19]=
{
	"divide error",
	"debug exception",
	"NMI interrupt",
	"breakpoint interrupt",
	"INTO detected overflow",
	"BOUND range exceeded",
	"invalid opcode",
	"device not available",
	"double fault",
	"coprocessor segment overrun (reserved)",
	"invalid task state segment",
	"segment not present",
	"stack fault",
	"general protection",
	"page fault",
	"exception 15",
	"floating-point error",
	"alignment check",
	"machine check"
};

struct intr_regs *	ureg;

void intr_set(int i, void *p, int dpl)
{
	extern unsigned short idt[];
	
	int s;
	
	i <<= 3;
	
	s = intr_dis();
	idt[i++] = (uintptr_t)p;
	idt[i++] = KERN_CS;
	idt[i++] = 0x8e00 + (dpl << 13);
	idt[i++] = (uintptr_t)p >> 16;
	idt[i++] = (uintptr_t)p >> 32;
	idt[i++] = (uintptr_t)p >> 48;
	idt[i++] = 0;
	idt[i++] = 0;
	intr_res(s);
}

static void intr_fault(struct intr_regs *r, int i)
{
	char *mode;
	
	if (r->cs & 3)
		mode = "USER";
	else
		mode = "KERNEL";
	
	printk("\n %s MODE EXCEPTION %i @ %hx:%lx\n\n", mode, i, (unsigned short)r->cs, (unsigned long)r->rip);
	
	printk(" RAX = %lx RFL = %lx\n", (unsigned long)r->rax, (unsigned long)r->rflags);
	printk(" RBX = %lx RSI = %lx\n", (unsigned long)r->rbx, (unsigned long)r->rsi);
	printk(" RCX = %lx RDI = %lx\n", (unsigned long)r->rcx, (unsigned long)r->rdi);
	printk(" RDX = %lx RBP = %lx\n", (unsigned long)r->rdx, (unsigned long)r->rbp);
	printk(" R8  = %lx R9  = %lx\n", (unsigned long)r->r8,  (unsigned long)r->r9);
	printk(" R10 = %lx R11 = %lx\n", (unsigned long)r->r10, (unsigned long)r->r11);
	printk(" R12 = %lx R13 = %lx\n", (unsigned long)r->r12, (unsigned long)r->r13);
	printk(" R14 = %lx R15 = %lx\n", (unsigned long)r->r14, (unsigned long)r->r15);
	
#if VERBOSE_PANIC
	backtrace((void *)r->rbp);
#else
	printk("\n");
#endif
	panic(exc_desc[i]);
}

void intr_exc(struct intr_regs *r, int i)
{
	if (i == 2 || i == 8)
		intr_fault(r, i);
	
	if (r->cs & 0x0003)
	{
		switch (i)
		{
		case 0: /* integer division error */
			signal_raise_fatal(SIGFPE);
			break;
		
		case 1: /* debug exception */
			r->rflags &= 0xfffffffffffffeff;
			signal_raise_fatal(SIGTRAP);
			break;
		
		case 3: /* breakpoint */
		case 4: /* INTO detected overflow */
		case 5: /* bound range exceeded */
			signal_raise_fatal(SIGTRAP);
			break;
		
		case 6: /* invalid opcode */
		case 7: /* device not available */
			signal_raise_fatal(SIGILL);
			break;
		
		case 11: /* segment not present */
		case 12: /* stack fault */
		case 13: /* general protection */
			signal_raise_fatal(SIGSEGV);
			break;
			
		case 14: /* page fault */
			pg_fault(1);
			break;
		
		case 16: /* FPU error */
			signal_raise_fatal(SIGFPE);
			asm("fnclex");
			break;
		
		case 17: /* alignment check */
			signal_raise_fatal(SIGTRAP);
			break;
		
		default:
			printk("exception %i\n", i);
			signal_raise_fatal(SIGILL);
			break;
		}
	}
	else
	{
		int k_fault = 1;
		
		if (i == 14)
			k_fault = pg_fault(0);
		
		if (k_fault)
			intr_fault(r, i);
	}
}

void intr_enter(struct intr_regs *r, int i)
{
	if (r->cs & 3)
		ureg = r;
}

void intr_leave(struct intr_regs *r, int i)
{
	int do_exec(void);
	
	int err;
	
	if (r->cs & 3)
	{
		task_rundp();
		
		intr_dis();
restart:
		while (curr->signal_pending)
		{
			intr_ena();
			signal_handle();
			intr_dis();
		}
		while (curr->stopped)
			task_suspend(NULL, WAIT_INTR);
		if (curr->signal_pending)
			goto restart;
		
		if (resched || curr->paused || curr->exited)
		{
			sched();
			goto restart;
		}
		
		if (curr->exec)
		{
			intr_ena();
			err = do_exec();
			if (err)
			{
				curr->signal_handler[SIGEXEC - 1] = SIG_DFL;
				signal_raise_fatal(SIGEXEC);
			}
			intr_dis();
			goto restart;
		}
	}
}

void intr_syscall(struct intr_regs *r, int i)
{
	if (r != ureg)
		panic("intr_syscall: r != ureg\n");
	
	syscall();
}

void intr_init(void)
{
	int i;
	
	for (i = 0; i < 256; i++)
		intr_set(i, asm_spurious, 0);
	
	intr_set(0,	asm_exc_0,	0);
	intr_set(1,	asm_exc_1,	0);
	intr_set(2,	asm_exc_2,	0);
	intr_set(3,	asm_exc_3,	3);
	intr_set(4,	asm_exc_4,	0);
	intr_set(5,	asm_exc_5,	0);
	intr_set(6,	asm_exc_6,	0);
	intr_set(7,	asm_exc_7,	0);
	intr_set(8,	asm_exc_8,	0);
	intr_set(9,	asm_exc_9,	0);
	intr_set(10,	asm_exc_10,	0);
	intr_set(11,	asm_exc_11,	0);
	intr_set(12,	asm_exc_12,	0);
	intr_set(13,	asm_exc_13,	0);
	intr_set(14,	asm_exc_14,	0);
	intr_set(15,	asm_exc_15,	0);
	intr_set(16,	asm_exc_16,	0);
	intr_set(17,	asm_exc_17,	0);
	intr_set(18,	asm_exc_18,	0);
	
	intr_set(0x80,	asm_syscall,	3);
}
