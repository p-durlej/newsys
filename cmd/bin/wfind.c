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
#include <wingui_form.h>
#include <wingui_dlg.h>
#include <newtask.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <regexp.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <paths.h>
#include <time.h>
#include <err.h>

#define DEBUG	0

static struct gadget *	root_input;
static struct gadget *	root_btn;
static struct gadget *	name_input;
static struct gadget *	name_sp_chk;
static struct gadget *	content_input;
static struct gadget *	content_rx_chk;
static struct gadget *	find_btn;
static struct gadget *	cancel_btn;
static struct form *	main_form;

static struct gadget *	prg_count_label;
static struct gadget *	prg_cwd_label;
static struct form *	prg_form;

static char *		prg_count_fmt;
static char *		prg_cwd_fmt;

static const char *	fname;
static int		fname_spat;

static const char *	content;
static regexp *		content_rx;

static char		cwd[PATH_MAX];
static int		count;
static int		stop;

static FILE *		tmpfile;

static void search(void);

static void browse_click(struct gadget *btn, int x, int y)
{
	char buf[PATH_MAX];
	
	strncpy(buf, root_input->text, sizeof buf);
	buf[sizeof buf - 1] = 0;
	
	if (dlg_file6(main_form, "Search in", "Select", buf, sizeof buf, DLG_FILE_WANT_DIR))
		input_set_text(root_input, buf);
}

static int mkxtemp(char *buf, size_t sz, mode_t mode)
{
	static int b;
	
	char pathname[PATH_MAX];
	char *tmpdir, *tmpsub = "";
	pid_t pid;
	time_t t;
	int i;
	int d;
	
	tmpdir = getenv("TMP");
	if (tmpdir == NULL)
	{
		tmpdir = getenv("HOME");
		tmpsub = "/.tmp";
		if (tmpdir == NULL)
		{
			tmpdir = "/tmp";
			tmpsub = "";
		}
	}
	
	pid = getpid();
	time(&t);
	
	for (i = 0; i < 1024; i++)
	{
		sprintf(pathname, "%s%s/search-%u-%lu-%u",
			tmpdir, tmpsub, (unsigned)pid, (unsigned long)t, b + i);
		d = open(pathname, O_CREAT | O_EXCL | O_RDWR, mode);
		if (d < 0)
		{
			if (errno == EEXIST)
				continue;
			return -1;
		}
		break;
	}
	
	if (buf != NULL)
	{
		if (strlen(pathname) >= sz)
		{
			unlink(pathname);
			errno = ENAMETOOLONG;
			return -1;
		}
		strcpy(buf, pathname);
	}
	
	b = i;
	return d;
}

static int fnmatch1(const char *pat, const char *name)
{
	const char *pp = pat, *np = name;
	
	while (*pp)
		switch (*pp)
		{
		case '*':
			while (*pp == '*' || *pp == '?')
				pp++;
			
			if (!*pp)
				return 1;
			
			for (; *np; np++)
				if (fnmatch1(pp, np))
					return 1;
			return 0;
		case '?':
			if (!*np)
				return 0;
			np++;
			pp++;
			break;
		default:
			if (*np != *pp)
				return 0;
			np++;
			pp++;
		}
	
	if (*np)
		return 0;
	return 1;
}

static int nmatch(const char *name)
{
	if (fname_spat)
		return fnmatch1(fname, name);
	return !strcmp(fname, name);
}

static int pcat(char *buf, const char *path, const char *name)
{
	size_t plen = strlen(path);
	size_t nlen = strlen(path);
	
	if (plen + nlen + 1 >= PATH_MAX)
	{
		errno = ENAMETOOLONG;
		return -1;
	}
	
	strcpy(buf, path);
	if (plen && path[plen - 1] != '/')
		strcat(buf, "/");
	strcat(buf, name);
	return 0;
}

static void updateprg(void)
{
	char *p;
	
	if (asprintf(&p, prg_count_fmt, count) < 0)
	{
		msgbox_perror(prg_form, "Find files", "asprintf", errno);
		exit(1);
	}
	label_set_text(prg_count_label, p);
	free(p);
	
	if (asprintf(&p, prg_cwd_fmt, cwd) < 0)
	{
		msgbox_perror(prg_form, "Find files", "asprintf", errno);
		exit(1);
	}
	label_set_text(prg_cwd_label, p);
	free(p);
}

static void found(const char *name)
{
	static char tmp[PATH_MAX];
	
#if DEBUG
	warnx("found \"%s/%s\"", cwd, name);
#endif
	
	if (pcat(tmp, cwd, name))
		return;
	fprintf(tmpfile, "%s\n", tmp);
	
	count++;
	updateprg();
	win_idle();
}

static void sfile(const char *name, const struct stat *st)
{
	static char tmp[PATH_MAX];
	
	char buf[4096];
	char *p;
	FILE *f;
	int m = 0;
	
	if (!nmatch(name))
		return;
	
	if (*content)
	{
		if (pcat(tmp, cwd, name))
			return;
		
		f = fopen(tmp, "r");
		if (f == NULL)
		{
#if DEBUG
			warnx("%s: fopen", tmp);
#endif
			return;
		}
		while (fgets(buf, sizeof buf, f))
		{
			p = strchr(buf, '\n');
			if (p != NULL)
				*p = 0;
			
			if (content_rx != NULL)
				m = regexec(content_rx, buf);
			else
				m = !!strstr(buf, content);
			
			if (m)
			{
#if DEBUG
				warnx("buf = \"%s\", content = \"%s\"", buf, content);
#endif
				break;
			}
		}
		fclose(f);
		
		if (!m)
			return;
	}
	
	found(name);
}

