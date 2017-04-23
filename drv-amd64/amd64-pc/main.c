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

#include <kern/module.h>
#include <kern/config.h>
#include <kern/power.h>
#include <kern/errno.h>

void con_init(void);
int  gc_init(void);

void do_panic(const char *msg);
int  apm_off(void);
int  halt(void);

static int suspend(void)
{
	return ENODEV;
}

struct power_ops pwops =
{
	.power_down	= apm_off,
	.suspend	= suspend,
	.halt		= halt,
};

int mod_onload(unsigned md, const char *pathname, const void *data, unsigned data_size)
{
	int cnt = 0;
	
#if KVERBOSE
	printk("amd64-pc.drv: amd64 pc machine driver\n");
#endif
	
	intr_init();
	clock_init();
	pwr_install(&pwops);
	panic_install(do_panic);
	return 0;
}

int mod_onunload(unsigned md)
{
	return 0;
}
