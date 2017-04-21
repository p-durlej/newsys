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
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <dev/fd.h>
#include <newtask.h>
#include <unistd.h>
#include <string.h>
#include <mount.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <paths.h>
#include <err.h>

static struct form *main_form;

static struct gadget *statlabel;
static struct gadget *drvlabel;
static struct gadget *progbar;

static const char *	pathname = "/dev/fd0,0";
static struct _mtab *	mnt;
static int		fd;

static int fmttrack(int track, int side)
{
	struct fmttrack fmt;
	int retries = 0;
	char msg[80];
	
	sprintf(msg, "Formatting track %i, side %i ...", track, side);
	label_set_text(statlabel, msg);
	win_idle();
	
	bzero(&fmt, sizeof fmt);
	fmt.cyl  = track;
	fmt.head = side;
	
retry:
	if (ioctl(fd, FDIOCFMTTRK, &fmt))
	{
		if (errno == EIO && ++retries <= 5)
		{
			sprintf(msg, "Retrying track %i, side %i ...", track, side);
			label_set_text(statlabel, msg);
			win_idle();
			goto retry;
		}
		
		sprintf(msg, "Track %i, side %i: %m", track, side);
		label_set_text(statlabel, msg);
		return -1;
	}
	bargraph_set_value(progbar, track * 2 + side);
	win_idle();
	return 0;
}

static int install_boot(void)
{
	char buf[512];
	int bfd;
	
	bfd = open("/lib/sys/noboot.bin", O_RDONLY);
	if (bfd < 0)
	{
		label_set_text(statlabel, "Cannot open noboot.bin.");
		return -1;
	}
	
	if (read(bfd, buf, sizeof buf) != sizeof buf)
	{
		label_set_text(statlabel, "Cannot read noboot.bin.");
		return -1;
	}
	
	close(bfd);
	
	lseek(fd, 0L, SEEK_SET);
	if (write(fd, buf, sizeof buf) != sizeof buf)
	{
		label_set_text(statlabel, "Cannot write boot record.");
		return -1;
	}
	
	return 0;
}

static int run_mkfs(void)
{
	pid_t pid;
	int st;
	
	label_set_text(statlabel, "Creating file system...");
	win_idle();
	
	pid = _newtaskl(_PATH_B_MKFS, _PATH_B_MKFS, "-q", pathname, "2880", (void *)NULL);
	if (pid < 0)
	{
		msgbox_perror(main_form, "Format Diskette...", "Task creation failed", errno);
		label_set_text(statlabel, "Cannot run mkfs.");
		return -1;
	}
	
	while (waitpid(pid, &st, 0) != pid)
		win_idle();
	
	if (st)
	{
		label_set_text(statlabel, "MKFS failed.");
		return -1;
	}	
	return 0;
}

static void find_mount(void)
{
	static struct _mtab mtab[MOUNT_MAX];
	
	const char *devname;
	int i;
	
	devname = strrchr(pathname, '/');
	if (devname)
		devname++;
	else
		devname = pathname;
	
	if (_mtab(mtab, sizeof mtab))
		err(1, NULL);
	
	for (i = 0; i < MOUNT_MAX; i++)
		if (!strcmp(mtab[i].device, devname))
		{
			mnt = &mtab[i];
			return;
		}
	mnt = NULL;
}

static void do_format(void)
{
	int i;
	
	label_set_text(statlabel, "Starting...");
	bargraph_set_value(progbar, 0);
	win_idle();
	
	find_mount();
	
	if (mnt && _umount(mnt->prefix))
	{
		label_set_text(statlabel, "Cannot unmount filesystem.");
		return;
	}
	
	fd = open(pathname, O_RDWR);
	if (fd < 0)
	{
		label_set_text(statlabel, "Cannot open device");
		goto fini;
	}
	
	for (i = 0; i < 80; i++)
		if (fmttrack(i, 0) || fmttrack(i, 1))
		{
			close(fd);
			goto fini;
		}
	
	if (install_boot())
		goto fini;
	
	close(fd);
	
	if (run_mkfs())
		goto fini;
	
	label_set_text(statlabel, "Formatting completed.");
	win_idle();
fini:
	if (mnt && _mount(mnt->prefix, mnt->device, mnt->fstype, mnt->flags))
		label_set_text(statlabel, "Cannot remount filesystem.");
}

static void update_drive(void)
{
	char buf[PATH_MAX + 16];
	const char *p;
	
	p = strrchr(pathname, '/');
	if (p)
		p++;
	else
		p = pathname;
	
	sprintf(buf, "Drive: %s", p);
	label_set_text(drvlabel, buf);
}

int main(int argc, char **argv)
{
	const char *p;
	
	if (argc > 1)
		pathname = argv[1];
	
	if (win_attach())
		err(1, NULL);
	
	main_form = form_load("/lib/forms/wfdfmt.frm");
	if (!main_form)
	{
		msgbox_perror(NULL, "Format Diskette", "Cannot load form", errno);
		return 1;
	}
	
	progbar	  = gadget_find(main_form, "progbar");
	statlabel = gadget_find(main_form, "status");
	drvlabel  = gadget_find(main_form, "drive");
	
	bargraph_set_labels(progbar, "", "");
	bargraph_set_limit(progbar, 159);
	
	for (;;)
		switch (form_wait(main_form))
		{
		case 1:
			form_busy(main_form);
			form_lock(main_form);
			do_format();
			form_unlock(main_form);
			form_unbusy(main_form);
			break;
		case 2:
			p = dlg_disk(main_form, NULL, pathname, DLG_DISK_FLOPPY);
			if (p)
			{
				pathname = p;
				update_drive();
			}
			break;
		default:
			return 0;
		}
}
