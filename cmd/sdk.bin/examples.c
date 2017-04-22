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
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <paths.h>
#include <errno.h>
#include <err.h>

#define SPATH	"/usr/examples"

static char dpath[PATH_MAX];

static void install(const char *name)
{
	static char buf[32768];
	char spn[PATH_MAX];
	char dpn[PATH_MAX];
	int sfd, dfd;
	ssize_t cnt;
	
	sprintf(spn, "%s/%s", SPATH, name);
	sprintf(dpn, "%s/%s", dpath, name);
	
	sfd = open(spn, O_RDONLY);
	if (sfd < 0)
		return;
	
	dfd = open(dpn, O_WRONLY | O_CREAT | O_EXCL, 0666);
	if (dfd < 0)
		goto clean;
	
	while (cnt = read(sfd, buf, sizeof buf), cnt)
		if (write(dfd, buf, cnt) != cnt)
		{
			unlink(dpn);
			goto clean;
		}
clean:
	close(sfd);
	close(dfd);
}

int main(int argc, char **argv)
{
	const char *home = getenv("HOME");
	struct dirent *de;
	DIR *d;
	
	if (win_attach())
		err(1, NULL);
	
	if (!home)
	{
		msgbox_perror(NULL, "Examples", "You have no $HOME.", errno);
		return 1;
	}
	
	sprintf(dpath, "%s/.examples", home);
	if (mkdir(dpath, 0755) && errno != EEXIST)
	{
		msgbox_perror(NULL, "Examples", "Cannot create directory", errno);
		return 1;
	}
	install(".dirinfo");
	
	d = opendir(SPATH);
	if (!d)
	{
		msgbox_perror(NULL, "Examples", "Cannot open " SPATH, errno);
		return 1;
	}
	
	while (de = readdir(d), de)
	{
		if (*de->d_name == '.')
			continue;
		
		install(de->d_name);
	}
	
	closedir(d);
	
	execl(_PATH_B_FILEMGR, _PATH_B_FILEMGR, dpath, (void *)NULL);
	msgbox_perror(NULL, "Examples", "Cannot run filemgr", errno);
	return 1;
}
