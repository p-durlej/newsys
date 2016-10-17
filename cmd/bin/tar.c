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

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <err.h>
#include <tar.h>

static const char *tar_pathname = "/dev/rdsk/fd0,0";

static const char **members;

static int pflag;
static int vflag;
static int fflag;
static int mode;

static int xit;

struct namenode
{
	struct namenode *next;
	ino_t		 ino;
	dev_t		 dev;
	char		 pathname[PATH_MAX];
} *namenodes;

static void store1(int fd, const char *pathname);

static void add_namenode(const char *pathname, dev_t dev, ino_t ino)
{
	struct namenode *nn;
	
	nn = malloc(sizeof *nn);
	if (!nn)
		err(1, "malloc");
	
	strcpy(nn->pathname, pathname);
	nn->ino = ino;
	nn->dev = dev;
	
	nn->next = namenodes;
	namenodes = nn;
}

static struct namenode *find_namenode(dev_t dev, ino_t ino)
{
	struct namenode *nn;
	
	for (nn = namenodes; nn; nn = nn->next)
		if (nn->dev == dev && nn->ino == ino)
			return nn;
	return NULL;
}

static char *spath(const char *pathname)
{
	static char rpath[PATH_MAX + 1];
	
	char cpath[PATH_MAX];
	char *name;
	char *dp;
	
	while (*pathname == '/')
		pathname++;
	
	strcpy(cpath, pathname);
	*rpath = 0;
	dp = rpath;
	
	name = strtok(cpath, "/");
	
	while (name)
	{
		if (!strcmp(name, "."))
			goto next;
		
		if (!strcmp(name, ".."))
		{
			while (dp > rpath)
				if (*--dp == '/')
					break;
			goto next;
		}
		
		strcat(rpath, "/");
		strcat(rpath, name);
		
next:
		name = strtok(NULL, "/");
	}
	
	if (!rpath[1])
		return ".";
	
	return rpath + 1;
}

static int mused(const char *pathname)
{
	int i;
	
	if (!*members)
		return 1;
	
	for (i = 0; members[i]; i++)
		if (!strcmp(members[i], pathname))
			return 1;
	return 0;
}

static void mkmstr(char *buf, mode_t mode, int lt)
{
	strcpy(buf, "?---------");
	
	switch (lt)
	{
	case '7':
	case '0':
	case 0:
		buf[0] = '-';
		break;
	case '1':
		buf[0] = 'h';
		break;
	case '2':
		buf[0] = 'l';
		break;
	case '3':
		buf[0] = 'c';
		break;
	case '4':
		buf[0] = 'b';
		break;
	case '5':
		buf[0] = 'd';
		break;
	case '6':
		buf[0] = 'p';
		break;
	default:
		;
	}
	
	if (mode & S_IRUSR)
		buf[1] = 'r';
	
	if (mode & S_IWUSR)
		buf[2] = 'w';
	
	if (mode & S_IXUSR)
		buf[3] = 'x';
	
	if (mode & S_ISUID)
		buf[3] = 's';
	
	if (mode & S_IRGRP)
		buf[4] = 'r';
	
	if (mode & S_IWGRP)
		buf[5] = 'w';
	
	if (mode & S_IXGRP)
		buf[6] = 'x';
	
	if (mode & S_ISGID)
		buf[6] = 's';
	
	if (mode & S_IROTH)
		buf[7] = 'r';
	
	if (mode & S_IWOTH)
		buf[8] = 'w';
	
	if (mode & S_IXOTH)
		buf[9] = 'x';
}

