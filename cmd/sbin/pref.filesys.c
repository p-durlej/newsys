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
#include <wingui_color.h>
#include <wingui_form.h>
#include <wingui_dlg.h>
#include <wingui.h>
#include <sys/wait.h>
#include <newtask.h>
#include <bioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <mount.h>
#include <errno.h>
#include <err.h>

#define MAIN_FORM	"/lib/forms/pref.filesys.frm"
#define MOUNT_FORM	"/lib/forms/mount.frm"
#define FSTAB_TMP	"/etc/fstab.tmp"
#define FSTAB		"/etc/fstab"
#define MKFS		"/sbin/mkfs"

#define REALPREFIX(p)	((p)[0] != '/' || (p)[1] ? (p) : "")

static struct _mtab mtab[MOUNT_MAX];

static struct form *main_form;
static struct gadget *fs_list;

static int device_pos, fstype_pos, flags_pos;

static const char *flagstr(int flags)
{
	static char buf[8];
	
	char *p;
	
	p = buf;
	if (flags & MF_READ_ONLY)
		*p++ = 'r';
	else
		*p++ = 'w';
	
	if (flags & MF_NO_ATIME)
		*p++ = 'n';
	
	if (flags & MF_REMOVABLE)
		*p++ = 'm';
	
	if (flags & MF_INSECURE)
		*p++ = 'i';
	
	*p = 0;
	return buf;
}

static void fmnt(FILE *f, struct _mtab *m)
{
	if (!m->mounted)
		return;
	
	fprintf(f, "%-10s %-16s %-6s %s\n", m->device, m->prefix, m->fstype, flagstr(m->flags));
}

static int save_fstab(void)
{
	FILE *f;
	int se;
	int i;
	
	f = fopen(FSTAB_TMP, "w");
	if (!f)
	{
		if (errno == EROFS && getenv("OSDEMO"))
			return 0;
		return -1;
	}
	for (i = 0; i < MOUNT_MAX; i++)
		fmnt(f, &mtab[i]);
	if (ferror(f))
	{
		se = errno;
		fclose(f);
		errno = se;
		return -1;
	}
	fclose(f);
	return rename(FSTAB_TMP, FSTAB);
}

static int do_mkfs(const char *device)
{
	struct bio_info bi;
	char *size_str;
	pid_t pid;
	int fd;
	int st;
	
	fd = open(device, O_RDWR);
	if (fd < 0)
		return -1;
	if (ioctl(fd, BIO_INFO, &bi))
		return -1;
	close(fd);
	if (asprintf(&size_str, "%llu", (long long)bi.size) < 0)
		return -1;
	
	pid = _newtaskl(MKFS, MKFS, device, size_str, "256", (void *)NULL);
	if (pid < 0)
		return -1;
	while (wait(&st) < 1)
		win_idle();
	
	if (WTERMSIG(st))
	{
		errno = EFAULT;
		return -1;
	}
	if (WEXITSTATUS(st))
	{
		errno = WEXITSTATUS(st);
		return -1;
	}
	return 0;
}

static int edit_fs(struct _mtab *m, int n)
{
	struct gadget *g_prefix;
	struct gadget *g_device;
	struct gadget *g_fstype;
	struct gadget *g_noatime;
	struct gadget *g_readonly;
	struct gadget *g_removable;
	struct gadget *g_insecure;
	struct gadget *g_mkfs;
	char pathname[PATH_MAX];
	struct form *f;
	const char *cp;
	char *p;
	int r;
	
	f = form_load(MOUNT_FORM);
	if (!f)
	{
		msgbox_perror(main_form, "File Systems", "Cannot load form", errno);
		return 0;
	}
	
	g_prefix    = gadget_find(f, "prefix");
	g_device    = gadget_find(f, "device");
	g_fstype    = gadget_find(f, "fstype");
	g_readonly  = gadget_find(f, "readonly");
	g_removable = gadget_find(f, "removable");
	g_insecure  = gadget_find(f, "insecure");
	g_noatime   = gadget_find(f, "noatime");
	g_mkfs	    = gadget_find(f, "mkfs");
	input_set_text(g_prefix, m->prefix);
	input_set_text(g_device, m->device);
	input_set_text(g_fstype, m->fstype);
	if (m->flags & MF_READ_ONLY)
		chkbox_set_state(g_readonly, 1);
	if (m->flags & MF_NO_ATIME)
		chkbox_set_state(g_noatime, 1);
	if (m->flags & MF_REMOVABLE)
		chkbox_set_state(g_removable, 1);
	if (m->flags & MF_INSECURE)
		chkbox_set_state(g_insecure, 1);
	if (!n)
		gadget_hide(g_mkfs);
	form_set_dialog(main_form, f);
retry:
	r = 0;
	switch (form_wait(f))
	{
	case 1:
		if (!*g_prefix->text || !*g_device->text || !*g_fstype->text)
		{
			msgbox(main_form, "File Systems", "All text fields must be filled in.");
			goto retry;
		}
		
		if (!chkbox_get_state(g_removable) && chkbox_get_state(g_insecure))
		{
			if (msgbox_ask(main_form, "File Systems",
				"Mounting a fixed disk filesystem in insecure mode\n"
				"is destructive as it will reset file ownership\n"
				"and permissions.\n\n"
				"Are you sure?") != MSGBOX_YES)
				goto retry;
		}
		
		strncpy(m->prefix, g_prefix->text, sizeof m->prefix - 1);
		strncpy(m->device, g_device->text, sizeof m->device - 1);
		strncpy(m->fstype, g_fstype->text, sizeof m->fstype - 1);
		m->flags = 0;
		if (chkbox_get_state(g_readonly))
			m->flags |= MF_READ_ONLY;
		if (chkbox_get_state(g_noatime))
			m->flags |= MF_NO_ATIME;
		if (chkbox_get_state(g_removable))
			m->flags |= MF_REMOVABLE;
		if (chkbox_get_state(g_insecure))
			m->flags |= MF_INSECURE;
		r = 1;
		
		if (chkbox_get_state(g_mkfs))
		{
			if (asprintf(&p, "Are you sure you want to create a new "
					 "filesystem on \"%s\"?\n\n"
					 "WARNING: This will erase all data "
					 "on this device !!",
				m->device) < 0)
				goto retry;
			if (msgbox_ask(main_form, "File Systems", p) != MSGBOX_YES)
			{
				free(p);
				goto retry;
			}
			
			if (do_mkfs(m->device))
			{
				msgbox_perror(f, "File Systems", "MKFS failed", errno);
				free(p);
				goto retry;
			}
			free(p);
			
			strcpy(m->fstype, "native");
		}
		break;
	case 3:
		sprintf(pathname, "/dev/%s", g_device->text);
		cp = dlg_disk(f, NULL, pathname, DLG_DISK_ANY);
		if (cp)
		{
			cp = strrchr(cp, '/');
			if (cp)
				input_set_text(g_device, cp + 1);
		}
		goto retry;
	}
	form_set_dialog(main_form, NULL);
	form_close(f);
	return r;
}

