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
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <os386.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <err.h>

#define DE_GROW_BY	64

static struct passwd *owner;
static struct group *group;

static int flag1;
static int lflag;
static int aflag;
static int dflag;
static int iflag;
static int qflag;
static int multi;
static int bigC;
static int ttyw;

static int xit;

static struct
{
	char fmt0[64];
	char fmt1[64];
	char fmtsz[64];
	char fmtdv[64];
	
	int ino_w;
	int nlink_w;
	int owner_w;
	int group_w;
	int size_w;
	int name_w;
} cols;

static struct ls_de
{
	struct stat st;
	char *group;
	char *owner;
	char *name;
	char *path;
} *dirents;
static int de_cnt = 0;
static int de_cap = 0;

static void procopt(int argc, char **argv)
{
	int opt;
	
	while (opt = getopt(argc, argv, "ladiq1C"), opt > 0)
		switch (opt)
		{
		case 'l':
			lflag = 1;
			break;
		case 'a':
			aflag = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'i':
			iflag = 1;
			break;
		case 'q':
			qflag = 1;
			break;
		case '1':
			flag1 = 1;
			break;
		case 'C':
			bigC = 1;
			break;
		default:
			exit(255);
		}
}

static void print_name(const char *name)
{
	const char *p;
	
	if (!qflag)
	{
		fputs(name, stdout);
		return;
	}
	
	for (p = name; *p; p++)
	{
		if (isprint(*p))
			putchar(*p);
		else
			putchar('?');
	}
}

static void pdirent(const struct ls_de *de)
{
	int i;
	
	if (iflag)
		printf(cols.fmt0, (unsigned long)de->st.st_ino);
	
	if (lflag)
	{
		char *tm = ctime(&de->st.st_mtime);
		char perm[11];
		char *nl;
		
		nl = strchr(tm, '\n');
		if (nl)
			*nl = 0;
		
		strcpy(perm, "----------");
		
		switch (S_IFMT & de->st.st_mode)
		{
		case S_IFREG:
			perm[0] = '-';
			break;
		case S_IFDIR:
			perm[0] = 'd';
			break;
		case S_IFCHR:
			perm[0] = 'c';
			break;
		case S_IFBLK:
			perm[0] = 'b';
			break;
		case S_IFIFO:
			perm[0] = 'p';
			break;
		case S_IFSOCK:
			perm[0] = 's';
			break;
		default:
			perm[0] = '?';
		}
		
		if (de->st.st_mode & S_IRUSR)
			perm[1] = 'r';
		if (de->st.st_mode & S_IWUSR)
			perm[2] = 'w';
		if (de->st.st_mode & S_IXUSR)
			perm[3] = 'x';
		
		if (de->st.st_mode & S_IRGRP)
			perm[4] = 'r';
		if (de->st.st_mode & S_IWGRP)
			perm[5] = 'w';
		if (de->st.st_mode & S_IXGRP)
			perm[6] = 'x';
		
		if (de->st.st_mode & S_IROTH)
			perm[7] = 'r';
		if (de->st.st_mode & S_IWOTH)
			perm[8] = 'w';
		if (de->st.st_mode & S_IXOTH)
			perm[9] = 'x';
		
		if (de->st.st_mode & S_ISUID)
			perm[3] = 's';
		if (de->st.st_mode & S_ISGID)
			perm[6] = 's';
		printf("%s ", perm);
		
		printf(cols.fmt1, (int)de->st.st_nlink, de->owner, de->group);
		
		switch (S_IFMT & de->st.st_mode)
		{
		case S_IFSOCK:
		case S_IFIFO:
			for (i = 0; i <= cols.size_w; i++)
				putchar(' ');
			break;
		case S_IFCHR:
		case S_IFBLK:
			printf(cols.fmtdv, (long)major(de->st.st_rdev), (long)minor(de->st.st_rdev));
			break;
		default:
			printf(cols.fmtsz, (unsigned long long)de->st.st_size);
		}
		
		fputs(tm, stdout);
		putchar(' ');
		print_name(de->name);
		putchar('\n');
	}
	else
	{
		print_name(de->name);
		if (bigC)
			for (i = strlen(de->name); i < cols.name_w; i++)
				putchar(' ');
		else
			putchar('\n');
	}
}