static void list1(const struct tar_file *tf, int vl)
{
	struct passwd *owner;
	struct group *group;
	char obuf[16], gbuf[16];
	char mtstr[32];
	char mstr[10];
	char *ostr, *gstr;
	struct tm *tm;
	
	if (vl < 0)
		return;
	
	if (vl > 0)
	{
		owner = getpwuid(tf->owner);
		if (!owner)
		{
			sprintf(obuf, "%li", (long)tf->owner);
			ostr = obuf;
		}
		else
			ostr = owner->pw_name;
		
		group = getgrgid(tf->group);
		if (!group)
		{
			sprintf(gbuf, "%li", (long)tf->owner);
			gstr = gbuf;
		}
		else
			gstr = group->gr_name;
		
		mkmstr(mstr, tf->mode, tf->link_type);
		
		tm = localtime(&tf->mtime);
		strftime(mtstr, sizeof mtstr, "%Y-%m-%d %H:%M", tm);
		
		printf("%s %s/%s %8ji %s ", mstr, ostr, gstr, (intmax_t)tf->size, mtstr);
		if (tf->link_type == '1')
			printf("%s == %s\n", tf->pathname, tf->link);
		else
			puts(tf->pathname);
		return;
	}
	
	printf("%s\n", tf->pathname);
}

static void sreg(int fd, struct tar_hdr *hd, const char *pathname, const struct stat *st)
{
	char buf[32768];
	ssize_t rwcnt, wcnt;
	ssize_t rcnt;
	int sfd;
	
	sfd = open(pathname, O_RDONLY);
	if (sfd < 0)
	{
		warn("%s: open", pathname);
		xit = 1;
		return;
	}
	
	while (rcnt = read(sfd, buf, sizeof buf), rcnt > 0)
	{
		rwcnt = (rcnt + 511) & ~511;
		memset(buf + rcnt, 0, rwcnt - rcnt);
		
		wcnt = write(fd, buf, rwcnt);
		if (wcnt < 0)
			err(1, "%s: write", tar_pathname);
		if (wcnt != rwcnt)
			errx(1, "%s: write: Short write", tar_pathname);
	}
	if (rcnt < 0)
	{
		warn("%s: read", pathname);
		xit = 1;
	}
	
	close(sfd);
}

static void sdir(int fd, struct tar_hdr *hd, const char *pathname)
{
	struct dirent *de;
	char *pathname2;
	DIR *d;
	
	d = opendir(pathname);
	
	errno = 0;
	while (de = readdir(d), de)
	{
		if (!strcmp(de->d_name, "."))
			continue;
		
		if (!strcmp(de->d_name, ".."))
			continue;
		
		if (asprintf(&pathname2, "%s/%s", pathname, de->d_name) < 0)
			err(1, "asprintf");
		
		store1(fd, pathname2);
		
		free(pathname2);
	}
	if (errno)
	{
		warn("%s: readdir", pathname);
		xit = 1;
	}
	
	closedir(d);
}

static void sspec(int fd, struct tar_hdr *hd, const char *pathname, const struct stat *st)
{
}

static void init_head(struct tar_hdr *hd, const struct stat *st, const char *pathname)
{
	if (strlen(pathname) >= sizeof hd->pathname)
	{
		errno = ENAMETOOLONG;
		warn("%s", pathname);
		xit = 1;
		return;
	}
	
	strcpy(hd->pathname, pathname);
	
	sprintf(hd->mode,  "%07o",  (int)st->st_mode);
	sprintf(hd->owner, "%07o",  (int)st->st_uid);
	sprintf(hd->group, "%07o",  (int)st->st_gid);
	sprintf(hd->size,  "%011o", (int)st->st_size);
	sprintf(hd->mtime, "%011o", (int)st->st_mtime);
	
	switch (st->st_mode & S_IFMT)
	{
	case S_IFREG:
		hd->link_type = TAR_LT_REG;
		break;
	case S_IFDIR:
		hd->link_type = TAR_LT_DIR;
		break;
	default:
		hd->link_type = '0';
	}
}

static void cksum(struct tar_hdr *hd)
{
	unsigned cksum;
	uint8_t *sp;
	int i;
	
	memset(hd->cksum, ' ', sizeof hd->cksum);
	
	sp = (uint8_t *)hd;
	for (cksum = 0, i = 0; i < sizeof *hd; i++)
		cksum += sp[i];
	
	sprintf(hd->cksum, "%06o ", cksum);
}