static void update(void)
{
	int i;
	
	list_clear(fs_list);
	for (i = 0; i < MOUNT_MAX; i++)
		if (mtab[i].mounted)
			list_newitem(fs_list, (char *)&mtab[i]);
}

static void add_click(void)
{
	int i;
	
	for (i = 0; i < MOUNT_MAX; i++)
		if (!mtab[i].mounted)
		{
			memset(&mtab[i], 0, sizeof mtab[i]);
			strcpy(mtab[i].prefix, "/mnt");
			strcpy(mtab[i].fstype, "native");
			mtab[i].mounted = 1;
retry:
			if (!edit_fs(&mtab[i], 1))
			{
				memset(&mtab[i], 0, sizeof mtab[i]);
				return;
			}
			if (_mount(REALPREFIX(mtab[i].prefix), mtab[i].device, mtab[i].fstype, mtab[i].flags))
			{
				msgbox_perror(main_form, "File Systems", "Cannot mount", errno);
				goto retry;
			}
			update();
			return;
		}
	msgbox(main_form, "File Systems", "Too many mounted file systems.");
}

static void rm_click(void)
{
	char *rp;
	int i;
	
	i = list_get_index(fs_list);
	if (i < 0 || i >= MOUNT_MAX)
		return;
	
	rp = REALPREFIX(mtab[i].prefix);
	if (!*rp && msgbox_ask(main_form, "File Systems", "Root file system is about to "
							  "be unmounted.\n\n"
							  "Do you wish to continue?") != MSGBOX_YES)
		return;
	if (_umount(rp))
	{
		msgbox_perror(main_form, "File Systems", "Cannot unmount", errno);
		return;
	}
	if (!*rp)
		sync();
	memmove(mtab + i, mtab + i + 1, sizeof *mtab * (MOUNT_MAX - i - 1));
	update();
}

static void edit_click(void)
{
	struct _mtab cm;
	char *rp;
	int i;
	
	i = list_get_index(fs_list);
	if (i < 0 || i >= MOUNT_MAX)
		return;
	cm = mtab[i];
	
	if (!edit_fs(&cm, 0))
		return;
	if (!strcmp(cm.prefix, mtab[i].prefix) &&
	    !strcmp(cm.device, mtab[i].device) &&
	    !strcmp(cm.fstype, mtab[i].fstype) &&
	    cm.flags == mtab[i].flags)
		return;
	
	rp = REALPREFIX(mtab[i].prefix);
	if (_umount(rp) && errno != ENOENT)
	{
		if (!strcmp(cm.prefix, mtab[i].prefix) &&
		    !strcmp(cm.device, mtab[i].device) &&
		    errno == EBUSY)
		{
			msgbox(main_form, "File Systems", "Save and reboot to apply the new flags.");
			mtab[i] = cm;
			update();
			return;
		}
		
		msgbox_perror(main_form, "File Systems", "Cannot unmount", errno);
		return;
	}
	
	rp = REALPREFIX(cm.prefix);
	if (_mount(rp, cm.device, cm.fstype, cm.flags))
	{
		msgbox_perror(main_form, "File Systems", "Cannot mount", errno);
		if (!*rp)
			sync();
		return;
	}
	mtab[i] = cm;
	update();
}

