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

#include <devices.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

void show_dev(struct device *dev)
{
	int i;
	
	printf("Device:\n\n");
	if (*dev->desktop_name)
		printf(" Desktop: \"%s\"\n", dev->desktop_name);
	printf(" Driver: \"%s\"\n", dev->driver);
	printf(" Name: \"%s\"\n", dev->name);
	printf(" Type: \"%s\"\n", dev->type);
	
	for (i = 0; i < DEV_MEM_COUNT; i++)
		if (dev->mem_size[i])
			printf(" Memory area #%i: 0x%08x - 0x%08x\n", i, dev->mem_base[i], dev->mem_base[i] + dev->mem_size[i] - 1);
	
	for (i = 0; i < DEV_IO_COUNT; i++)
		if (dev->io_size[i])
			printf(" I/O area #%i: 0x%04x - 0x%04x\n", i, dev->io_base[i], dev->io_base[i] + dev->io_size[i] - 1);
	
	for (i = 0; i < DEV_DMA_COUNT; i++)
		if (dev->dma_nr[i] != (unsigned)-1)
			printf(" DMA channel #%i: %i\n", i, dev->dma_nr[i]);
	
	for (i = 0; i < DEV_IRQ_COUNT; i++)
		if (dev->irq_nr[i] != (unsigned)-1)
			printf(" IRQ line #%i: %i\n", i, dev->irq_nr[i]);
	
	printf("\n");
}

int main(int argc, char **argv)
{
	struct device dev;
	char *name;
	int cnt;
	int fd;
	
	name = argc > 1 ? argv[1] : "/etc/devices";
	
	fd = open(name, O_RDONLY);
	if (fd < 0)
		err(errno, "%s", name);
	
	while (cnt = read(fd, &dev, sizeof dev), cnt)
	{
		if (cnt < 0)
			err(errno, "%s", name);
		
		if (cnt != sizeof dev)
			err(255, "%s: Short read", name);
		
		show_dev(&dev);
	}
	return 0;
}