static void sdir(const char *name, const struct stat *st)
{
	size_t len = strlen(cwd);
	
#if DEBUG
	warnx("descending into \"%s/%s\"", cwd, name);
#endif
	
	if (pcat(cwd, cwd, name))
		return;
	search();
	cwd[len] = 0;
	
	if (!nmatch(name))
		return;
	if (*content)
		return;
	found(name);
}

static void sspec(const char *name, const struct stat *st)
{
	if (!nmatch(name))
		return;
	if (*content)
		return;
	found(name);
}

static void snode(const char *name, const struct stat *st)
{
#if DEBUG
	warnx("examining \"%s/%s\"", cwd, name);
#endif
	
	switch (st->st_mode & S_IFMT)
	{
	case S_IFREG:
		sfile(name, st);
		break;
	case S_IFDIR:
		sdir(name, st);
		break;
	default:
		sspec(name, st);
	}
}

static void search(void)
{
	static char tmp[PATH_MAX];
	
	struct dirent *de;
	struct stat st;
	DIR *d;
	
#if DEBUG
	warnx("searching in \"%s\"", cwd);
#endif
	updateprg();
	win_idle();
	
	d = opendir(cwd);
	if (d == NULL)
	{
#if DEBUG
		warn("%s", cwd);
#endif
		return;
	}
	while (de = readdir(d), de != NULL)
	{
		if (stop)
			break;
		
		if (!strcmp(de->d_name, "."))
			continue;
		if (!strcmp(de->d_name, ".."))
			continue;
		
		if (pcat(tmp, cwd, de->d_name))
			continue;
		
		if (stat(tmp, &st))
			continue;
		
		snode(de->d_name, &st);
	}
	closedir(d);
}

static int prg_close(struct form *f)
{
	stop = 1;
	return 0;
}

static void find_click(struct gadget *btn, int x, int y)
{
	char tmp[PATH_MAX];
	int d;
	
	fname_spat = chkbox_get_state(name_sp_chk);
	fname	   = name_input->text;
	
	content = content_input->text;
	if (chkbox_get_state(content_rx_chk))
	{
		content_rx = regcomp(content);
		if (content_rx == NULL)
		{
			msgbox(main_form, "Find files",
				"Malformed regular expression.");
			return;
		}
	}
	
	d = mkxtemp(tmp, sizeof tmp, 0600);
	if (d < 0)
	{
		msgbox_perror(main_form, "Find files", "Cannot create temporary file", errno);
		return;
	}
	unlink(tmp);
	sprintf(tmp, "%i", d);
	
	tmpfile = fdopen(d, "w");
	if (tmpfile == NULL)
	{
		msgbox_perror(main_form, "Find files", "Cannot fdopen temporary file", errno);
		close(d);
		return;
	}
	
	if (strlen(root_input->text) >= sizeof cwd)
	{
		msgbox_perror(main_form, "Find files", "", ENAMETOOLONG);
		fclose(tmpfile);
		return;
	}
	strcpy(cwd, root_input->text);
	calloc(1, 32);
	
	count = 0;
	stop = 0;
	
	prg_form = form_load("/lib/forms/wfindp.frm");
	
	form_on_close(prg_form, prg_close);
	
	prg_count_label	= gadget_find(prg_form, "count_label");
	prg_cwd_label	= gadget_find(prg_form, "cwd_label");
	
	prg_count_fmt = strdup(prg_count_label->text);
	prg_cwd_fmt   = strdup(prg_cwd_label->text);
	
	if (prg_count_fmt == NULL || prg_cwd_fmt == NULL)
	{
		msgbox(main_form, "Find files", "Out of memory");
		exit(1);
	}
	
	form_show(prg_form);
	search();
	form_on_close(prg_form, NULL);
	form_close(prg_form);
	// fclose(tmpfile);
	fflush(tmpfile);
	
	if (lseek(d, 0L, SEEK_CUR) < 0)
		err(1, "%i: lseek", d);
	
	if (!count)
	{
		msgbox(main_form, "Find files", "No matching files");
		goto clean;
	}
	
	if (_newtaskl(_PATH_B_FILEMGR, _PATH_B_FILEMGR, "-l", tmp, (void *)NULL) < 0)
		msgbox_perror(main_form, "Find files", "Cannot run " _PATH_B_FILEMGR, errno);
	
	goto clean;
clean:
	free(content_rx);
	
	content_rx = NULL;
	content = NULL;
	fname = NULL;
}

static void cancel_click(struct gadget *btn, int x, int y)
{
	exit(0);
}

int main(int argc, char **argv)
{
	signal(SIGCHLD, SIG_IGN);
	
	if (win_attach())
		err(255, NULL);
	
	main_form = form_load("/lib/forms/wfind.frm");
	if (main_form == NULL)
	{
		msgbox_perror(NULL, "Find files", "Cannot load main form", errno);
		return 1;
	}
	root_input     = gadget_find(main_form, "root_input");
	root_btn       = gadget_find(main_form, "root_btn");
	name_input     = gadget_find(main_form, "name_input");
	name_sp_chk    = gadget_find(main_form, "name_sp_chk");
	content_input  = gadget_find(main_form, "content_input");
	content_rx_chk = gadget_find(main_form, "content_rx_chk");
	root_btn       = gadget_find(main_form, "root_btn");
	find_btn       = gadget_find(main_form, "find_btn");
	cancel_btn     = gadget_find(main_form, "cancel_btn");
	
	button_on_click(root_btn,   browse_click);
	button_on_click(find_btn,   find_click);
	button_on_click(cancel_btn, cancel_click);
	
	input_set_text(root_input, "/");
	input_set_text(name_input, "*");
	chkbox_set_state(name_sp_chk, 1);
	chkbox_set_state(content_rx_chk, 1);
	
	form_wait(main_form);
	return 0;
}