static void adirent(const char *pathname, const struct stat *st, const char *path)
{
	struct ls_de *de;
	
	if (de_cnt >= de_cap)
	{
		de_cap	+= DE_GROW_BY;
		dirents  = realloc(dirents, de_cap * sizeof *dirents);
		if (!dirents)
			err(errno, NULL);
	}
	de = &dirents[de_cnt++];
	memset(de, 0, sizeof *de);
	
	de->name = strdup(pathname);
	if (de->name == NULL)
		err(errno, NULL);
	
	de->path = strdup(path);
	if (de->path == NULL)
		err(errno, NULL);
	
	de->st = *st;
	if (lflag)
	{
		if (!owner || owner->pw_uid != st->st_uid)
			owner = getpwuid(st->st_uid);
		if (!owner)
		{
			if (asprintf(&de->owner, "%u", (unsigned)st->st_uid) < 0)
				err(errno, NULL);
		}
		else
		{
			de->owner = strdup(owner->pw_name);
			if (!de->owner)
				err(errno, NULL);
		}
		
		if (!group || group->gr_gid != st->st_gid)
			group = getgrgid(st->st_gid);
		if (!group)
		{
			if (asprintf(&de->group, "%u", (unsigned)st->st_gid) < 0)
				err(errno, NULL);
		}
		else
		{
			de->group = strdup(group->gr_name);
			if (!de->group)
				err(errno, NULL);
		}
	}
}

static void collect_dir(char *pathname)
{
	char filename[PATH_MAX];
	struct dirent *de;
	struct stat st;
	DIR *dir;
	
	dir = opendir(pathname);
	if (!dir)
		err(errno, "%s", pathname);
	
	for (;;)
	{
		errno = 0;
		de = readdir(dir);
		if (errno)
			err(errno, "%s", pathname);
		if (!de)
			break;
		
		if (*de->d_name == '.' && !aflag)
			continue;
		
		if (lflag || iflag)
		{
			if (strlen(pathname) + strlen(de->d_name) + 1 >= sizeof filename)
			{
				warnx("%s/%s: Pathname too long", pathname, de->d_name);
				xit = 1;
				continue;
			}
			snprintf(filename, sizeof filename, "%s/%s", pathname, de->d_name);
			if (stat(filename, &st))
			{
				warn("%s", de->d_name);
				xit = 1;
				continue;
			}
		}
		
		adirent(de->d_name, &st, pathname);
	}
	closedir(dir);
}

static int numlen(unsigned long long v)
{
	int len;
	
	if (!v)
		return 1;
	for (len = 0; v; v /= 10)
		len++;
	return len;
}

static void cwid(void)
{
	struct ls_de *p, *e;
	int size_maj = 0;
	int size_min = 0;
	int majl, minl;
	
	memset(&cols, 0, sizeof cols);
	cols.name_w = 8;
	
	e = dirents + de_cnt;
	for (p = dirents; p < e; p++)
	{
		int len;
		
		if (iflag)
		{
			len = numlen(p->st.st_ino);
			if (len > cols.ino_w)
				cols.ino_w = len;
		}
		
		if (lflag)
		{
			len = numlen(p->st.st_nlink);
			if (len > cols.nlink_w)
				cols.nlink_w = len;
			
			if (p->owner)
			{
				len = strlen(p->owner);
				if (len > cols.owner_w)
					cols.owner_w = len;
			}
			if (p->group)
			{
				len = strlen(p->group);
				if (len > cols.group_w)
					cols.group_w = len;
			}
			
			len = numlen(p->st.st_size);
			if (len > cols.size_w)
				cols.size_w = len;
			
			majl = numlen(major(p->st.st_rdev));
			minl = numlen(minor(p->st.st_rdev));
			if (majl > size_maj)
				size_maj = majl;
			if (minl > size_min)
				size_min = minl;
			
			len = majl + minl + 2;
			if (len > cols.size_w)
				cols.size_w = len;
		}
		
		len = strlen(p->name) + 1;
		if (len > cols.name_w)
			cols.name_w = len;
	}
	sprintf(cols.fmt0, "%%%ilu ", cols.ino_w);
	sprintf(cols.fmt1, "%%%ii %%-%is %%-%is ", cols.nlink_w, cols.owner_w, cols.group_w);
	sprintf(cols.fmtsz, "%%%illu ", cols.size_w);
	sprintf(cols.fmtdv, "%%%ilu, %%%ilu ", size_maj, size_min);
}

