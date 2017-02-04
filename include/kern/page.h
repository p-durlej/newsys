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

#ifndef _KERN_PAGE_H
#define _KERN_PAGE_H

#include <kern/arch/page.h>

#ifndef __ASSEMBLER__

#include <stdint.h>

#ifndef NULL
#define NULL (void *)0
#endif

#define pg2vap(pg)	((void *)((uintptr_t)(pg) << PAGE_SHIFT))
#define pg2size(pg)	((size_t)(pg) << PAGE_SHIFT)

#define vap2pg(p)	((vpage)((uintptr_t)(p) >> PAGE_SHIFT))

extern vpage *pg_tab;

extern ppage pg_nfree_dma;
extern ppage pg_nfree;
extern ppage pg_total;
extern ppage pg_vfree;

void	pg_init(void);
void	pg_utlb(void);

void	pg_ena0(void);
void	pg_dis0(void);

int	pg_alloc(ppage *p);
void	pg_free(ppage p);

int	pg_alloc_dma(ppage *p, ppage s);
void	pg_free_dma(ppage p, ppage s);

int	pg_adget(vpage *p, vpage s);
void	pg_adput(vpage p, vpage s);

int	pg_atmem(vpage p, vpage s, int reuse);
void	pg_dtmem(vpage p, vpage s);

int	pg_atdma(vpage p, vpage s);
void	pg_dtdma(vpage p, vpage s);

int	pg_atphys(vpage p, vpage s, ppage phys, int cache);
void	pg_dtphys(vpage p, vpage s);

int	pg_uauto(vpage p);
int	pg_fault(int umode);

int	pg_newdir(vpage *buf);
void	pg_setdir(vpage *dir);
vpage *	pg_getdir(void);

void *	pg_getphys(void *p);

int	pg_altab(vpage pti, int flags);

#endif

#endif
