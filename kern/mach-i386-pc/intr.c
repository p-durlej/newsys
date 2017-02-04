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

#include <kern/arch/selector.h>
#include <kern/arch/cpu.h>
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
	"Divide Error",
	"Debug Exception",
	"NMI Interrupt",
	"Breakpoint Interrupt",
	"INTO Detected Overflow",
	"BOUND Range Exceeded",
	"Invalid Opcode",
	"Device Not Available",
	"Double Fault",
	"CoProcessor Segment Overrun (Reserved)",
	"Invalid Task State Segment",
	"Segment Not Present",
	"Stack Fault",
	"General Protection",
	"Page Fault",
	"Exception 15",
	"Floating-Point Error",
	"Alignment Check",
	"Machine Check"
};

struct intr_regs *	ureg;

void intr_set(int i, void *p, int dpl)
{
	extern unsigned short idt[];
	
	int s;
	
	s = intr_dis();
	idt [i * 4 + 0] = (unsigned short)(unsigned)p;
	idt [i * 4 + 1] = KERN_CS;
	idt [i * 4 + 2] = 0x8e00 + (dpl << 13);
	idt [i * 4 + 3] = (unsigned short)((unsigned)p >> 16);
	intr_res(s);
}

static void intr_fault(struct intr_regs *r, int i)
{
	char *mode;
	uint32_t *p;
	
	if (r->eflags & (1 << 17))
		mode = "V86";
	else
	{
		if (r->cs & 3)
			mode = "USER";
		else
			mode = "KERNEL";
	}
	
	printk("\n %s MODE EXCEPTION %i @ %x:%x\n", mode, i, r->cs, r->eip);
	
	printk(" EAX=%x EFL=%x\n", r->eax, r->eflags);
	printk(" EBX=%x ESI=%x\n", r->ebx, r->esi);
	printk(" ECX=%x EDI=%x\n", r->ecx, r->edi);
	printk(" EDX=%x EBP=%x",   r->edx, r->ebp);
	
	if (i == 6)
	{
		printk("\n\n");
		for (p = (uint32_t *)r->eip; p < (uint32_t *)r->eip + 16; p++)
			printk("%x ", *p);
	}
	
	backtrace((void *)r->ebp);
	panic(exc_desc[i]);
}

static void *v86_ptr(uint16_t seg, uint16_t off)
{
	return (void *)(seg * 16 + off);
}

static void v86_softint(struct intr_regs *r, int intr)
{
	uint16_t *vec;
	uint16_t *sp;
	
	// printk("v86_softint: 0x%x, r = %p\n", intr, r);
	vec = (uint16_t *)(intr * 4);
	sp  = v86_ptr(r->ss, r->esp);
	
	*--sp = r->eflags;
	*--sp = r->cs;
	*--sp = r->eip + 2;
	r->esp -= 6;
	
	// printk("v86_softint: will return to %x:%x\n", r->cs, r->eip + 2);
	// printk("v86_softint: interrupt vector %x:%x\n", vec[1], vec[0]);
	
	r->eflags &= ~FLAG_IF;
	r->eip	   = vec[0];
	r->cs	   = vec[1];
}

static void v86_exc(struct intr_regs *r, int i)
{
	uint8_t *ip = v86_ptr(r->cs, r->eip);
	
	// printk("v86_exc: *ip = %x\n", *ip);
	switch (i)
	{
	case 13: /* general protection */
		switch (ip[0])
		{
		case 0xcd:
			v86_softint(r, ip[1] & 255);
			break;
		default:
			intr_fault(r, i);
		}
		break;
	default:
		intr_fault(r, i);
	}
}

void intr_exc(struct intr_regs *r, int i)
{
	extern int fpu_present;
	
	if (i == 2 || i == 8)
		intr_fault(r, i);
	
	if (r->eflags & FLAG_VM86)
	{
		v86_exc(r, i);
		return;
	}
	
	if (r->cs & 0x0003)
	{
		/* if (i != 14)
		{
			printk("\n USER MODE EXCEPTION %i @ %p:%p\n", i, r->cs, r->eip);
			
			printk(" EAX=%x EFL=%x\n", r->eax, r->eflags);
			printk(" EBX=%x ESI=%x\n", r->ebx, r->esi);
			printk(" ECX=%x EDI=%x\n", r->ecx, r->edi);
			printk(" EDX=%x EBP=%x\n", r->edx, r->ebp);
			printk(" ESP=%x\n", r->esp);
			
			printk(" curr->exec_name = \"%s\"\n", curr->exec_name);
			printk(" curr->pid = %i\n", curr->pid);
		} */
		
		switch (i)
		{
		case 0: /* integer division error */
			signal_raise_fatal(SIGFPE);
			break;
		
		case 1: /* debug exception */
			r->eflags &= 0xfffffeff;
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
			if (fpu_present)
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
	if (r->cs & 0x0003 && !(r->eflags & FLAG_VM86))
		ureg = r;
}

void intr_leave(struct intr_regs *r, int i)
{
	int do_exec(void);
	
	int err;
	
	if (r->cs & 0x0003 && !(r->eflags & FLAG_VM86))
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
	if (r->eflags & FLAG_VM86)
	{
		r->eflags &= ~FLAG_VM86;
		r->eip	  += r->cs << 4;
		r->cs	   = KERN_CS;
		r->ds	   = KERN_DS;
		r->es	   = KERN_DS;
		r->fs	   = KERN_DS;
		r->gs	   = KERN_DS;
		r->ss	   = KERN_DS;
		return;
	}
	
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
