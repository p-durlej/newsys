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

#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <mount.h>
#include <paths.h>

static struct _mtab mtab[MOUNT_MAX];

static void mkicon(const char *name, const char *path)
{
	char pathname[PATH_MAX];
	FILE *f;
	
	sprintf(pathname, "%s/%s", _PATH_P_MICONS, name);
	if (!access(pathname, 0))
		return;
	
	f = fopen(pathname, "w");
	
	fprintf(f, "# SHLINK\n\n");
	fprintf(f, "exec=%s\n", path);
	fprintf(f, "icon=floppy.pnm\n");
	
	fclose(f);
}

int main()
{
	struct dirent *de;
	DIR *d;
	int i;
	
	memset(&mtab, 0, sizeof mtab);
	if (_mtab(mtab, sizeof mtab))
		err(1, "_mtab");
	
	umask(022);
	
	for (i = 0; i < MOUNT_MAX; i++)
	{
		struct _mtab *m = &mtab[i];
		
		if (!m->mounted || !(m->flags & MF_REMOVABLE))
			continue;
		
		if (!*m->prefix) /* skip the root fs */
			continue;
		
		mkicon(m->device, m->prefix);
	}
	
	d = opendir(_PATH_P_MICONS);
	if (!d)
		err(1, "%s", _PATH_P_MICONS);
	
	while (de = readdir(d), de)
	{
		int i;
		
		if (*de->d_name == '.')
			continue;
		
		for (i = 0; i < MOUNT_MAX; i++)
		{
			struct _mtab *m = &mtab[i];
			
			if (m->mounted && !strcmp(de->d_name, m->device))
				break;
		}
		
		if (i >= MOUNT_MAX)
		{
			char pathname[PATH_MAX];
			
			sprintf(pathname, "%s/%s", _PATH_P_MICONS, de->d_name);
			unlink(pathname);
		}
	}
	
	closedir(d);
	
	return 0;
}
