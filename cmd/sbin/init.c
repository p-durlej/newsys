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
#include <priv/wingui.h>
#include <priv/libc.h>
#include <priv/sys.h>
#include <sys/wait.h>
#include <newtask.h>
#include <devices.h>
#include <bioctl.h>
#include <wingui.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <mount.h>
#include <event.h>
#include <os386.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <paths.h>
#include <err.h>

#define REALPREFIX(p)		((p)[0] != '/' || (p)[1] ? (p) : "")

#define perror(msg)		_sysmesg_perror(msg)

static pid_t alt_vtty_pid;

static void _sysmesg_perror(const char *msg)
{
	_sysmesg(msg);
	_sysmesg(": ");
	_sysmesg(strerror(errno));
	_sysmesg("\n");
}

static void init_modules(void)
{
	char buf[1024];
	char *nl;
	FILE *ml;
	
	ml = fopen(_PATH_E_MODULES, "r");
	if (!ml)
	{
		perror(_PATH_E_MODULES);
		return;
	}
	
	while (fgets(buf, sizeof buf, ml))
	{
		nl = strchr(buf, '\n');
		if (nl)
			*nl = 0;
		
		if (!*buf || *buf == '#')
			continue;
		
		if (_boot_flags() & BOOT_VERBOSE)
			warnx("loading %s", buf);
		if (_mod_load(buf, NULL, 0) < 0)
			perror(buf);
	}
	
	fclose(ml);
}

static void init_desktops(void)
{
	struct dirent *de;
	char arg[PATH_MAX];
	DIR *d;
	
	d = opendir(_PATH_E_DESKTOPS);
	if (!d)
	{
		perror(_PATH_E_DESKTOPS);
		return;
	}
	
	while ((de = readdir(d)))
	{
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;
		
		strcpy(arg, _PATH_E_DESKTOPS "/");
		strcat(arg, de->d_name);
		
		if (_newtaskl(_PATH_B_IDESKTOP, _PATH_B_IDESKTOP, arg, NULL) < 0)
			perror(_PATH_B_IDESKTOP);
	}
	closedir(d);
}

static void init_font(const char *name, const char *pathname, int crit)
{
	char msg[32 + PATH_MAX];
	int ftd;
	
	if (win_load_font(&ftd, name, pathname))
	{
		perror(pathname);
		if (crit)
		{
			sprintf(msg, "init: %s: cannot load", pathname);
			_panic(msg);
		}
	}
}

static void init_default_fonts(void)
{
	init_font("mono",		_PATH_FONT_MONO,	1);
	init_font("system",		_PATH_FONT_SYSTEM,	1);
	init_font("mono-large",		_PATH_FONT_MONO_L,	0);
	init_font("system-large",	_PATH_FONT_SYSTEM_L,	0);
	init_font("mono-narrow",	_PATH_FONT_MONO_N,	0);
	init_font("mono-large-narrow",	_PATH_FONT_MONO_LN,	0);
}

static void init_fonts(void)
{
	static const char *names[] = { "mono", "system", "mono-large", "system-large", "mono-narrow", "mono-large-narrow" };
	
	char buf[PATH_MAX];
	const char *n;
	FILE *f;
	char *p;
	int cnt;
	
	f = fopen(_PATH_E_SYSFONTS, "r");
	if (!f)
	{
		if (errno != ENOENT)
			perror(_PATH_E_SYSFONTS);
		init_default_fonts();
		return;
	}
	
	cnt = 0;
	while (fgets(buf, sizeof buf, f))
	{
		p = strchr(buf, '\n');
		if (p)
			*p = 0;
		
		if (cnt < sizeof names / sizeof *names)
			n = names[cnt];
		else
		{
			n = strrchr(buf, '/');
			if (p)
				n++;
			else
				n = buf;
		}
		
		init_font(n, buf, cnt++ < 2);
	}
	fclose(f);
}

static void do_std_mounts(void)
{
	if (_mount(_PATH_M_DEV, "nodev", "dev", 0) && errno != EMOUNTED)
		perror(_PATH_M_DEV " (devfs)");
	
	if (_mount(_PATH_M_PTY, "nodev", "pty", 0) && errno != EMOUNTED)
		perror(_PATH_M_PTY " (ptyfs)");
}