static void store1(int fd, const char *pathname)
{
	union
	{
		struct tar_hdr hd;
		char buf[512];
	} u;
	struct namenode *nn;
	struct stat st;
	ssize_t wcnt;
	int link = 0;
	
	if (stat(pathname, &st))
	{
		warn("%s: stat", pathname);
		xit = 1;
		return;
	}
	
	if (!S_ISREG(st.st_mode))
		st.st_size = 0;
	
	memset(&u, 0, sizeof u);
	init_head(&u.hd, &st, pathname);
	
	if (st.st_nlink > 1)
	{
		nn = find_namenode(st.st_dev, st.st_ino);
		if (nn)
		{
			strcpy(u.hd.size, "00000000000");
			strcpy(u.hd.link, nn->pathname);
			u.hd.link_type = TAR_LT_HARD;
			link = 1;
		}
		else
			add_namenode(pathname, st.st_dev, st.st_ino);
	}
	
	cksum(&u.hd);
	
	wcnt = write(fd, &u, sizeof u);
	if (wcnt < 0)
		err(1, "%s: write", tar_pathname);
	if (wcnt != sizeof u)
		errx(1, "%s: write: Short write", tar_pathname);
	
	if (!link)
		switch (st.st_mode & S_IFMT)
		{
		case S_IFREG:
			sreg(fd, &u.hd, pathname, &st);
			break;
		case S_IFDIR:
			sdir(fd, &u.hd, pathname);
			break;
		default:
			sspec(fd, &u.hd, pathname, &st);
			break;
		}
}

static void store(int fd)
{
	char endblk[1024];
	ssize_t wcnt;
	int i;
	
	if (!*members)
		warnx("no members");
	
	for (i = 0; members[i]; i++)
		store1(fd, members[i]);
	
	memset(endblk, 0, sizeof endblk);
	
	wcnt = write(fd, endblk, sizeof endblk);
	if (wcnt < 0)
		err(1, "%s: write", tar_pathname);
	if (wcnt != sizeof endblk)
		errx(1, "%s: write: Short write", tar_pathname);
}

static void create(void)
{
	int fd;
	
	fd = open(tar_pathname, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd < 0)
		err(1, "%s: open", tar_pathname);
	
	store(fd);
	
	if (close(fd))
		err(1, "%s: close", tar_pathname);
}

static void append(void)
{
	struct tar *tar;
	
	tar = tar_openw(tar_pathname);
	if (!tar)
		err(1, "%s: open", tar_pathname);
	
	if (lseek(tar->fd, tar->append_offset, SEEK_SET) < 0)
		err(1, "%s: lseek", tar_pathname);
	
	store(tar->fd);
	
	tar_close(tar);
}

static void list(void)
{
	struct tar *tar;
	int i;
	
	for (i = 0; members[i]; i++)
		warnx("* %s", members[i]);
	
	tar = tar_open(tar_pathname);
	if (!tar)
		err(1, "%s", tar_pathname);
	
	for (i = 0; i < tar->file_cnt; i++)
		if (mused(tar->files[i].pathname))
			list1(&tar->files[i], vflag);
	
	tar_close(tar);
}

static void preserve(const struct tar_file *tf, int fd, const char *pathname)
{
	mode_t mode = tf->mode;
	int fdo = 0;
	
	if (!pflag)
		return;
	
	if (fd < 0)
	{
		fd = open(pathname, O_NOIO);
		if (fd < 0)
		{
			warn("%s: open", pathname);
			return;
		}
		fdo = 1;
	}
	
	if (fchown(fd, tf->owner, tf->group))
	{
		warn("%s: chown", pathname);
		xit = 1;
		goto clean;
	}
	
	if (fchmod(fd, mode))
	{
		warn("%s: chmod", pathname);
		xit = 1;
	}
	
clean:
	if (fdo)
		close(fd);
}

static void xdir(const struct tar *tar, const struct tar_file *tf, const char *pathname)
{
	if (mkdir(pathname, tf->mode) && errno != EEXIST)
	{
		warn("%s: mkdir", pathname);
		xit = 1;
		return;
	}
	
	preserve(tf, -1, pathname);
}

