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
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <os386.h>
#include <paths.h>
#include <sha.h>

#define PASSWD_PATH	"/etc/passwd"
#define NPASSWD_PATH	"/etc/passwd-new"
#define OPASSWD_PATH	"/etc/passwd-old"
#define SECRET_PATH	"/etc/passwd-secret"
#define NSECRET_PATH	"/etc/passwd-secret-new"
#define OSECRET_PATH	"/etc/passwd-secret-old"
#define LOCK_PATH	"/etc/passwd-lock"

#define PASSWD_BUF	4096
#define PW_HASH_LEN	(SHA_LENGTH * 2)
#define PW_LEN		63

typedef char passwd_buf_t[256];
typedef char passwd_shabuf_t[SHA256_LENGTH * 2 + 1];

static FILE *		passwd_file;
static passwd_buf_t	passwd_buf;
static struct passwd	passwd_st;
static passwd_shabuf_t	passwd_shabuf;

static void pw_lock(void)
{
restart:
	if (mknod(LOCK_PATH, S_IFREG | 0600, 0))
	{
		sleep(1);
		goto restart;
	}
}

static void pw_unlock(void)
{
	int saved_errno;
	
	saved_errno = _get_errno();
	unlink(LOCK_PATH);
	_set_errno(saved_errno);
}

static int parse_pwent(struct passwd *pw, char *s)
{
	char *p;
	
	p = strchr(p, '\n');
	if (p)
		*p = 0;
	
	pw->pw_name = s;
	p = strchr(s, ':');
	if (!p)
		goto fail;
	*p = 0;
	p = strchr(p + 1, ':');
	if (!p)
		goto fail;
	p++;
	
	pw->pw_uid = strtoul(p, &p, 0);
	if (*p != ':')
		goto fail;
	p++;
	
	pw->pw_gid = strtoul(p, &p, 0);
	if (*p != ':')
		goto fail;
	p++;
	
	pw->pw_gecos = p;
	p = strchr(p, ':');
	if (!p)
		goto fail;
	*p = 0;
	p++;
	
	pw->pw_dir = p;
	p = strchr(p, ':');
	if (!p)
		goto fail;
	*p = 0;
	p++;
	
	pw->pw_shell = p;
	return 0;

fail:
	_sysmesg("WARNING: malformed password file\n");
	_set_errno(EINVAL);
	return -1;
}

static FILE *create_file(char *pathname, mode_t mode)
{
	FILE *f;
	int fd;
	
	fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, 0600);
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

struct passwd *getpwent(void)
{
	if (!passwd_file)
	{
		passwd_file = fopen(PASSWD_PATH, "r");
		if (!passwd_file)
			return NULL;
		fcntl(fileno(passwd_file), F_SETFD, FD_CLOEXEC);
	}
	
	if (!get_line(passwd_buf, sizeof passwd_buf, passwd_file))
		return NULL;
	if (parse_pwent(&passwd_st, passwd_buf))
		return NULL;
	return &passwd_st;
}

void setpwent(void)
{
	if (passwd_file)
		fseek(passwd_file, 0, SEEK_SET);
}

void endpwent(void)
{
	if (passwd_file)
	{
		fclose(passwd_file);
		passwd_file = NULL;
	}
}

struct passwd *getpwnam(const char *name)
{
	struct passwd *pw;
	
	setpwent();
	while ((pw = getpwent()))
		if (!strcmp(pw->pw_name, name))
			return pw;
	_set_errno(ENOENT);
	return NULL;
}

struct passwd *getpwuid(uid_t uid)
{
	struct passwd *pw;
	
	setpwent();
	while ((pw = getpwent()))
		if (pw->pw_uid == uid)
			return pw;
	_set_errno(ENOENT);
	return NULL;
}