static void do_auto_mount(const char *name, int removable)
{
	char pathname[PATH_MAX];
	int flags = 0;
	
	sprintf(pathname, "%s/%s", _PATH_P_MEDIA, name);
	mkdir(pathname, 0777);
	
	if (removable)
		flags |= MF_REMOVABLE | MF_INSECURE | MF_NO_ATIME;
	
	_mount(pathname, name, "native", flags);
}

static void do_auto_mounts(void)
{
	struct _mtab mtab[MOUNT_MAX];
	char pathname[PATH_MAX];
	struct bio_info bi;
	struct dirent *de;
	DIR *dir;
	int fd;
	int i;
	
	do_std_mounts();
	
	bzero(mtab, sizeof mtab);
	
	if (_mtab(mtab, sizeof mtab))
		return;
	
	dir = opendir("/dev/rdsk");
	if (!dir)
		return;
	
	while ((de = readdir(dir)))
	{
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;
		
		for (i = 0; i < MOUNT_MAX; i++)
		{
			if (!mtab[i].mounted)
				continue;
			
			if (!strcmp(mtab[i].device, de->d_name))
				goto next;
		}
		
		sprintf(pathname, "/dev/rdsk/%s", de->d_name);
		
		fd = open(pathname, O_RDWR);
		if (fd < 0)
			continue;
		
		if (!ioctl(fd, BIO_INFO, &bi))
			switch (bi.type)
			{
			case BIO_TYPE_FD:
				do_auto_mount(de->d_name, 1);
				break;
			case BIO_TYPE_HD_SECTION:
				if (bi.os != BIO_OS_SELF)
				{
					close(fd);
					continue;
				}
				do_auto_mount(de->d_name, 0);
				break;
			default:
				;
			}
		
		close(fd);
next:
		;
	}
	
	closedir(dir);
}

static void do_mount(char *s)
{
	char *prefix;
	char *device;
	char *fstype;
	char *flagp;
	int flags;
	int se;
	
	device = strtok(s,    "\t ");
	prefix = strtok(NULL, "\t ");
	fstype = strtok(NULL, "\t ");
	flagp  = strtok(NULL, "\t ");
	
	if (!flagp)
		goto bad;
	flags = 0;
	while (*flagp)
		switch(*flagp++)
		{
		case 'r':
			flags |= MF_READ_ONLY;
			break;
		case 'w':
			flags &= ~MF_READ_ONLY;
			break;
		case 'n':
			flags |= MF_NO_ATIME;
			break;
		case 'm':
			flags |= MF_REMOVABLE;
			break;
		case 'i':
			flags |= MF_INSECURE;
			break;
		default:
			goto bad;
		}
	
	if (!strcmp(prefix, "/"))
		flags |= MF_READ_ONLY;
	
	if (_umount(REALPREFIX(prefix)) && errno != ENOENT)
	{
		se = errno;
		_sysmesg(prefix);
		_sysmesg(": _umount: ");
		_sysmesg(strerror(se));
		_sysmesg("\n");
	}
	if (_mount(REALPREFIX(prefix), device, fstype, flags) && errno != EMOUNTED)
	{
		se = errno;
		_sysmesg(prefix);
		_sysmesg(": _mount: ");
		_sysmesg(strerror(se));
		_sysmesg("\n");
	}
	return;
bad:
	_sysmesg(_PATH_E_FSTAB ": invalid format\n");
}