static void xreg(const struct tar *tar, const struct tar_file *tf, const char *pathname)
{
	static char buf[32768];
	
	ssize_t isz, osz;
	off_t size;
	off_t off;
	int fd;
	
	size = tf->size;
	off = 0;
	
	fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, tf->mode & 04777);
	if (fd < 0)
	{
		warn("%s: open", pathname);
		xit = 1;
		return;
	}
	
	while (off < size)
	{
		if (size - off > sizeof buf)
			isz = sizeof buf;
		else
			isz = size - off;
		
		isz = tar_read(tar, tf, buf, isz, off);
		if (isz < 0)
			err(1, "%s", tar_pathname);
		
		osz = write(fd, buf, isz);
		if (osz < 0)
		{
			warn("%s: write", pathname);
			break;
		}
		
		off += osz;
	}
	
	preserve(tf, fd, pathname);
	if (close(fd))
	{
		warn("%s: close", pathname);
		xit = 1;
	}
}

static void xlink(const struct tar *tar, const struct tar_file *tf, const char *pathname)
{
	char npathname[PATH_MAX];
	char *opathname;
	
	strcpy(npathname, pathname);
	opathname = spath(tf->link);
	
	switch (tf->link_type)
	{
	case TAR_LT_HARD:
		if (link(opathname, npathname))
		{
			warn("%s -> %s", npathname, opathname);
			xit = 1;
		}
		break;
	default:
		warnx("%s: Unknown link type", npathname);
		xit = 1;
	}
}

static void xspec(const struct tar *tar, const struct tar_file *tf, const char *pathname)
{
	warnx("%s: Special file", pathname);
	xit = 1;
}

static void xfifo(const struct tar *tar, const struct tar_file *tf, const char *pathname)
{
	warnx("%s: FIFO file", pathname);
	xit = 1;
}

static void extract1(struct tar *tar, const struct tar_file *tf)
{
	char *spn;
	
	spn = spath(tf->pathname);
	list1(tf, vflag - 1);
	
	switch (tf->link_type)
	{
	case TAR_LT_REG:
	case 0:
		xreg(tar, tf, spn);
		break;
	case TAR_LT_CHR:
	case TAR_LT_BLK:
		xspec(tar, tf, spn);
		break;
	case TAR_LT_FIFO:
		xfifo(tar, tf, spn);
		break;
	case TAR_LT_DIR:
		xdir(tar, tf, spn);
		break;
	case TAR_LT_HARD:
		xlink(tar, tf, spn);
		break;
	default:
		;
	}
}

static void extract(void)
{
	struct tar *tar;
	int i;
	
	tar = tar_open(tar_pathname);
	if (!tar)
		err(1, "%s", tar_pathname);
	
	for (i = 0; i < tar->file_cnt; i++)
		if (mused(tar->files[i].pathname))
			extract1(tar, &tar->files[i]);
	
	tar_close(tar);
}

static void usage(void)
{
	fputs("usage: tar {crtux}[p][v...][f ARCHIVE] [FILE...]\n", stderr);
	exit(1);
}

static int parse_opts(int argc, const char **argv)
{
	const char *p;
	
	if (argc < 1)
		usage();
	
	for (p = argv[0]; *p; p++)
		switch (*p)
		{
		case 'c':
		case 'r':
		case 't':
			mode = *p;
			break;
		case 'u':
			mode = 'r';
			break;
		case 'x':
			mode = 'x';
			break;
		case 'f':
			fflag = 1;
			break;
		case 'p':
			pflag = 1;
			break;
		case 'v':
			vflag++;
			break;
		default:
			usage();
		}
	
	if (fflag)
	{
		if (argc < 2)
			usage();
		
		tar_pathname = argv[1];
		return 2;
	}
	
	return 1;
}

int main(int argc, const char **argv)
{
	members = argv + parse_opts(argc - 1, argv + 1) + 1;
	
	switch (mode)
	{
	case 'c':
		create();
		break;
	case 'r':
		append();
		break;
	case 't':
		list();
		break;
	case 'x':
		extract();
		break;
	default:
		abort();
	}
	return xit;
}
