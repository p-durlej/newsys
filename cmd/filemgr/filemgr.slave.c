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

#include <wingui_msgbox.h>
#include <wingui_form.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <err.h>

static void fail(char *msg);
static void usage(void);

static void cpmv_file(char *src, char *dst, int move);
static void cpmv_dir(char *src, char *dst, int move);
static void cpmv(char *src, char *dst, int move);

static void del_dir(char *pathname);
static void del(char *pathname);

static void cancel_click();

static void cpmv_status(char *src, char *dst, int move);
static void del_status(char *path);

static struct form *cpmv_form;
static struct form *del_form;

static int detached;
static int detach;

static void fail(char *msg)
{
	if (!detached)
		msgbox(cpmv_form ? cpmv_form : del_form, "File Manager Slave", msg);
	
	exit(1);
}

static void usage(void)
{
	msgbox(NULL,
		"File Manager Slave",
		"usage:\n\n"
		"    filemgr.slave copy SOURCE DESTINATION\n"
		"    filemgr.slave move SOURCE DESTINATION\n"
		"    filemgr.slave del PATHNAME");
	exit(1);
}

static void check_paths(const char *src, const char *dst)
{
	static char csrc[PATH_MAX];
	static char cdst[PATH_MAX];
	static char cwd[PATH_MAX];
	static char msg[256 + PATH_MAX];
	size_t len;
	
	if (strlen(src) >= sizeof csrc || strlen(dst) >= sizeof cdst)
		fail("Pathname too long.");
	strcpy(csrc, src);
	strcpy(cdst, dst);
	
	if (getcwd(cwd, sizeof cwd) == NULL)
	{
		sprintf(msg, "Cannot get CWD:\n\n%m", src);
		fail(msg);
	}
	
	if (_mkcanon(cwd, csrc) || _mkcanon(cwd, cdst))
	{
		sprintf(msg, "Cannot canonicalize pathnames:\n\n%m", src);
		fail(msg);
	}
	
	if (!strcmp(csrc, cdst))
		fail("Destination same as source.");
	
	len = strlen(csrc);
	if (!strncmp(csrc, cdst, len) && cdst[len] == '/')
		fail("Destination is a subdir of source.");
}

static void cpmv_file(char *src, char *dst, int move)
{
	static char msg[256 + PATH_MAX];
	static char buf[262144];
	
	struct stat st;
	int i_fd;
	int o_fd;
	int cnt;
	
	i_fd = open(src, O_RDONLY);
	if (i_fd < 0)
	{
		sprintf(msg, "Cannot open \"%s\":\n\n%m", src);
		fail(msg);
	}
	fstat(i_fd, &st);
	
	o_fd = open(dst, O_WRONLY | O_CREAT | O_EXCL, 0600);
	if (o_fd < 0)
	{
		sprintf(msg, "Cannot create \"%s\":\n\n%m", dst);
		fail(msg);
	}
	
	while ((cnt = read(i_fd, buf, sizeof buf)))
	{
		if (cnt < 0)
		{
			sprintf(msg, "Cannot read \"%s\":\n\n%m", src);
			fail(msg);
		}
		
		if (write(o_fd, buf, cnt) != cnt)
		{
			sprintf(msg, "Cannot write \"%s\":\n\n%m", dst);
			fail(msg);
		}
		
		if (detach)
		{
			win_detach();
			detached = 1;
			detach = 0;
		}
		
		if (!detached)
			win_idle();
	}
	fchmod(o_fd, st.st_mode);
	
	close(i_fd);
	close(o_fd);
	
	if (move && unlink(src))
	{
		sprintf(msg, "Cannot unlink \"%s\":\n\n%m", src);
		fail(msg);
	}
}

static void cpmv_dir(char *src, char *dst, int move)
{
	char msg[256 + PATH_MAX];
	char path1[PATH_MAX];
	char path2[PATH_MAX];
	struct dirent *de;
	struct stat st;
	DIR *d;
	
	if (stat(src, &st))
	{
		sprintf(msg, "Cannot stat \"%s\":\n\n%m", src);
		fail(msg);
	}
	d = opendir(src);
	if (!d)
	{
		sprintf(msg, "Cannot open \"%s\":\n\n%m", src);
		fail(msg);
	}
	if (mkdir(dst, st.st_mode))
	{
		sprintf(msg, "Cannot mkdir \"%s\":\n\n%m", dst);
		fail(msg);
	}
	while ((de = readdir(d)))
	{
		if (!strcmp(de->d_name, "."))
			continue;
		if (!strcmp(de->d_name, ".."))
			continue;
		
		strcpy(path1, src);
		strcat(path1, "/");
		strcat(path1, de->d_name);
		
		strcpy(path2, dst);
		strcat(path2, "/");
		strcat(path2, de->d_name);
		
		cpmv(path1, path2, move);
	}
	closedir(d);
	
	if (move && rmdir(src))
	{
		sprintf(msg, "Cannot rmdir \"%s\":\n\n%m", src);
		fail(msg);
	}
}