static void do_mounts(void)
{
	struct stat st;
	ssize_t cnt;
	char *buf;
	char *p1;
	char *p;
	int fd = -1;
	
	if (_boot_flags() & BOOT_SINGLE)
		goto std;
	
	fd = open(_PATH_E_FSTAB, O_RDONLY);
	if (fd < 0)
	{
		if (errno != ENOENT)
			perror(_PATH_E_FSTAB ": open");
		do_auto_mounts();
		return;
	}
	fstat(fd, &st);
	
	buf = malloc(st.st_size + 1);
	if (!buf)
	{
		perror(_PATH_E_FSTAB ": malloc");
		goto std;
	}
	buf[st.st_size] = 0;
	
	cnt = read(fd, buf, st.st_size);
	if (cnt < 0)
	{
		perror(_PATH_E_FSTAB ": read");
		free(buf);
		close(fd);
		goto std;
	}
	close(fd);
	
	if (cnt != st.st_size)
	{
		_sysmesg(_PATH_E_FSTAB ": read: Short read\n");
		free(buf);
		goto std;
	}
	
	p = buf;
	for (;;)
	{
		p1 = strchr(p, '\n');
		if (p1)
			*p1 = 0;
		while (isspace(*p))
			p++;
		if (*p && *p != '#')
			do_mount(p);
		if (!p1)
			break;
		p = p1 + 1;
	}
	free(buf);
	
std:
	if (fd >= 0)
		close(fd);
	do_std_mounts();
}

static void sysinstall(void)
{
	struct stat st;
	ssize_t cnt;
	char *buf;
	char *p;
	int fd;
	
	fd = open(_PATH_E_SS, O_RDONLY);
	if (fd < 0)
	{
		perror(_PATH_E_SS);
		_panic("init: " _PATH_E_SS ": cannot open");
	}
	
	memset(&st, 0x55, sizeof st);
	fstat(fd, &st);
	
	buf = malloc(st.st_size + 1);
	if (!buf)
	{
		perror(_PATH_E_SS);
		_panic("init: memory allocation failed");
	}
	
	cnt = read(fd, buf, st.st_size);
	if (cnt != st.st_size)
	{
		perror(_PATH_E_SS);
		_panic("init: " _PATH_E_SS ": cannot read");
	}
	buf[st.st_size] = 0;
	close(fd);
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("init: starting sysinstall\n");
	p = strchr(buf, '\n');
	if (p)
		*p = 0;
	
	p = strchr(buf, ' ');
	if (p)
	{
		*p = 0;
		execl(buf, buf, p + 1, (void *)NULL);
	}
	else
		execl(buf, buf, (void *)NULL);
	
	perror(buf);
	_panic("init: cannot exec sysinstall");
}

static void add_blocks(void)
{
	char buf[64];
	int nbuf;
	int fd = 123;
	
	memset(buf, 0, sizeof buf);
	
	fd = open("/etc/cache", O_RDONLY);
	if (fd < 0)
	{
		perror("/etc/cache: open");
		return;
	}
	if (read(fd, buf, sizeof buf - 1) < 0)
	{
		perror("/etc/cache: read");
		close(fd);
		return;
	}
	nbuf = strtoul(buf, NULL, 0);
	close(fd);
	
	if (_boot_flags() & BOOT_VERBOSE)
	{
		sprintf(buf, "init: block cache size is %i\n", nbuf);
		_sysmesg(buf);
	}
	
	if (_blk_add(nbuf))
		perror("init: _blk_add");
}

static void init_environ(void)
{
	char buf[256];
	char *p;
	FILE *f;
	
	setenv("HOME", _PATH_H_DEFAULT, 1);
	
	f = fopen("/etc/environ", "r");
	if (!f)
	{
		perror("/etc/environ");
		return;
	}
	while (fgets(buf, sizeof buf, f))
	{
		p = strchr(buf, '\n');
		if (p)
			*p = 0;
		putenv(strdup(buf));
	}
	fclose(f);
}

static void init_clock(void)
{
	int status;
	pid_t pid;
	
	pid = _newtaskl(_PATH_B_HWCLOCK, _PATH_B_HWCLOCK, "r", NULL);
	if (pid < 0)
		perror(_PATH_B_HWCLOCK);
	
	while (wait(&status) != pid);
}

static void init_fsi(void)
{
	int status;
	pid_t pid;
	
	pid = _newtaskl(_PATH_B_MKFSI, _PATH_B_MKFSI, NULL);
	if (pid < 0)
	{
		perror(_PATH_B_MKFSI);
		return;
	}
	
	while (wait(&status) != pid);
}

