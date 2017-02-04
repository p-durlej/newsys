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

#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <err.h>

#define SHELLS	"/etc/shells"

static void chk_shell(const char *sh)
{
	static char buf[PATH_MAX];
	
	char *p;
	FILE *f;
	
	f = fopen(SHELLS, "r");
	while (fgets(buf, sizeof buf, f))
	{
		p = strchr(buf, '\n');
		if (p != NULL)
			*p = 0;
		
		if (!strcmp(sh, buf))
		{
			fclose(f);
			return;
		}
	}
	errx(127, "%s: Not in " SHELLS, sh);
}

int main(int argc, char **argv)
{
	static char buf[PATH_MAX];
	
	const char *newsh = NULL;
	const char *user  = NULL;
	struct passwd *pw;
	uid_t ruid;
	char *p;
	int c;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		printf("\nUsage: %s [-s SHELL] [user]\n\n"
		       "Change user's shell.\n\n"
		       "  -s change the user's shell to SHELL\n\n",
		       argv[0]);
		return 0;
	}
	ruid = getuid();
	
	while (c = getopt(argc, argv, "s:"), c > 0)
		switch (c)
		{
		case 's':
			newsh = optarg;
			break;
		default:
			return 127;
		}
	
	if (optind < argc)
	{
		if (ruid)
			errx(127, "You are not root");
		user = argv[optind];
	}
	else
	{
		pw = getpwuid(ruid);
		if (pw == NULL)
			errx(127, "No account");
		
		user = strdup(pw->pw_name);
		if (user == NULL)
			err(errno, "strdup");
	}
	
	if (newsh == NULL)
	{
		printf("New shell for %s: ", user);
		if (fgets(buf, sizeof buf, stdin) == NULL)
		{
			if (ferror(stdin))
				err(errno, "stdin");
			return 0;
		}
		p = strchr(buf, '\n');
		if (p != NULL)
			*p = 0;
		newsh = buf;
	}
	
	if (ruid)
		chk_shell(newsh);
	
	if (_setshell(user, newsh))
		err(errno, "_setshell");
	if (access(newsh, X_OK))
		warn("warning: %s", newsh);
	return 0;
}
