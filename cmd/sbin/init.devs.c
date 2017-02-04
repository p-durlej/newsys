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

#include <priv/wingui.h>
#include <devices.h>
#include <wingui.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <paths.h>
#include <fcntl.h>
#include <stdio.h>
#include <os386.h>
#include <errno.h>
#include <err.h>

int main(int argc, char **argv)
{
	char *desktop_name = "";
	char *devs_path = _PATH_E_DEVICES;
	struct device dev;
	int off;
	int cnt;
	int vfd;
	int fd;
	int i;
	
	if (_boot_flags() & BOOT_VERBOSE)
		warnx("loading device drivers");
	
	/* XXX should use getopt */
	
	for (i = 1; i < argc; i++)
	{
		if (*argv[i] != '-')
			break;
		switch (argv[i][1])
		{
		case '-':
			i++;
			goto endopt;
		case 'c':
			devs_path = argv[i] + 2;
			continue;
		default:
			goto endopt;
		}
	}
endopt:
	if (i < argc)
	{
		desktop_name = argv[i];
		if (win_attach())
			err(255, NULL);
	}
	
	fd = open(devs_path, O_RDONLY);
	if (fd < 0)
	{
		fprintf(stderr, "init.devs: opening %s: %m\n", devs_path);
		return errno;
	}
	vfd = open(_PATH_V_DEVICES, O_WRONLY | O_CREAT | O_ATOMIC, 0600);
	if (vfd < 0)
		perror("init.devs: opening " _PATH_V_DEVICES);
	
	while (off = lseek(fd, 0L, SEEK_CUR), cnt = read(fd, &dev, sizeof dev))
	{
		if (cnt < 0)
		{
			fprintf(stderr, "init.devs: reading %s: %m\n", devs_path);
			return errno;
		}
		if (cnt != sizeof dev)
		{
			fprintf(stderr, "init.devs: %s: Short read\n", devs_path);
			return 255;
		}
		if (strcmp(dev.desktop_name, desktop_name))
			continue;
		if (!*dev.driver)
			continue;
		
		if (_boot_flags() & BOOT_VERBOSE)
			warnx("loading %s", dev.driver);
		
		dev.md = _mod_load(dev.driver, &dev, sizeof dev);
		if (dev.md < 0)
		{
			dev.err = errno;
			
			warn("%s", dev.driver);
		}
		if (vfd >= 0)
		{
			lseek(vfd, off, SEEK_SET);
			if (write(vfd, &dev, sizeof dev) < 0)
				perror("init.devs: " _PATH_V_DEVICES);
		}
		
		if (*desktop_name && !strcmp(dev.type, "display"))
		{
			int w, h;
			int wd;
			
			win_desktop_size(&w, &h);
			win_creat(&wd, 1, 0, 0, w, h, NULL, NULL);
			win_setptr(wd, WIN_PTR_BUSY);
			wp_load();
		}
	}
	return 0;
}