static void fsck(void)
{
	int status;
	pid_t pid;
	
	pid = _newtaskl(_PATH_B_FSCK, _PATH_B_FSCK, "-sqwF", NULL);
	if (pid < 0)
		perror(_PATH_B_FSCK);
	
	while (wait(&status) != pid);
}

static void spawn_vtty(void)
{
	alt_vtty_pid = _newtaskl(_PATH_B_VTTY, _PATH_B_VTTY, "-f", NULL);
	if (alt_vtty_pid < 0)
		warn("%s", _PATH_B_VTTY);
}

static void alt_sig_chld(int nr)
{
	int status;
	pid_t pid;
	
	while (pid = waitpid(-1, &status, WNOHANG), pid)
		if (pid == alt_vtty_pid)
		{
			alt_vtty_pid = 0;
			spawn_vtty();
		}
	signal(SIGCHLD, alt_sig_chld);
}

static void load_module(const char *pathname, void *data, size_t data_size)
{
	char msg[32 + PATH_MAX];
	
	if (_mod_load(pathname, data, data_size) < 0)
	{
		perror(pathname);
		
		sprintf(msg, "init: %s: cannot load", pathname);
		_panic(msg);
	}
}

static void alt_boot(void)
{
	struct device pckbd;
	struct device mouse;
	struct device comm;
	struct device vga;
	struct event e;
	int desk_w;
	int desk_h;
	int wd;
	int i;
	
	do_std_mounts();
	
	memset(&pckbd, 0, sizeof pckbd);
	memset(&mouse, 0, sizeof mouse);
	memset(&vga,   0, sizeof vga);
	for (i = 0; i < 4; i++)
	{
		pckbd.irq_nr[i] = -1U;
		pckbd.dma_nr[i] = -1U;
		mouse.irq_nr[i] = -1U;
		mouse.dma_nr[i] = -1U;
		vga.irq_nr[i]   = -1U;
		vga.dma_nr[i]   = -1U;
	}
	
	pckbd.io_base[0] = 0x60;
	pckbd.io_size[0] = 0x10;
	pckbd.irq_nr[0]  = 1;
	pckbd.irq_nr[1]  = 12;
	
	mouse.io_base[0] = 0x3f8;
	mouse.io_size[0] = 8;
	mouse.irq_nr[0]  = 4;
	
	strcpy(comm.name, "comm1");
	comm.io_base[0] = 0x2f8;
	comm.io_size[0] = 8;
	comm.irq_nr[0]  = 3;
	
	vga.io_base[0]   = 0x3c0;
	vga.io_size[0]   = 0x20;
	
	init_default_fonts();
	
	if (win_newdesktop("single"))
	{
		warn("win_newdesktop");
		_panic("init: desktop creation failed");
	}
	
#ifdef __ARCH_I386__
	load_module("/lib/drv/vga.drv", &vga, sizeof vga);
#elif defined __ARCH_AMD64__
	load_module("/lib/drv/framebuf.sys",	NULL, 0);
	load_module("/lib/drv/bootfb.drv",	&vga, sizeof vga);
#else
#error Unknown arch
#endif
	load_module("/lib/drv/pckbd.drv",  &pckbd, sizeof pckbd);
	load_module("/lib/drv/mouse.drv",  &mouse, sizeof mouse);
	load_module("/lib/drv/serial.drv", &comm,  sizeof comm);
	
	wc_load_cte();
	
	win_desktop_size(&desk_w, &desk_h);
	win_creat(&wd, 1, 0, 0, desk_w, desk_h, NULL, NULL);
	
	putenv("PATH=/bin:/sbin");
	
	signal(SIGCHLD, alt_sig_chld);
	spawn_vtty();
	
	for (;;)
	{
		if (!_evt_wait(&e) && e.type == E_WINGUI && e.win.type == WIN_E_REDRAW)
		{
			win_color bg;
			
			win_rgb2color(&bg, 128, 128, 128);
			win_clip(wd, e.win.redraw_x, e.win.redraw_y, e.win.redraw_w, e.win.redraw_h, 0, 0);
			win_paint();
			win_rect(wd, bg, 0, 0, desk_w, desk_h);
			win_end_paint();
		}
	}
}

