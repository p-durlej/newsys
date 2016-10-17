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

#ifndef _DEVICES_H
#define _DEVICES_H

#include <limits.h>

#define DEV_MEM_COUNT	16
#define DEV_IO_COUNT	16
#define DEV_IRQ_COUNT	16
#define DEV_DMA_COUNT	16

struct device
{
	char	 desktop_name[16];
	char	 parent_type[16];
	char	 parent[16];
	int	 _unused; /* formely the boot flag */
	
	char	 driver[PATH_MAX];
	char	 desc[64];
	char	 name[16];
	char	 type[16];
	
	int	 pci_bus, pci_dev, pci_func;
	
	unsigned mem_base[DEV_MEM_COUNT];
	unsigned mem_size[DEV_MEM_COUNT];
	unsigned io_base[DEV_IO_COUNT];
	unsigned io_size[DEV_IO_COUNT];
	unsigned dma_nr[DEV_DMA_COUNT];
	unsigned irq_nr[DEV_IRQ_COUNT];
	
	int	 err;
	int	 md;
};

#endif
