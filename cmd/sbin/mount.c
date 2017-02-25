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

#include <mkcanon.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <mount.h>
#include <stdio.h>
#include <errno.h>

void usage()
{
	fprintf(stderr, "Usage: mount OPTIONS [--] DEVICE PREFIX\n\n");
	fprintf(stderr, "Mount a filesystem.\n\n");
	fprintf(stderr, " -t TYPE  device contains file system of type TYPE\n");
	fprintf(stderr, " -r       mount read-only\n");
	fprintf(stderr, " -m       mount a removable disk drive\n");
	fprintf(stderr, " -i       mount in insecure mode\n");
	fprintf(stderr, " -R       remount filesystem\n");
	fprintf(stderr, " -n       do not update access time on files in this filesystem\n\n");
}

int main(int argc, char **argv)
{
	char prefix[PATH_MAX];
	char device[PATH_MAX];
	char *devname;
	char *type = "native";
	int remount = 0;
	int flags = 0;
	int i;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		usage();
		return 0;
	}
	
	if (argc < 3)
	{
		fprintf(stderr, "mount: wrong number of arguments\n");
		return 255;
	}
	
	i = 1;
	while (*argv[i] == '-') // XXX
	{
		char *p;
		
		if (!strcmp(argv[i], "-t"))
		{
			if (!argv[++i])
			{
				fprintf(stderr, "mount: option -t requires an argument\n");
				return 255;
			}
			type = argv[i++];
			continue;
		}
		
		p = argv[i] + 1;
		while (*p)
		{
			switch (*p)
			{
				case 'R':
					remount = 1;
					break;
				case 'r':
					flags |= MF_READ_ONLY;
					break;
				case 'm':
					flags |= MF_REMOVABLE;
					break;
				case 'n':
					flags |= MF_NO_ATIME;
					break;
				case 'i':
					flags |= MF_INSECURE;
					break;
				case '-':
					i++;
					goto end_opt;
				default:
					fprintf(stderr, "mount: bad option '%c'\n", *p);
					return 255;
			}
			p++;
		}
		
		i++;
	}
	
end_opt:
	
	if (i + 2 > argc)
	{
		fprintf(stderr, "mount: wrong number of arguments\n");
		return 255;
	}
	
	strcpy(device, argv[i]);
	strcpy(prefix, argv[i + 1]);
	
	_mkcanon("/dev", device);
	_mkcanon(".",	 prefix);
	
	if (!strncmp(device, "/dev/", 5))
		devname = device + 5;
	else
		devname = device;
	
	if (remount)
		_umount(prefix);
	
	if (_mount(prefix, devname, type, flags))
	{
		perror("mount: _mount");
		return errno;
	}
	
	return 0;
}
