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

#include <dev/pci.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <err.h>

int main()
{
	struct pci_dev pcd;
	ssize_t cnt;
	int fd;
	int i;
	
	fd = open("/dev/pci", O_RDONLY);
	if (fd < 0)
		err(1, "/dev/pci");
	
	while (cnt = read(fd, &pcd, sizeof pcd), cnt == sizeof pcd)
	{
		printf("PCI dev, bus = %i, dev = %i, func = %i: \n",
			pcd.bus, pcd.dev, pcd.func);
		printf("  vendor = 0x%x\n", (unsigned)pcd.dev_vend);
		printf("  id     = 0x%x\n", (unsigned)pcd.dev_id);
		printf("  class  = 0x%x\n", (unsigned)pcd.dev_class);
		printf("  rev    = 0x%x\n", (unsigned)pcd.dev_rev);
		printf("  IRQ    = %i\n", pcd.irq);
		printf("  intr   = %c\n", "-ABCD"[pcd.intr]);
		
		for (i = 0; i < 6; i++)
		{
			uint32_t base = pcd.mem_base[i];
			uint32_t size = pcd.mem_size[i];
			
			if (!size)
				continue;
			
			if (pcd.is_io[i])
				printf("  I/O #%i: 0x%04jx, size 0x%04jx\n", i, (uintmax_t)base, (uintmax_t)size);
			else
				printf("  Mem #%i: 0x%08jx, size 0x%08jx\n", i, (uintmax_t)base, (uintmax_t)size);
		}
		putchar('\n');
	}
	if (cnt < 0)
		err(1, "/dev/pci");
	if (cnt)
		errx(1, "/dev/pci: Short read");
	
	return 0;
}
