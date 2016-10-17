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

#include <priv/natfs.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <err.h>

int main(int argc, char **argv)
{
	struct nat_super sb;
	char *dev_path;
	int indl = -1;
	int ndir = -1;
	int fd;
	int c;
	
	while (c = getopt(argc, argv, "I:D:"), c > 0)
		switch (c)
		{
		case 'I':
			indl = atoi(optarg);
			break;
		case 'D':
			ndir = atoi(optarg);
			break;
		default:
			return 255;
		}
	if (optind != argc - 1)
		errx(1, "wrong nr of args");
	dev_path = argv[optind];
	
	fd = open(dev_path, O_RDWR);
	if (fd < 0)
		err(1, "%s", dev_path);
	
	lseek(fd, 512, SEEK_SET);
	if (read(fd, &sb, sizeof sb) < 0)
		err(1, "%s", dev_path);
	
	if (indl >= 0)
		sb.nindirlev = indl;
	if (ndir >= 0)
		sb.ndirblks = ndir;
	
	printf("nindirlev = %i\n", (int)sb.nindirlev);
	printf("ndirblks  = %i\n", (int)sb.ndirblks);
	
	if (indl >= 0 || ndir >= 0)
	{
		lseek(fd, 512, SEEK_SET);
		if (write(fd, &sb, sizeof sb) < 0)
			err(1, "%s", dev_path);
	}
	
	close(fd);
	return 0;
}
