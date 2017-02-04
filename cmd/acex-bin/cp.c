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

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <err.h>

static int xcode;

static int confirm  = 1;
static int recur    = 0;
static int preserve = 0;
static int verbose  = 0;

static int do_chown = 1;

static char *basename(const char *name);
static void  procopt(int argc, char **argv);
static int   overwrite(const char *name);
static void  copyattr(const char *src, const char *dst);
static int   do_cpdir(const char *src, const char *dst, const struct stat *st);
static void  do_cp(const char *src, const char *dst);
static void  usage(void);

static char *basename(const char *name)
{
	char *s;
	
	s = strrchr(name, '/');
	if (s)
		return s + 1;
	return (char *)name;
}

static char *scwd(const char *name)
{
	char *p;
	
	p = (char *)name;
	while (p[0] == '.' && p[1] == '/')
		p += 2;
	return p;
}

static void copyattr(const char *src, const char *dst) /* XXX */
{
	mode_t prevmask;
	struct stat st;
	
	prevmask = umask(0); /* XXX */
	
	if (stat(src, &st))
	{
		if (!xcode)
			xcode = errno;
		warn("%s: stat", scwd(src));
		return;
	}
	
	if (do_chown)
	{
		if (chown(dst, st.st_uid, st.st_gid))
		{
			if (!xcode)
				xcode = errno;
			st.st_mode &= ~(S_ISUID | S_ISGID);
			warn("%s: chown", scwd(dst));
		}
	}
	else
		st.st_mode &= ~(S_ISUID | S_ISGID);
	
	if (chmod(dst, st.st_mode))
	{
		if (!xcode)
			xcode = errno;
		warn("%s: chmod", scwd(dst));
	}
	
	umask(prevmask); /* XXX */
}

static int overwrite(const char *name)
{
	if (confirm)
	{
		char buf[MAX_CANON];
		int cnt;
		
		do
		{
			fprintf(stderr, "Overwrite %s? ", scwd(name));
			
			cnt = read(STDIN_FILENO, buf, MAX_CANON);
			if (!cnt)
			{
				if (!xcode)
					xcode = 255;
				return -1;
			}
			if (cnt < 0)
			{
				if (!xcode)
					xcode = errno;
				perror("cp: stdin");
				return -1;
			}
		}
		while (cnt != 2);
		
		if (*buf != 'y')
			return -1;
	}
	
	if (remove(name))
	{
		if (!xcode)
			xcode = errno;
		warn("%s: remove", scwd(name));
		return -1;
	}
	return 0;
}

static int do_cpdir(const char *src, const char *dst, const struct stat *st)
{
	struct dirent *de;
	DIR *dir;
	
	if (mkdir(dst, st->st_mode) && errno != EEXIST)
	{
		if (!xcode)
			xcode = errno;
		warn("%s: mkdir", scwd(dst));
		return 0;
	}
	
	dir = opendir(src);
	if (!dir)
	{
		if (!xcode)
			xcode = errno;
		warn("%s: opendir", scwd(src));
		return 1;
	}
	
	errno = 0;
	while ((de = readdir(dir)))
	{
		char srcfile[PATH_MAX];
		char dstfile[PATH_MAX];
		
		if (!strcmp(de->d_name, "."))
			continue;
		if (!strcmp(de->d_name, ".."))
			continue;
		
		if (strlen(de->d_name) + strlen(src) + 1 >= sizeof srcfile)
		{
			if (!xcode)
				xcode = ENAMETOOLONG;
			warnx("%s/%s: Name too long", src, de->d_name);
			continue;
		}
		
		if (strlen(de->d_name) + strlen(src) + 1 >= sizeof dstfile)
		{
			if (!xcode)
				xcode = ENAMETOOLONG;
			warnx("%s/%s: Name too long", dst, de->d_name);
			continue;
		}
		
		sprintf(srcfile, "%s/%s", src, de->d_name);
		sprintf(dstfile, "%s/%s", dst, de->d_name);
		do_cp(srcfile, dstfile);
		errno=0;
	}
	if (errno)
	{
		if (!xcode)
			xcode = errno;
		warn("%s: readdir", scwd(src));
	}
	closedir(dir);
	return 1;
}