static int de_cmp(const void *de0, const void *de1)
{
#define de0	((struct ls_de *)de0)
#define de1	((struct ls_de *)de1)
	int r;
	
	r = strcmp(de0->path, de1->path);
	if (r)
		return r;
	
	return strcmp(de0->name, de1->name);
#undef de0
#undef de1
}

static void collect(char *pathname)
{
	struct stat st;
	
	if (stat(pathname, &st))
		err(errno, "%s", pathname);
	
	if (S_ISDIR(st.st_mode) && !dflag)
		collect_dir(pathname);
	else
		adirent(pathname, &st, "");
}

static void list(void)
{
	static const struct ls_de nulde = { .path = "", .name = "" };
	
	const struct ls_de *prev = &nulde;
	struct ls_de *p, *e;
	int colw;
	int ncol;
	int i;
	
	colw = cols.name_w + 1;
	if (cols.ino_w)
		colw += cols.ino_w + 1;
	ncol = ttyw / colw;
	
	e = dirents + de_cnt;
	for (i = ncol - 1, p = dirents; p < e; p++, i--)
	{
		if (strcmp(prev->path, p->path) && multi)
		{
			printf("\n%s:\n", p->path);
			i = ncol - 1;
		}
		else if (!strcmp(prev->name, p->name))
			continue;
		pdirent(p);
		prev = p;
		
		if (bigC && !i)
		{
			putchar('\n');
			i = ncol;
		}
		/* free(p->owner);
		free(p->group);
		free(p->name); */
	}
	if (bigC && i != ncol - 1)
		putchar('\n');
	
	de_cnt = de_cap = 0;
	/* free(dirents); */
	dirents = NULL;
}

static void usage(void)
{
	printf("\nUsage: ls [-1adilqC] [FILE...]\n"
		"Directory listing.\n\n"
		"  -1 single-column output\n"
		"  -a show all files\n"
		"  -d do not enter directories\n"
		"  -i show inode numbers\n"
		"  -l enable long list format\n"
		"  -q replace non-printable characters with '?'\n"
		"  -C multi-column output (cannot be used with -l)\n\n"
		);
	exit(0);
}

int main(int argc, char **argv)
{
	struct pty_size psz;
	const char *p;
	int i;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
		usage();
	procopt(argc, argv);
	
	p = strrchr(argv[0], '/');
	if (p != NULL)
		p++;
	else
		p = argv[0];
	
	if (!strcmp(p, "ll"))
		lflag = 1;
	
	if (isatty(1))
	{
		bigC  = 1;
		ttyw  = 80;
		qflag = 1;
		
		if (!ioctl(1, PTY_GSIZE, &psz))
			ttyw = psz.w;
	}
	if (lflag || flag1)
		bigC = 0;
	
	if (optind < argc - 1)
		multi = 1;
	
	for (i = optind; i < argc; i++)
		collect(argv[i]);
	
	if (optind >= argc)
		collect(".");
	
	qsort(dirents, de_cnt, sizeof *dirents, de_cmp);
	cwid();
	list();
	return xit;
}
