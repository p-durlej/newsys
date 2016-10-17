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
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <grp.h>
#include <pwd.h>
#include <err.h>

int main(int argc, char **argv)
{
	struct passwd *pw;
	struct group *gr;
	struct stat st;
	char *group;
	char *user;
	uid_t uid = -1;
	gid_t gid = -1;
	int mchgrp = 0;
	int xcode = 0;
	char *p;
	int fd;
	int i;
	
	p = strchr(argv[0], '/');
	if (p == NULL)
		p = argv[0];
	else
		p++;
	
	if (!strcmp(p, "chgrp"))
		mchgrp = 1;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		printf("\nUsage: chown UID[:GID] FILE...\n"
			"       chgrp      GID  FILE...\n"
			"Set the owner of FILEs to specified UID and possibly GID.\n\n"
			);
		return 0;
	}
	
	if (argc < 3)
		errx(255, "too few arguments");
	
	if (mchgrp)
	{
		group = argv[1];
		user  = NULL;
	}
	else
	{
		user  = argv[1];
		group = strchr(user, ':');
		if (group)
		{
			*group = 0;
			group++;
		}
	}
	
	if (group != NULL)
	{
		if (isdigit(*group))
			gid = atoi(group);
		else
		{
			gr = getgrnam(group);
			if (gr == NULL)
				errx(errno, "group %s not found", group);
			gid = gr->gr_gid;
		}
	}
	
	if (user != NULL)
	{
		if (isdigit(*user))
			uid = atoi(user);
		else
		{
			pw = getpwnam(user);
			if (pw == NULL)
				errx(errno, "user %s not found", user);
			uid = pw->pw_uid;
		}
	}
	
	for (i = 2; i < argc; i++)
	{
		fd = open(argv[i], O_NOIO);
		if (fd < 0)
		{
			xcode = errno;
			warn("%s", argv[i]);
			continue;
		}
		
		if (user == NULL || group == NULL)
		{
			if (fstat(fd, &st))
			{
				xcode = errno;
				warn("%s", argv[i]);
				close(fd);
				continue;
			}
			
			if (user == NULL)
				uid = st.st_uid;
			if (group == NULL)
				gid = st.st_gid;
		}
		
		if (fchown(fd, uid, gid))
		{
			xcode = errno;
			warn("%s", argv[i]);
		}
		
		close(fd);
	}
	return xcode;
}
