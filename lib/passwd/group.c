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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <os386.h>

#define GROUP_PATH	"/etc/group"
#define NGROUP_PATH	"/etc/group-new"
#define OGROUP_PATH	"/etc/group-old"
#define LOCK_PATH	"/etc/passwd-lock"

#define GROUP_BUF	256

typedef char group_buf_t[GROUP_BUF];

static FILE *		group_file;
static group_buf_t	group_buf;
static struct group	group_st;

static void gr_lock(void)
{
restart:
	if (mknod(LOCK_PATH, S_IFREG | 0600, 0))
	{
		sleep(1);
		goto restart;
	}
}

static void gr_unlock(void)
{
	int saved_errno;
	
	saved_errno = _get_errno();
	unlink(LOCK_PATH);
	_set_errno(saved_errno);
}

static int parse_grent(struct group *gr, char *s)
{
	char *p;
	
	p = strchr(p, '\n');
	if (p)
		*p = 0;
	
	gr->gr_name = s;
	p = strchr(s, ':');
	if (!p)
		goto fail;
	*p = 0;
	p = strchr(p + 1, ':');
	if (!p)
		goto fail;
	p++;
	
	gr->gr_gid = atol(p);
	return 0;

fail:
	_sysmesg("WARNING: malformed group file\n");
	_set_errno(EINVAL);
	return -1;
}

static FILE *create_file(char *pathname, mode_t mode)
{
	FILE *f;
	int fd;
	
	fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, mode);
	if (fd < 0)
		return NULL;
	
	if (fchmod(fd, mode))
	{
		close(fd);
		return NULL;
	}
	
	f = fdopen(fd, "w");
	if (!f)
		close(fd);
	
	return f;
}

static char *get_line(char *buf, size_t len, FILE *f)
{
	char *p;
	
	do
	{
		if (!fgets(buf, len, f))
			return NULL;
		p = strchr(buf, '\n');
		if (p)
			*p = 0;
	}
	while (!*buf);
	return buf;
}

struct group *getgrent(void)
{
	if (!group_file)
	{
		group_file = fopen(GROUP_PATH, "r");
		if (!group_file)
			return NULL;
		fcntl(fileno(group_file), F_SETFD, FD_CLOEXEC);
	}
	if (!get_line(group_buf, sizeof group_buf, group_file))
		return NULL;
	if (parse_grent(&group_st, group_buf))
		return NULL;
	return &group_st;
}

void setgrent(void)
{
	if (group_file)
		fseek(group_file, 0, SEEK_SET);
}

void endgrent(void)
{
	if (group_file)
	{
		fclose(group_file);
		group_file = NULL;
	}
}

struct group *getgrnam(const char *name)
{
	struct group *gr;
	
	setgrent();
	while ((gr = getgrent()))
		if (!strcmp(name, gr->gr_name))
			return gr;
	return NULL;
}

struct group *getgrgid(gid_t gid)
{
	struct group *gr;
	
	setgrent();
	while ((gr = getgrent()))
		if (gid == gr->gr_gid)
			return gr;
	return NULL;
}

int _newgroup(const char *name, gid_t gid)
{
	char buf[GROUP_BUF];
	struct group gr;
	FILE *f;
	
	if (!*name)
	{
		_set_errno(EINVAL);
		return -1;
	}
	
	endgrent();
	gr_lock();
	
	f = fopen(GROUP_PATH, "r+");
	if (!f)
	{
		gr_unlock();
		return -1;
	}
	while (get_line(buf, sizeof buf, f))
	{
		if (parse_grent(&gr, buf))
		{
			gr_unlock();
			return -1;
		}
		if (!strcmp(gr.gr_name, name))
		{
			_set_errno(EEXIST);
			fclose(f);
			gr_unlock();
			return -1;
		}
	}
	fprintf(f, "%s:x:%li:\n", name, (long)gid);
	fclose(f);
	gr_unlock();
	return 0;
}

int _delgroup(const char *name)
{
	int len = strlen(name);
	char buf[GROUP_BUF];
	FILE *fi;
	FILE *fo;
	
	endgrent();
	gr_lock();
	
	fi = fopen(GROUP_PATH, "r");
	if (!fi)
	{
		gr_unlock();
		return -1;
	}
	fo = create_file(NGROUP_PATH, 0644);
	if (!fo)
	{
		fclose(fi);
		gr_unlock();
		return -1;
	}
	while (get_line(buf, sizeof buf, fi))
		if (strncmp(buf, name, len) || buf[len] != ':')
			fprintf(fo, "%s\n", buf);
	fclose(fi);
	fclose(fo);
	
	if (rename(GROUP_PATH, OGROUP_PATH))
	{
		gr_unlock();
		return -1;
	}
	if (rename(NGROUP_PATH, GROUP_PATH))
	{
		gr_unlock();
		return -1;
	}
	gr_unlock();
	return 0;
}
