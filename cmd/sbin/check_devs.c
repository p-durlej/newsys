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

#include <wingui_msgbox.h>
#include <wingui.h>
#include <devices.h>
#include <unistd.h>
#include <string.h>
#include <mount.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <paths.h>
#include <err.h>

static int xit;

static void check_devs(void)
{
	char msg[256 + PATH_MAX];
	struct device dev;
	int cnt;
	int fd;
	
	fd = open(_PATH_V_DEVICES, O_RDONLY);
	if (fd < 0)
	{
		msgbox_perror(NULL, "Devices", "Cannot open " _PATH_V_DEVICES, errno);
		xit = 1;
		return;
	}
	while ((cnt = read(fd, &dev, sizeof dev)))
	{
		if (cnt < 0)
		{
			msgbox_perror(NULL, "Devices", "Cannot read " _PATH_V_DEVICES, errno);
			xit = 1;
			return;
		}
		if (cnt != sizeof dev)
		{
			msgbox(NULL, "Devices", _PATH_V_DEVICES ":\n\nShort read");
			xit = 1;
			return;
		}
		if (dev.md < 0 && dev.err)
		{
			sprintf(msg, "Device driver for %s (%s) failed to run:\n\n"
				     "    %s\n\n"
				     "Please check device configuration in the Hardware applet in Prefs.",
				dev.desc, dev.name, strerror(dev.err));
			msgbox(NULL, "Devices", msg);
		}
	}
}

static void root_mismatch(const char *device, const char *type)
{
	char buf[256];
	FILE *sf, *df;
	
	if (msgbox_ask(NULL, "Root device", "Root filesystem configuration is not up to date.\n\nUpdate the configuration?") != MSGBOX_YES)
		return;
	
	sf = fopen(_PATH_E_BOOT_CONF, "r");
	if (sf == NULL)
		return;
	
	df = fopen(_PATH_E_BOOT_CONF ".new", "w");
	if (df == NULL)
		return;
	
	while (fgets(buf, sizeof buf, sf))
	{
		if (!strncmp(buf, "rdev=", 5))
		{
			fprintf(df, "rdev=%s\n", device);
			continue;
		}
		
		if (!strncmp(buf, "rtype=", 6))
		{
			fprintf(df, "rtype=%s\n", type);
			continue;
		}
		
		fputs(buf, df);
	}
	
	fclose(sf);
	if (fclose(df))
		goto save_failed;
	
	link(_PATH_E_BOOT_CONF, _PATH_E_BOOT_CONF ".old");
	
	if (rename(_PATH_E_BOOT_CONF ".new", _PATH_E_BOOT_CONF))
		goto save_failed;
	
	return;
save_failed:
	msgbox_perror(NULL, "Devices", "Failed to save " _PATH_E_BOOT_CONF, errno);
	unlink(_PATH_E_BOOT_CONF ".new");
}

static void check_root(void)
{
	char *rdev = "", *rtype = "";
	struct _mtab mtab[MOUNT_MAX];
	char buf[256];
	char *p;
	FILE *f;
	int i;
	
	f = fopen(_PATH_E_BOOT_CONF, "r");
	if (f == NULL)
	{
		msgbox_perror(NULL, "Devices", "Cannot open " _PATH_E_BOOT_CONF, errno);
		return;
	}
	
	while (fgets(buf, sizeof buf, f))
	{
		p = strchr(buf, '\n');
		if (p != NULL)
			*p = 0;
		
		if (!strncmp(buf, "rdev=", 5))
		{
			rdev = strdup(buf + 5);
			continue;
		}
		
		if (!strncmp(buf, "rtype=", 6))
		{
			rtype = strdup(buf + 6);
			continue;
		}
	}
	
	fclose(f);
	
	memset(mtab, 0, sizeof mtab);
	if (_mtab(mtab, sizeof mtab))
	{
		msgbox_perror(NULL, "Devices", "Cannot get mounted filesystem table", errno);
		return;
	}
	
	for (i = 0; i < MOUNT_MAX; i++)
	{
		if (!mtab[i].mounted)
			continue;
		
		if (*mtab[i].prefix)
			continue;
		
		if (strcmp(mtab[i].device, rdev) || strcmp(mtab[i].fstype, rtype))
		{
			root_mismatch(mtab[i].device, mtab[i].fstype);
			return;
		}
	}
}

int main()
{
	if (win_attach())
		err(255, NULL);
	umask(077);
	
	check_devs();
	check_root();
	return xit;
}