static void mkfs_click(void)
{
	struct bio_info bi;
	char size_str[32];
	struct _mtab *m;
	char *p;
	int fd;
	int r;
	
	m = (void *)list_get_text(fs_list, list_get_index(fs_list));
	if (!m)
		return;
	
	if (asprintf(&p, "Are you sure you want to create a new filesystem on \"%s\"?\n\n"
			 "WARNING: This will erase all data on this device !!",
			 m->device) < 0)
		return;
	r = msgbox_ask(main_form, "File Systems", p);
	free(p);
	
	if (r != MSGBOX_YES)
		return;
	
	fd = open(m->device, O_RDWR);
	if (fd < 0)
	{
		msgbox_perror(main_form, "File Systems", "Cannot open device", errno);
		return;
	}
	if (ioctl(fd, BIO_INFO, &bi))
	{
		msgbox_perror(main_form, "File Systems", "BIO_INFO failed", errno);
		close(fd);
		return;
	}
	close(fd);
	sprintf(size_str, "%llu", (long long)bi.size);
	
	p = REALPREFIX(m->prefix);
	if (_umount(p) && errno != ENOENT)
	{
		msgbox_perror(main_form, "File Systems", "Cannot unmount", errno);
		return;
	}
	
	if (do_mkfs(m->device))
		msgbox_perror(main_form, "File Systems", "MKFS failed", errno);
	else
		strcpy(m->fstype, "native");
	
	if (_mount(p, m->device, m->fstype, m->flags))
	{
		msgbox_perror(main_form, "File Systems", "Cannot mount", errno);
		if (!*p)
			sync();
		return;
	}
	update();
}

static void draw_item(struct gadget *g, int i, int wd, int x, int y, int w, int h)
{
	struct _mtab *m;
	win_color fg;
	win_color bg;
	int si;
	
	m  = (void *)list_get_text(g, i);
	si = list_get_index(g);
	if (si != i)
	{
		fg = wc_get(WC_FG);
		bg = wc_get(WC_BG);
	}
	else
	{
		if (form_get_focus(g->form) == g)
		{
			fg = wc_get(WC_SEL_FG);
			bg = wc_get(WC_SEL_BG);
		}
		else
		{
			fg = wc_get(WC_NFSEL_FG);
			bg = wc_get(WC_NFSEL_BG);
		}
	}
	
	win_rect(wd, bg, x,	y, w, h);
	win_text(wd, fg, x + 1,	y, m->prefix);
	win_text(wd, fg, x + device_pos, y, m->device);
	win_text(wd, fg, x + fstype_pos, y, m->fstype);
	win_text(wd, fg, x + flags_pos,  y, flagstr(m->flags));
}

static void on_request(struct gadget *g, int i, int b)
{
	edit_click();
}

static int on_close(struct form *f)
{
	if (save_fstab())
	{
		msgbox_perror(main_form, "File Systems",
					 "Unable to write the file system "
					 "table back to disk", errno);
		return 0;
	}
	return 1;
}

static void load_form(void)
{
	int x;
	
	main_form = form_load(MAIN_FORM);
	fs_list = gadget_find(main_form, "list");
	list_set_draw_cb(fs_list, draw_item);
	list_on_request(fs_list, on_request);
	form_on_close(main_form, on_close);
	
	x = gadget_find(main_form, "prefix_label")->rect.x;
	device_pos = gadget_find(main_form, "device_label")->rect.x - x;
	fstype_pos = gadget_find(main_form, "fstype_label")->rect.x - x;
	flags_pos  = gadget_find(main_form, "flags_label")->rect.x  - x;
}

int main(int argc, char **argv)
{
	int i;
	
	if (win_attach())
		err(255, NULL);
	
	if (geteuid())
	{
		msgbox(NULL, "File Systems", "You are not root.");
		return 0;
	}
	load_form();
	
	if (_mtab(mtab, sizeof mtab))
	{
		msgbox_perror(NULL, "File Systems", "Unable to retrieve mount table", errno);
		return 0;
	}
	for (i = 0; i < MOUNT_MAX; i++)
		if (!*mtab[i].prefix && mtab[i].mounted)
			strcpy(mtab[i].prefix, "/");
	update();
	
	if (chdir("/dev"))
	{
		msgbox_perror(NULL, "File Systems", "Cannot chdir to /dev", errno);
		return 0;
	}
	
	for (;;)
	{
		switch (form_wait(main_form))
		{
		case 0:
			break;
		case 1:
			if (save_fstab())
			{
				msgbox_perror(main_form, "File Systems", "Unable to write the file system table back to disk", errno);
				continue;
			}
			break;
		case 2:
			add_click();
			continue;
		case 3:
			rm_click();
			continue;
		case 4:
			edit_click();
			continue;
		case 5:
			mkfs_click();
			continue;
		default:
			abort();
		}
		break;
	}
	return 0;
}