static void cpmv(char *src, char *dst, int move)
{
	char msg[256 + PATH_MAX];
	struct stat st;
	
	if (!access(dst, 0))
	{
		errno = EEXIST;
		sprintf(msg, "Cannot create \"%s\":\n\n%m", dst);
		fail(msg);
	}
	
	cpmv_status(src, dst, move);
	
	if (move && !rename(src, dst))
		return;
	
	if (stat(src, &st))
	{
		sprintf(msg, "Cannot stat \"%s\":\n\n%m", src);
		fail(msg);
	}
	
	switch (st.st_mode & S_IFMT)
	{
	case S_IFREG:
		cpmv_file(src, dst, move);
		break;
	case S_IFDIR:
		cpmv_dir(src, dst, move);
		break;
	default:
		if (mknod(dst, st.st_mode, st.st_rdev))
		{
			sprintf(msg, "Cannot mknod \"%s\":\n\n%m");
			fail(msg);
		}
	}
}

static void del_dir(char *pathname)
{
	char msg[256 + PATH_MAX];
	char sub[PATH_MAX];
	struct dirent *de;
	DIR *d;
	
	d = opendir(pathname);
	if (!d)
	{
		sprintf(msg, "Cannot open \"%s\":\n\n%m", pathname);
		fail(msg);
	}
	while ((de = readdir(d)))
	{
		if (!strcmp(de->d_name, "."))
			continue;
		if (!strcmp(de->d_name, ".."))
			continue;
		
		strcpy(sub, pathname);
		strcat(sub, "/");
		strcat(sub, de->d_name);
		
		del(sub);
	}
	closedir(d);
	
	if (rmdir(pathname))
	{
		sprintf(msg, "Cannot rmdir \"%s\":\n\n%m", pathname);
		fail(msg);
	}
}

static void del(char *pathname)
{
	char msg[256 + PATH_MAX];
	struct stat st;
	
	del_status(pathname);
	
	if (stat(pathname, &st))
	{
		sprintf(msg, "Cannot stat \"%s\":\n\n%m", pathname);
		fail(msg);
	}
	
	if ((st.st_mode & S_IFMT) == S_IFDIR)
		del_dir(pathname);
	else
	{
		if (unlink(pathname))
		{
			sprintf(msg, "Cannot unlink \"%s\":\n\n%m", pathname);
			fail(msg);
		}
	}
}

static void cancel_click()
{
	exit(0);
}

static void detach_click()
{
	detach = 1;
}

static void cpmv_status(char *src, char *dst, int move)
{
	static struct gadget *l1;
	static struct gadget *l2;
	static struct gadget *l3;
	static struct gadget *l4;
	static char l_src[PATH_MAX];
	static char l_dst[PATH_MAX];
	
	if (detach)
	{
		win_detach();
		detached = 1;
		detach = 0;
	}
	
	if (detached)
		return;
	
	strcpy(l_src, src);
	strcpy(l_dst, dst);
	
	if (!cpmv_form)
	{
		cpmv_form = form_creat(FORM_TITLE | FORM_FRAME, 1, -1, -1, 320, 68, move ? "Moving" : "Copying");
		l1 = label_creat(cpmv_form, 2,  2, move ? "Moving:" : "Copying:");
		l2 = label_creat(cpmv_form, 2, 18, "");
		l3 = label_creat(cpmv_form, 2, 34, "To:");
		l4 = label_creat(cpmv_form, 2, 50, "");
		button_creat(cpmv_form, 196, 46, 60, 20, "Detach", detach_click);
		button_creat(cpmv_form, 258, 46, 60, 20, "Cancel", cancel_click);
	}
	
	label_set_text(l2, l_src);
	label_set_text(l4, l_dst);
	win_idle();
}

static void del_status(char *path)
{
	static struct gadget *l1;
	static struct gadget *l2;
	static char l_path[PATH_MAX];
	
	if (detach)
	{
		win_detach();
		detached = 1;
		detach = 0;
	}
	
	if (detached)
		return;
	
	strcpy(l_path, path);
	
	if (!del_form)
	{
		del_form = form_creat(FORM_TITLE | FORM_FRAME, 1, -1, -1, 320, 68, "Removing");
		l1 = label_creat(del_form, 2,  2, "Removing");
		l2 = label_creat(del_form, 2, 18, "");
		button_creat(del_form, 196, 46, 60, 20, "Detach", detach_click);
		button_creat(del_form, 258, 46, 60, 20, "Cancel", cancel_click);
	}
	
	label_set_text(l2, l_path);
	win_idle();
}

static void x_stat(const char *pathname, struct stat *st)
{
	char msg[PATH_MAX + 256];
	
	if (stat(pathname, st))
	{
		sprintf(msg, "Cannot stat %s", pathname);
		msgbox_perror(NULL, "File Manager", msg, errno);
		exit(255);
	}
}

int main(int argc, char **argv)
{
	if (win_attach())
		err(255, NULL);
	
	if (argc < 2)
		usage();
	
	if (!strcmp(argv[1], "copy") && argc == 4)
	{
		check_paths(argv[2], argv[3]);
		cpmv(argv[2], argv[3], 0);
		return 0;
	}
	
	if (!strcmp(argv[1], "move") && argc == 4)
	{
		check_paths(argv[2], argv[3]);
		cpmv(argv[2], argv[3], 1);
		return 0;
	}
	
	if (!strcmp(argv[1], "drag") && argc == 4)
	{
		struct stat sst, dst;
		int mv = 1;
		
		x_stat(argv[2], &sst);
		
		if (sst.st_uid != getuid())
			mv = 0;
		
		check_paths(argv[2], argv[3]);
		cpmv(argv[2], argv[3], mv);
		return 0;
	}
	
	if (!strcmp(argv[1], "del") && argc == 3)
	{
		del(argv[2]);
		return 0;
	}
	
	usage();
	return 1;
}