int _newuser(const char *name, uid_t uid, gid_t gid)
{
	char buf[PASSWD_BUF];
	FILE *f;
	int len;
	
	if (!*name)
	{
		_set_errno(EINVAL);
		return -1;
	}
	
	if (geteuid())
	{
		_set_errno(EPERM);
		return -1;
	}
	pw_lock();
	
	len = strlen(name);
	f = fopen(PASSWD_PATH, "r+");
	if (!f)
	{
		pw_unlock();
		return -1;
	}
	while (get_line(buf, sizeof buf, f))
		if (!strncmp(buf, name, len) && buf[len] == ':')
		{
			_set_errno(EEXIST);
			fclose(f);
			pw_unlock();
			return -1;
		}
	fprintf(f, "%s:x:%li:%li::/:%s\n", name, (long)uid, (long)gid, _PATH_B_DEFSHELL);
	fclose(f);
	pw_unlock();
	return 0;
}

int _deluser(const char *name)
{
	int len = strlen(name);
	char buf[PASSWD_BUF];
	FILE *fi = NULL;
	FILE *fo = NULL;
	
	if (geteuid())
	{
		_set_errno(EPERM);
		return -1;
	}
	pw_lock();
	
	fi = fopen(SECRET_PATH, "r");
	if (!fi)
		goto fail;
	fo = create_file(NSECRET_PATH, 0600);
	if (!fo)
		goto fail;
	
	while (get_line(buf, sizeof buf, fi))
		if (strncmp(buf, name, len) || buf[len] != ':')
			fprintf(fo, "%s\n", buf);
	fclose(fi);
	fclose(fo);
	fi = NULL;
	fo = NULL;
	
	if (rename(SECRET_PATH, OSECRET_PATH))
		goto fail;
	if (rename(NSECRET_PATH, SECRET_PATH))
		goto fail;
	
	fi = fopen(PASSWD_PATH, "r");
	if (!fi)
		goto fail;
	fo = create_file(NPASSWD_PATH, 0644);
	if (!fo)
		goto fail;
	
	while (get_line(buf, sizeof buf, fi))
		if (strncmp(buf, name, len) || buf[len] != ':')
			fprintf(fo, "%s\n", buf);
	
	fclose(fi);
	fclose(fo);
	
	if (rename(PASSWD_PATH, OPASSWD_PATH))
		goto fail;
	if (rename(NPASSWD_PATH, PASSWD_PATH))
		goto fail;
	
	pw_unlock();
	return 0;
fail:
	if (fi)
		fclose(fi);
	if (fo)
		fclose(fo);
	pw_unlock();
	return -1;
}

#define USER_MOD_FUNC(func_name, param_type, field_type, field)			\
	int func_name(const char *name, param_type param)			\
	{									\
		char buf[PASSWD_BUF];						\
		struct passwd pw;						\
		FILE *fi = NULL;						\
		FILE *fo = NULL;						\
		int found = 0;							\
										\
		if (geteuid())							\
		{								\
			_set_errno(EPERM);					\
			return -1;						\
		}								\
		pw_lock();							\
										\
		fi = fopen(PASSWD_PATH, "r");					\
		if (!fi)							\
			goto fail;						\
		fo = create_file(NPASSWD_PATH, 0644);				\
		if (!fo)							\
			goto fail;						\
										\
		while (get_line(buf, sizeof buf, fi))				\
		{								\
			if (parse_pwent(&pw, buf))				\
				goto fail;					\
			if (!strcmp(pw.pw_name, name))				\
			{							\
				pw.field = (field_type)param;			\
				found = 1;					\
			}							\
										\
			fprintf(fo, "%s:x:%li:%li:%s:%s:%s\n",			\
				pw.pw_name, (long)pw.pw_uid, (long)pw.pw_gid,	\
				pw.pw_gecos, pw.pw_dir, pw.pw_shell);		\
		}								\
										\
		fclose(fi);							\
		fclose(fo);							\
		fi = NULL;							\
		fo = NULL;							\
										\
		if (rename(PASSWD_PATH, OPASSWD_PATH))				\
			goto fail;						\
		if (rename(NPASSWD_PATH, PASSWD_PATH))				\
			goto fail;						\
		pw_unlock();							\
		if (!found)							\
		{								\
			_set_errno(ENOENT);					\
			return -1;						\
		}								\
		return 0;							\
	fail:									\
		if (fi)								\
			fclose(fi);						\
		if (fo)								\
			fclose(fo);						\
		pw_unlock();							\
		return -1;							\
	}