static void find_devs(void)
{
	pid_t pid;
	int st;
	
	win_newdesktop("find_devs");
	
	pid = _newtaskl(_PATH_B_FIND_DEVS, _PATH_B_FIND_DEVS, (void *)NULL);
	if (pid < 0)
	{
		perror(_PATH_B_FIND_DEVS);
		_panic("init: " _PATH_B_FIND_DEVS ": cannot execute");
	}
	while (waitpid(pid, &st, 0) <= 0)
		evt_idle();
	
	execl(_PATH_B_SHUTDOWN, _PATH_B_SHUTDOWN, "-r", (void *)NULL);
	warn("%s", _PATH_B_SHUTDOWN);
}

static void sig_down(int nr)
{
	char *opt = "-h";
	int i;
	
	switch (nr)
	{
	case SIGINT:
		opt = "-r";
		break;
	case SIGUSR1:
		opt = "-h";
		break;
	case SIGUSR2:
		opt = "-p";
		break;
	default:
		abort();
	}
	
	for (i = 1; i <= NSIG; i++)
		signal(i, SIG_IGN);
	
	execl(_PATH_B_SHUTDOWN, _PATH_B_SHUTDOWN, opt, (void *)NULL);
	warn("%s", _PATH_B_SHUTDOWN);
}

static void sig_alrm(int nr)
{
	if (sync())
		warn("sync");
	signal(SIGALRM, sig_alrm);
	alarm(15);
}

int main()
{
	int status;
	int i;
	
	if (getpid() != 1)
	{
		if (!win_attach())
			msgbox(NULL, "init", "This program should not be called directly.");
		errx(255, "this program should not be called directly");
	}
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("init: booting...\n");
	
	for (i = 0; i < OPEN_MAX; i++)
		close(i);
	
	signal(SIGUSR1, sig_down);
	signal(SIGUSR2, sig_down);
	signal(SIGINT,  sig_down);
	signal(SIGCHLD, SIG_DFL);
	signal(SIGEVT,  SIG_DFL);
	umask(077);
	
	if (access(_PATH_E_SS, 0))
	{
		if ((_boot_flags() & BOOT_SINGLE))
			alt_boot();
	}
	
	if (!access("/etc/cache", 0))
		add_blocks();
	
	do_mounts();
	
	if (open("/dev/console", O_RDONLY))
	{
		_cprintf("init: /dev/console: %m\n");
		_panic("init: /dev/console: cannot open");
	}
	if (open("/dev/console", O_WRONLY) != 1)
	{
		_cprintf("init: /dev/console: %m\n");
		_panic("init: /dev/console: cannot open");
	}
	if (open("/dev/console", O_WRONLY) != 2)
	{
		_cprintf("init: /dev/console: %m\n");
		_panic("init: /dev/console: cannot open");
	}
	
	init_environ();
	fsck();
	
	if (!access(_PATH_E_SS, 0))
		sysinstall();
	
	if ((_boot_flags() & BOOT_DETECT))
		find_devs();
	
	if (!access(_PATH_E_BSCRIPT, 0))
	{
		if (_boot_flags() & BOOT_VERBOSE)
			_sysmesg("init: starting " _PATH_E_BSCRIPT "\n");
		if (_newtaskl(_PATH_B_SH, _PATH_B_SH, _PATH_E_BSCRIPT, NULL) < 0)
			perror(_PATH_B_SH);
		while (wait(&status) < 1);
	}
	
	unlink(_PATH_V_DEVICES);
	
	init_modules();
	init_clock();
	init_fonts();
	
	if (_boot_flags() & BOOT_TEXT)
	{
		if (_newtaskl(_PATH_B_VTTY_CON, _PATH_B_VTTY_CON, "-l", NULL) < 0)
			perror(_PATH_B_VTTY_CON);
	}
	else
	{
		init_fsi();
		init_desktops();
	}
	
	sig_alrm(-1);
	for (;;)
		wait(&status);
}