static int do_cpreg(const char *src, const char *dst)
{
	struct stat st;
	char buf[512];
	int srcfd;
	int dstfd;
	int cnt;
	
	srcfd = open(src, O_RDONLY);
	if (srcfd < 0)
	{
		if (!xcode)
			xcode = errno;
		warn("%s: open", scwd(src));
		return 0;
	}
	
	if (fstat(srcfd, &st))
	{
		if (!xcode)
			xcode = errno;
		warn("%s: fstat", scwd(src));
		close(srcfd);
		return 0;
	}
	
creat:
	if (preserve)
		dstfd = open(dst, O_WRONLY | O_CREAT | O_EXCL, S_IWUSR);
	else
		dstfd = open(dst, O_WRONLY | O_CREAT | O_EXCL, 0666);
	if (dstfd < 0)
	{
		if (errno == EEXIST)
		{
			if (overwrite(dst))
			{
				close(srcfd);
				return 0;
			}
			goto creat;
		}
		if (!xcode)
			xcode = errno;
		warn("%s: open", scwd(dst));
		close(srcfd);
		return 0;
	}
	
	while (cnt = read(srcfd, buf, sizeof buf), cnt)
	{
		if (cnt < 0)
		{
			if (!xcode)
				xcode = errno;
			warn("%s: read", scwd(src));
			goto clean;
		}
		
		if (write(dstfd, buf, cnt) != cnt)
		{
			if (!xcode)
				xcode = errno;
			warn("%s: write", scwd(dst));
			goto clean;
		}
	}
	
clean:
	close(srcfd);
	if (close(dstfd))
	{
		if (!xcode)
			xcode = errno;
		warn("%s: close", scwd(dst));
	}
	return 1;
}

static void do_cp(const char *src, const char *dst)
{
	struct stat st;
	int x = 1;
	
	if (verbose)
		printf("'%s' -> '%s'\n", scwd(src), scwd(dst));
	
	if (stat(src, &st))
	{
		if (!xcode)
			xcode = errno;
		warn("%s: stat", scwd(src));
		return;
	}
	
	if (recur)
	{
		switch (st.st_mode & S_IFMT)
		{
		case S_IFCHR:
		case S_IFBLK:
		case S_IFIFO:
			if (mknod(dst, st.st_mode, st.st_rdev))
			{
				if (!xcode)
					xcode = errno;
				warn("%s: mknod", scwd(dst));
				return;
			}
			break;
		case S_IFDIR:
			x = do_cpdir(src, dst, &st);
			break;
		case S_IFREG:
			x = do_cpreg(src, dst);
			break;
		default:
			warnx("%s: invalid inode type", scwd(src));
			if (!xcode)
				xcode = EINVAL;
			return;
		}
	}
	else
		x = do_cpreg(src, dst);
	
	if (preserve && x)
		copyattr(src, dst);
}

static void procopt(int argc, char **argv)
{
	int opt;
	
	while (opt = getopt(argc, argv, "firRpv"), opt > 0)
		switch (opt)
		{
		case 'f':
			confirm = 0;
			break;
		case 'i':
			confirm = 1;
			break;
		case 'r':
		case 'R':
			recur	 = 1;
			// preserve = 1;
			break;
		case 'p':
			preserve = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			exit(255);
		}
}

static void usage(void)
{
	printf("\nUsage: cp [-firRpv] SOURCE... DEST\n\n"
		"Copy SOURCE to DESTINATION\n\n"
		" -f  force (do not ask for confirmation)\n"
		" -i  interactive mode (ask for confirmation)\n"
		" -r  process directiories recursively\n"
		" -R  same as -r\n"
		" -p  preserve mode and owner uid/gid\n"
		" -v  verbose mode (show what is being copied)\n\n"
		);
	exit(0);
}

int main(int argc, char **argv)
{
	int dest_dir = 0;
	struct stat st;
	char *dest;
	int i;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
		usage();
	procopt(argc, argv);
	
	if (optind > argc - 2)
		errx(255, "too few arguments");
	
	if (geteuid())
		do_chown = 0;
	
	dest = argv[argc - 1];
	if (stat(dest, &st))
	{
		if (errno != ENOENT)
			err(errno, "%s: stat", scwd(dest));
	}
	else if (S_ISDIR(st.st_mode))
		dest_dir = 1;
	
	for (i = optind; i < argc - 1; i++)
	{
		char buf[PATH_MAX];
		
		if (dest_dir)
		{
			strcpy(buf, dest);
			strcat(buf, "/");
			strcat(buf, basename(argv[i]));
		}
		else
			strcpy(buf, dest);
		
		do_cp(argv[i], buf);
	}
	return xcode;
}
