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

#include <sys/types.h>
#include <dev/rd.h>
#include <stdlib.h>
#include <unistd.h>
#include <bioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

static int xcode;

static void showrd(const char *dn)
{
	struct bio_info bi;
	int d;
	
	d = open(dn, O_RDWR);
	if (d < 0)
	{
		xcode = errno;
		warn("%s: open", dn);
		return;
	}
	
	if (ioctl(d, BIO_INFO, &bi))
	{
		xcode = errno;
		warn("%s: BIO_INFO", dn);
		close(d);
		return;
	}
	close(d);
	
	printf("%s: %ju\n", dn, (uintmax_t)bi.size);
}

static void showall(void)
{
	char dn[16];
	int i;
	
	for (;;)
	{
		sprintf(dn, "/dev/rd%i", i++);
		if (access(dn, 0) && errno == ENOENT)
			break;
		showrd(dn);
	}
}

int main(int argc, char **argv)
{
	blk_t sz;
	char *dn;
	int d;
	
	if (argc > 3)
		errx(127, "wrong nr of args");
	
	if (argc > 1)
	{
		dn = argv[1];
		
		if (argc > 2)
		{
			sz = strtoull(argv[2], NULL, 0);
			
			d = open(dn, O_RDWR);
			if (d < 0)
				err(errno, "%s: open", dn);
			
			if (ioctl(d, RDIOCNEW, &sz))
				err(errno, "%s: RDIOCNEW", dn);
		}
		else
			showrd(dn);
	}
	else
		showall();
	
	return 0;
}
