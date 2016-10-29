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

#include <config/defaults.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

#include "filemgr.h"

#define	DB_PATH		"/etc/filetype/"

static char *getft(const char *pathname)
{
	unsigned char buf[1024];
	int cnt;
	int fd;
	int i;
	
	fd = open(pathname, O_RDONLY);
	if (fd < 0)
		return "default";
	
	cnt = read(fd, buf, sizeof buf);
	close(fd);
	
	if (cnt < 0)
		return "default";
	
	if (cnt > 9 && !memcmp(buf, "# SHLINK\n", 9))
		return "shlink";
	
	for (i = 0; i < cnt; i++)
		if (buf[i] < 0x20 && buf[i] != '\n' && buf[i] != '\t' && buf[i] != '\r')
		    	return "default";
	
	return "ext-txt";
}

static int find_icon(char *buf, size_t sz)
{
#define THEMEDIR	"/lib/w_themes/"
#define ICON64DIR	"/lib/icons64/"
#define ICONDIR		"/lib/icons/"
	char pathname[PATH_MAX];
	char theme[NAME_MAX + 1];
	int cnt;
	
	if (*buf == '/')
		return 0;
	
	if (c_load("/user/w_theme", theme, sizeof theme))
		strcpy(theme, DEFAULT_THEME);
	
	if (*theme)
	{
		theme[sizeof theme - 1] = 0;
		
		if (config.large_icons)
		{
			cnt = snprintf(pathname, sizeof pathname, THEMEDIR "%s/icons64/%s", theme, buf);
			if (cnt < 0 || cnt >= sizeof pathname)
				goto namelong;
			
			if (!access(pathname, R_OK))
				goto found;
		}
		
		cnt = snprintf(pathname, sizeof pathname, THEMEDIR "%s/icons/%s", theme, buf);
		if (cnt < 0 || cnt >= sizeof pathname)
			goto namelong;
		
		if (!access(pathname, R_OK))
			goto found;
	}
	
	if (config.large_icons)
	{
		cnt = snprintf(pathname, sizeof pathname, ICON64DIR "%s", buf);
		if (cnt < 0 || cnt >= sizeof pathname)
			goto namelong;
		
		if (!access(pathname, R_OK))
			goto found;
	}
	
	cnt = snprintf(pathname, sizeof pathname, ICONDIR "%s", buf);
	if (cnt < 0 || cnt >= sizeof pathname)
		goto namelong;
	
	if (!access(pathname, R_OK))
		goto found;
	
	errno = ENOENT;
	return -1;
namelong:
	errno = ENAMETOOLONG;
	return -1;
found:
	if (strlen(pathname) >= sz)
		goto namelong;
	strcpy(buf, pathname);
	return 0;
#undef THEMEDIR
#undef ICON64DIR
#undef ICONDIR
}

int ft_getinfo(struct file_type *ft, const char *pathname)
{
	char ftdn[PATH_MAX];
	char line[1024];
	struct stat st;
	char *type;
	char *nl;
	FILE *f;
	
	memset(ft, 0, sizeof *ft);
	strcpy(ft->icon, DEFAULT_ICON);
	
	if (stat(pathname, &st))
	{
		type = "default";
		goto skip;
	}
	
	switch (st.st_mode & S_IFMT)
	{
	case S_IFDIR:
		type = "dir";
		break;
	case S_IFBLK:
		type = "bdev";
		break;
	case S_IFCHR:
		type = "cdev";
		break;
	case S_IFIFO:
		type = "fifo";
		break;
	case S_IFSOCK:
		type = "sock";
		break;
	case S_IFREG:
		if (st.st_mode & 0111)
			type = "exec";
		else
			type = strrchr(pathname, '.');
			if (!type)
				type = getft(pathname);
		break;
	default:
		type = "default";
		break;
	}
	
	if (sizeof DB_PATH + strlen(type) + 4 >= sizeof ftdn)
		type = getft(pathname);
	
skip:
	strcpy(ftdn, DB_PATH);
	if (*type == '.')
	{
		strcat(ftdn, "ext-");
		strcat(ftdn, type + 1);
	}
	else
		strcat(ftdn, type);
	
	if (access(ftdn, R_OK))
	{
		type = getft(pathname);
		strcpy(ftdn, DB_PATH);
		strcat(ftdn, type);
	}
	
	f = fopen(ftdn, "r");
	if (!f)
	{
		strcpy(ftdn, DB_PATH "default");
		type = "default";
		
		f = fopen(ftdn, "r");
		if (!f)
			return -1;
	}
	
	while (fgets(line, sizeof line, f))
	{
		nl = strchr(line, '\n');
		
		if (nl)
			*nl = 0;
		
		if (strlen(line) > 5)
		{
			if (!memcmp(line, "desc=", 5))
			{
				strncpy(ft->desc, line + 5, sizeof ft->desc);
				ft->desc[sizeof ft->desc - 1] = 0;
			}
			if (!memcmp(line, "icon=", 5))
			{
				strncpy(ft->icon, line + 5, sizeof ft->icon);
				ft->icon[sizeof ft->icon - 1] = 0;
			}
			if (!memcmp(line, "exec=", 5))
			{
				strncpy(ft->exec, line + 5, sizeof ft->exec);
				ft->exec[sizeof ft->exec - 1] = 0;
			}
		}
	}
	
	strcpy(ft->type, type);
	fclose(f);
	
	if (!strcmp(type, "shlink"))
	{
		f = fopen(pathname, "r");
		if (!f)
			return 0;
		
		while (fgets(line, sizeof line, f))
		{
			nl = strchr(line, '\n');
			if (nl)
				*nl = 0;
			
			if (strlen(line) > 5 && !memcmp(line, "icon=", 5))
			{
				strncpy(ft->icon, line + 5, sizeof ft->icon);
				ft->icon[sizeof ft->icon - 1] = 0;
			}
		}
		
		fclose(f);
	}
	if (find_icon(ft->icon, sizeof ft->icon))
		return -1;
	
	return 0;
}
