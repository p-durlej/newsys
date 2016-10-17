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

#include <kern/intr.h>

#define CASCADE_IRQ	2
#define IRQ_BASE	0x40

static int		irq_spur[16];
static void *		irq_hand[16];
int			irq_level;

void asm_irq_0();
void asm_irq_1();
void asm_irq_2();
void asm_irq_3();
void asm_irq_4();
void asm_irq_5();
void asm_irq_6();
void asm_irq_7();
void asm_irq_8();
void asm_irq_9();
void asm_irq_10();
void asm_irq_11();
void asm_irq_12();
void asm_irq_13();
void asm_irq_14();
void asm_irq_15();

void i8259_init(void)
{
	outb(0x20, 0x11);
	outb(0x21, IRQ_BASE);
	outb(0x21, 1 << CASCADE_IRQ);
	outb(0x21, 0x01);
	
	outb(0xa0, 0x11);
	outb(0xa1, IRQ_BASE + 8);
	outb(0xa1, CASCADE_IRQ);
	outb(0xa1, 0x01);
	
	outb(0x21, 0xfb);
	outb(0xa1, 0xff);
}

void irq_ena(int i)
{
	if (i < 8)
		outb(0x21, inb(0x21) & ~(1 << i));
	else
		outb(0xa1, inb(0xa1) & ~(1 << (i-8)));
}

void irq_dis(int i)
{
	if (i < 8)
		outb(0x21, inb(0x21) | (1 << i));
	else
		outb(0xa1, inb(0xa1) | (1 << (i-8)));
}

void irq_eoi(int i)
{
	if (i >= 16 || i < 0)
		panic("irq_eoi: invalid irq number");
	
	if (i < 8)
		outb(0x20, 0x60 | i);
	else
	{
		outb(0xa0, 0x60 | (i - 8));
		outb(0x20, 0x62);
	}
}

void irq_set(int i, void *proc)
{
	int s;
	
	s = intr_dis();
	irq_hand[i] = proc;
	intr_res(s);
}

static void irq_spurious(int i)
{
	if (!irq_spur[i])
	{
		printk("intr_irq: unexpected irq %i\n", i);
		irq_spur[i] = 1;
	}
	irq_dis(i);
	irq_eoi(i);
}

void intr_irq(int i)
{
	void (*proc)(int i, int count);
	
	if (i == 2)
		return;
	
	if (i < 8)
	{
		if ((inb(0x21) >> i) & 1)
		{
			irq_spurious(i);
			return;
		}
	}
	else
	{
		if ((inb(0xa1) >> (i - 8)) & 1)
		{
			irq_spurious(i);
			return;
		}
	}
	
	if (!irq_hand[i])
	{
		printk("intr_irq: no handler for irq %i\n", i);
		panic("intr_irq: no handler for irq");
	}
	
	proc = irq_hand[i];
	proc(i, 1);
	irq_eoi(i);
}

void intr_init(void)
{
	intr_dis(); // XXX
	
	intr_set(IRQ_BASE     , asm_irq_0,  0);
	intr_set(IRQ_BASE +  1, asm_irq_1,  0);
	intr_set(IRQ_BASE +  2, asm_irq_2,  0);
	intr_set(IRQ_BASE +  3, asm_irq_3,  0);
	intr_set(IRQ_BASE +  4, asm_irq_4,  0);
	intr_set(IRQ_BASE +  5, asm_irq_5,  0);
	intr_set(IRQ_BASE +  6, asm_irq_6,  0);
	intr_set(IRQ_BASE +  7, asm_irq_7,  0);
	intr_set(IRQ_BASE +  8, asm_irq_8,  0);
	intr_set(IRQ_BASE +  9, asm_irq_9,  0);
	intr_set(IRQ_BASE + 10, asm_irq_10, 0);
	intr_set(IRQ_BASE + 11, asm_irq_11, 0);
	intr_set(IRQ_BASE + 12, asm_irq_12, 0);
	intr_set(IRQ_BASE + 13, asm_irq_13, 0);
	intr_set(IRQ_BASE + 14, asm_irq_14, 0);
	intr_set(IRQ_BASE + 15, asm_irq_15, 0);
	
	i8259_init();
	intr_ena();
}