USER_MOD_FUNC(_sethome, const char *, char *, pw_dir)
USER_MOD_FUNC(_setshell, const char *, char *, pw_shell)

static const char *hash_pass(const char *name, const char *pass)
{
	uint8_t ibuf[LOGNAME_MAX + PASSWD_MAX + 2];
	uint8_t buf8[SHA256_LENGTH];
	struct sha256 *sha;
	uint8_t *p;
	int i;
	
	if (strlen(name) > LOGNAME_MAX || strlen(pass) > PASSWD_MAX)
	{
		_set_errno(EINVAL);
		return NULL;
	}
	sprintf((char *)ibuf, "%s/%s", name, pass);
	
	sha = sha256_creat(); /* XXX */
	if (!sha)
		return NULL;
	sha256_init(sha, strlen(name) + strlen(pass));
	for (p = ibuf; p < ibuf + sizeof ibuf; p += SHA256_CHUNK)
		sha256_feed(sha, p);
	sha256_read(sha, buf8);
	sha256_free(sha);
	for (i = 0; i < sizeof buf8; i++)
		sprintf(passwd_shabuf + 2 * i, "%02x", buf8[i]);
	return passwd_shabuf;
}

int _setpass(const char *name, const char *pass)
{
	char buf[PASSWD_BUF];
	FILE *fi = NULL;
	FILE *fo = NULL;
	int len = strlen(name);
	int found = 0;
	const char *hash;
	
	hash = hash_pass(name, pass);
	if (!hash)
		return -1;
	
	if (geteuid())
	{
		_set_errno(EPERM);
		return -1;
	}
	
	pw_lock();
	
	fi = fopen(SECRET_PATH, "r");
	if (!fi)
		goto fail;
	fo = create_file(NSECRET_PATH, 0600);
	if (!fo)
		goto fail;
	
	while (get_line(buf, sizeof buf, fi))
	{
		if (!strncmp(buf, name, len) && buf[len] == ':')
		{
			fprintf(fo, "%s:%s\n", name, hash);
			found = 1;
		}
		else
			fprintf(fo, "%s\n", buf);
	}
	if (!found)
		fprintf(fo, "%s:%s\n", name, hash);
	fclose(fi);
	fclose(fo);
	fi = NULL;
	fo = NULL;
	
	if (rename(SECRET_PATH, OSECRET_PATH))
		goto fail;
	if (rename(NSECRET_PATH, SECRET_PATH))
		goto fail;
	
	pw_unlock();
	return 0;
fail:
	if (fi)
		fclose(fi);
	if (fo)
		fclose(fo);
	pw_unlock();
	return -1;
}

int _chkpass(const char *name, const char *pass)
{
	char buf[PASSWD_BUF];
	int len = strlen(name);
	const char *hash;
	FILE *f;
	
	hash = hash_pass(name, pass);
	if (!hash)
		return -1;
	
	f = fopen(SECRET_PATH, "r");
	if (!f)
		return -1;
	
	while (get_line(buf, sizeof buf, f))
	{
		if (strncmp(buf, name, len))
			continue;
		if (buf[len] != ':')
			continue;
		if (strcmp(buf + len + 1, hash))
		{
			_set_errno(EACCES);
			fclose(f);
			return -1;
		}
		fclose(f);
		return 0;
	}
	_set_errno(EACCES);
	fclose(f);
	return -1;
}

USER_MOD_FUNC(_setgroup, gid_t, gid_t, pw_gid)
