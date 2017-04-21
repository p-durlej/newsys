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

#include <machine/sysinstall.h>
#include <machine/machdef.h>
#include <config/sysinstall.h>
#include <config/defaults.h>
#include <priv/copyright.h>
#include <priv/wingui.h>
#include <priv/natfs.h>
#include <wingui_msgbox.h>
#include <wingui_bitmap.h>
#include <wingui_color.h>
#include <wingui_form.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dev/rd.h>
#include <devices.h>
#include <newtask.h>
#include <mount.h>
#include <limits.h>
#include <string.h>
#include <wingui.h>
#include <unistd.h>
#include <dirent.h>
#include <bioctl.h>
#include <stdlib.h>
#include <confdb.h>
#include <ctype.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <paths.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <event.h>
#include <os386.h>
#include <err.h>

#define BLK_SIZE	512

#define MOUNT		"/mnt"
#define SOURCE		"/src"

#define TITLE		"SysInstall"

#define WELCOME		"Welcome to SysInstall!\n\n"							\
			"SysInstall is a program to guide you through\n"				\
			"the system setup procedure.\n\n"						\
			"Alternatively you can run the system in the\n"					\
			"single user mode.\n\n"								\
			"Click No, if you want to start the single user\nmode.\n\n"			\
			"Clock Yes, if you want to begin the operating\nsystem installation.\n\n"	\
			"Do you want to start the installation?"

#define WIDTH		300
#define HEIGHT		170

#define PART_MAX	32

#define RD_SIZE_STR	"2048"
#define RD_SIZE		2048
#define RD_MOUNT	"/tmp"
#define RD		"rd0"

#define FS_RSVD		768
#define FS_RSVD_STR	"768"

#ifndef SYSINST_BOOT_DEV
#define SYSINST_BOOT_DEV	NULL
#endif

#ifndef SYSINST_ROOT_DEV
#define SYSINST_ROOT_DEV	NULL
#endif

#ifndef SYSINST_INST_DEV
#define SYSINST_INST_DEV	"fd0,0"
#endif

#ifndef SYSINST_INST_FSTYPE
#define SYSINST_INST_FSTYPE	"boot"
#endif

void sig_chld(int nr);

struct part
{
	blk_t	offset;
	blk_t	size;
	int	os_type;
	char	dev[NAME_MAX + 1];
	char	disk[NAME_MAX + 1];
	char	desc[128];
} part[PART_MAX];

int part_count;

static struct bitmap *backdrop;
int backdrop_wd;

char *inst_fstype = SYSINST_INST_FSTYPE;
char *inst_dev = SYSINST_INST_DEV;
char *boot_dev = SYSINST_BOOT_DEV;
char *root_dev = SYSINST_ROOT_DEV;
char *disk_dev;

char *script_p;
char *script;

static struct win_rect wsr;

int desk_w;
int desk_h;

#define perror(msg)	_sysmesg_perror(msg)

void _sysmesg_perror(const char *msg)
{
	_sysmesg(msg);
	_sysmesg(": ");
	_sysmesg(strerror(errno));
	_sysmesg("\n");
}

#define CONT		1
#define MKFS		2
#define DETAILS		3
#define FDISK		4

void shutdown(void)
{
	int i;
	
	for (i = 0; i < OPEN_MAX; i++)
		close(i);
	
	_umount(SOURCE);
	_umount(MOUNT);
	_umount(_PATH_M_PTY);
	_umount(_PATH_M_DEV);
	_umount("");
	_shutdown(SHUTDOWN_HALT);
}

void fail(void)
{
	msgbox(NULL, TITLE, "An unrecoverable error caused SysInstall to fail.\n"
			    "The system may be unable to boot.\n\n"
			    "Press OK to shutdown.");
	win_idle();
	shutdown();
}

int x_open(char *pathname, int flags, int mode)
{
	char buf[256 + PATH_MAX];
	int fd;
	
	fd = open(pathname, flags, mode);
	if (fd < 0)
	{
		sprintf(buf, "Cannot open \"%s\"", pathname);
		msgbox_perror(NULL, TITLE, buf, errno);
		fail();
	}
	return fd;
}

int x_close(int fd)
{
	return close(fd);
}

int x_read(int fd, char *buf, int len)
{
	char msg[256];
	int cnt;
	
retry:
	cnt = read(fd, buf, len);
	if (cnt < 0)
	{
		sprintf(msg, "Read error:\n\n%m\n\nTry again?");
		if (msgbox_ask(NULL, TITLE, msg) == MSGBOX_YES)
			goto retry;
		fail();
	}
	
	return cnt;
}

int x_write(int fd, char *buf, int len)
{
	char msg[256];
	
	if (write(fd, buf, len) != len)
	{
		sprintf(msg, "Write error:\n\n%m");
		msgbox(NULL, TITLE, msg);
		fail();
	}
	
	return len;
}

void part_detail(struct part *pi)
{
	struct gadget *g;
	struct form *f;
	char size_str[256];
	char off_str[256];
	char os_str[5];
	
	sprintf(size_str, "%i (%i MBytes)", pi->size, pi->size / 2048);
	sprintf(off_str, "%i", pi->offset);
	if (pi->os_type > 0)
		sprintf(os_str, "0x%02x", pi->os_type & 0xff);
	else
		strcpy(os_str, "N/A");
	
	f = form_creat(FORM_TITLE | FORM_FRAME, 0, -1, -1, 240, 74, "Partition information");
	
	label_creat(f, 2,  2, "Device name:");
	label_creat(f, 2, 15, "First block:");
	label_creat(f, 2, 28, "Size:");
	label_creat(f, 2, 41, "OS type:");
	
	label_creat(f, 80,  2, pi->dev);
	label_creat(f, 80, 15, off_str);
	label_creat(f, 80, 28, size_str);
	label_creat(f, 80, 41, os_str);
	
	g = button_creat(f, 2, 56, 60, 16, "OK", NULL);
	button_set_result(g, CONT);
	
	form_wait(f);
	form_close(f);
}

int mkfs(struct part *pi)
{
	char size_str[16];
	char msg[256];
	int status;
	
	sprintf(msg, "Are you sure you want to create a new filesystem on \"%s\"?\n\n"
		     "WARNING: This will erase all data on this device !!", pi->dev);
	
	if (msgbox_ask(NULL, TITLE, msg) != MSGBOX_YES)
		return 0;
	win_idle();
	
	sprintf(size_str, "%i", pi->size);
	
	signal(SIGCHLD, SIG_DFL);
	if (_newtaskl(_PATH_B_MKFS, _PATH_B_MKFS, "-q", pi->dev, size_str, FS_RSVD_STR, (void *)NULL) < 0)
	{
		sprintf(msg, "Cannot run \"" _PATH_B_MKFS "\":\n\n%m");
		msgbox(NULL, TITLE, msg);
	}
	else
	{
		while (wait(&status) < 0);
		signal(SIGCHLD, sig_chld);
		
		if (status)
			msgbox(NULL, TITLE,
				"The mkfs program did not succeed.\n\n"
				"Please verify whether the drives are correctly\n"
				"connected and powered and try again.");
		else
			msgbox(NULL, TITLE, "New filesystem has been created.");
	}
	
	return 1;
}

int is_valid_root(struct part *pi)
{
	char msg[256];
	
	if (pi->os_type != BIO_OS_SELF)
	{
		sprintf(msg, "\"%s\" is not a valid system partition.", pi->dev);
		msgbox(NULL, TITLE, msg);
		return 0;
	}
	return 1;
}

int has_room_for_kern(struct part *pi)
{
	struct nat_super
	{
		char magic[8];
		unsigned fat_block;
		unsigned fat_size;
		unsigned data_block;
		unsigned data_size;
		unsigned root_block;
	} super;
	char path[PATH_MAX];
	char msg[256];
	int fd;

	strcpy(path, "/dev/");
	strcat(path, pi->dev);
	fd = open(path, O_RDWR);
	if (fd < 0)
	{
		sprintf(msg, "Cannot open \"%s\":\n\n%m", pi->dev);
		msgbox(NULL, TITLE, msg);
		return 0;
	}
	lseek(fd, BLK_SIZE, SEEK_SET);
	if (read(fd, &super, sizeof super) != sizeof super)
	{
		sprintf(msg, "Cannot read \"%s\":\n\n%m", pi->dev);
		msgbox(NULL, TITLE, msg);
		close(fd);
		return 0;
	}
	close(fd);
	
	if (memcmp(super.magic, NAT_MAGIC, sizeof super.magic))
	{
		if (msgbox_ask(NULL, TITLE, "There is no filesystem on the device.\n\nCreate a new filesystem?") == MSGBOX_YES)
			return mkfs(pi);
		return 0;
	}
	
	if (super.fat_block < FS_RSVD || super.data_block < FS_RSVD)
	{
		sprintf(msg, "There is not enough room for loader on \"%s\".", pi->dev);
		msgbox(NULL, TITLE, msg);
		return 0;
	}
	return 1;
}

static void sig_evt(int nr)
{
	signal(SIGEVT, sig_evt);
	win_idle();
}

static void fdisk(struct form *form, const char *disk)
{
	char otbl[64], ntbl[64];
	pid_t pid;
	void *clh;
	int st;
	int fd;
	
	fd = open(disk, O_RDONLY);
	if (fd < 0)
		goto dk_err;
	if (lseek(fd, 512 - 64 - 2, SEEK_SET) < 0)
		goto dk_err;
	if (read(fd, otbl, sizeof otbl) != sizeof otbl)
		goto dk_err;
	
	clh = signal(SIGCHLD, SIG_DFL);
	pid = _newtaskl(_PATH_B_VTTY, _PATH_B_VTTY, "-Mwc70", _PATH_B_FDISK, disk, (void *)NULL);
	
	signal(SIGEVT, sig_evt);
	while (wait(&st) != pid);
	signal(SIGCHLD, clh);
	signal(SIGEVT, SIG_DFL);
	
	if (lseek(fd, 512 - 64 - 2, SEEK_SET) < 0)
		goto dk_err;
	if (read(fd, ntbl, sizeof ntbl) != sizeof ntbl)
		goto dk_err;
	close(fd);
	
	if (memcmp(otbl, ntbl, sizeof otbl))
	{
		msgbox(form, TITLE, "Partition table was modified.\n\n"
				    "The system will now reboot.");
		sync();
		_shutdown(SHUTDOWN_REBOOT);
	}
	
	return;
dk_err:
	msgbox_perror(form, TITLE, "Disk error", errno);
	close(fd);
}

static int is_empty_disk(struct form *f, struct part *disk)
{
	char buf[512];
	int fd;
	int i;
	
	if (disk->offset)
		return 0;
	
	fd = open(disk->dev, O_RDONLY);
	if (fd < 0)
	{
		msgbox_perror(f, TITLE, "Cannot open disk", errno);
		return 0;
	}
	
	if (read(fd, buf, sizeof buf) != sizeof buf)
	{
		msgbox_perror(f, TITLE, "Cannot read disk", errno);
		close(fd);
		return 0;
	}
	
	close(fd);
	
	for (i = 0; i < 512; i++)
		if (buf[i])
			return 0;
	return 1;
}

static void init_empty_disk(struct form *f, struct part *disk)
{
	void *clh;
	pid_t pid;
	int st;
	
	if (msgbox_ask(f, TITLE, "The selected disk appears empty.\n\nInitialize with defaults?") != MSGBOX_YES)
		return;
	
	clh = signal(SIGCHLD, SIG_DFL);
	pid = _newtaskl(_PATH_B_FDISK, _PATH_B_FDISK, "-i", disk->dev, (void *)NULL);
	
	signal(SIGEVT, sig_evt);
	while (wait(&st) != pid);
	signal(SIGCHLD, clh);
	signal(SIGEVT, SIG_DFL);
	
	if (st)
	{
		msgbox(f, TITLE, "Failed to initialize the disk.");
		return;
	}
	
	msgbox(f, TITLE, "Partition table was modified.\n\n"
			 "The system will now reboot.");
	sync();
	_shutdown(SHUTDOWN_REBOOT);
}

void partition(void)
{
	struct gadget *li;
	struct gadget *b;
	struct form *f;
	char msg[256];
	int form_x;
	int form_y;
	int r;
	int i;
	
	if (root_dev)
		return;
	
	if (chdir("/dev"))
	{
		sprintf(msg, "\"/dev\" is not available:\n\n%m");
		msgbox(NULL, TITLE, msg);
		fail();
	}
	
	form_x = (wsr.w - WIDTH)  / 2;
	form_y = (wsr.h - HEIGHT) / 2;
	
	f = form_creat(FORM_FRAME | FORM_TITLE, 1, form_x, form_y, WIDTH, HEIGHT, TITLE);
	label_creat(f, 2, 2, "Please select a partition or an empty disk to install\nthe system to:");
	
	li = list_creat(f, 2, 26, WIDTH - 4, HEIGHT - 46);
	list_set_flags(li, LIST_FRAME | LIST_VSBAR);
	
	if (!part_count)
	{
		msgbox(f, TITLE, "No disk partitions.\n\n"
				 "Please reboot your computer,\n"
				 "create partitons and run\n"
				 "SysInstall again.\n\n"
				 "SysInstall will now exit.");
		fail();
	}
	
	for (i = 0; i < part_count; i++)
		list_newitem(li, part[i].desc);
	
	b = button_creat(f, 2, HEIGHT - 18, 60, 16, "Continue", NULL);
	button_set_result(b, CONT);
	
	b = button_creat(f, 64, HEIGHT - 18, 60, 16, "Details", NULL);
	button_set_result(b, DETAILS);
	
	b = button_creat(f, 126, HEIGHT - 18, 60, 16, "MkFS", NULL);
	button_set_result(b, MKFS);
	
	b = button_creat(f, 188, HEIGHT - 18, 60, 16, "FDisk", NULL);
	button_set_result(b, FDISK);
	
restart:
	
	do
	{
		r = form_wait(f);
		i = list_get_index(li);
		
		switch (r)
		{
		case DETAILS:
			form_lock(f);
			if (i >= 0)
				part_detail(&part[i]);
			else
				msgbox(f, TITLE, "Please select a partition.");
			form_unlock(f);
			break;
		case MKFS:
			form_lock(f);
			if (i < 0)
			{
				msgbox(f, TITLE, "Please select a partition.");
				form_unlock(f);
				continue;
			}
			if (!is_valid_root(&part[i]))
			{
				form_unlock(f);
				continue;
			}
			mkfs(&part[i]);
			form_unlock(f);
			break;
		case FDISK:
			if (i < 0 || *part[i].disk)
			{
				msgbox(f, TITLE, "Please select a disk.");
				continue;
			}
			fdisk(f, part[i].dev);
			break;
		}
	} while (r != CONT);
	
	form_lock(f);
	if (i < 0)
	{
		msgbox(f, TITLE, "Please select a partition or an empty disk.");
		form_unlock(f);
		goto restart;
	}
	
	if (is_empty_disk(f, &part[i]))
	{
		init_empty_disk(f, &part[i]);
		form_unlock(f);
		goto restart;
	}
	
	if (!is_valid_root(&part[i]) || !has_room_for_kern(&part[i]))
	{
		form_unlock(f);
		goto restart;
	}
	form_unlock(f);
	
	root_dev = strdup(part[i].dev);
	disk_dev = strdup(part[i].disk);
	
	form_close(f);
	win_idle();
	chdir("/");
}

void copy_file(char *src, char *dst, mode_t mode)
{
	static char buf[262144];
	
	int src_fd;
	int dst_fd;
	int cnt;
	
#if SYSINST_DEBUG
	warnx("copying %s -> %s", src, dst);
#endif
	
	src_fd = x_open(src, O_RDONLY, 0);
	dst_fd = x_open(dst, O_WRONLY | O_CREAT | O_TRUNC, mode);
	
	while ((cnt = x_read(src_fd, buf, sizeof buf)))
	{
		x_write(dst_fd, buf, cnt);
		win_idle();
	}
	
	x_close(src_fd);
	x_close(dst_fd);
}

#if defined __MACH_I386_PC__ || defined __MACH_AMD64_PC__
void install_boot(void)
{
	static char buf[(FS_RSVD - 2) * BLK_SIZE];
	static char msg[1024];

	struct bio_info bi;
	int src_fd;
	int dst_fd;
	int cnt;

	chdir("/dev");
	dst_fd = x_open(root_dev, O_RDWR, 0);
	if (dst_fd < 0)
	{
		sprintf(msg, "Cannot open \"%s\":\n\n%m", root_dev);
		msgbox(NULL, TITLE, msg);
		fail();
	}
	if (ioctl(dst_fd, BIO_INFO, &bi))
	{
		sprintf(msg, "Cannot get info about \"%s\":\n\n%m", root_dev);
		msgbox(NULL, TITLE, msg);
		fail();
	}

	src_fd = x_open(_PATH_I386_BB, O_RDONLY, 0);

	cnt = x_read(src_fd, buf, BLK_SIZE);
	if (cnt != BLK_SIZE)
	{
		msgbox(NULL, TITLE, "Cannot read \"" _PATH_I386_BB "\": \n\nShort read");
		fail();
	}

	buf[4] =  bi.offset + 2;
	buf[5] = (bi.offset + 2) >> 8;
	buf[6] = (bi.offset + 2) >> 16;
	buf[7] = (bi.offset + 2) >> 24;

	x_write(dst_fd, buf, cnt);
	x_close(src_fd);

	src_fd = x_open(_PATH_L_SYSLOAD, O_RDONLY, 0);
	cnt = x_read(src_fd, buf, sizeof buf);
	if (lseek(dst_fd, 2L * BLK_SIZE, SEEK_SET) < 0L)
	{
		sprintf(msg, "Cannot seek on \"%s\":\n\n%m", root_dev);
		msgbox(NULL, TITLE, msg);
		fail();
	}
	x_write(dst_fd, buf, cnt);
	x_close(src_fd);

	x_close(dst_fd);
}
#else
#error	unsupported machine type
#endif

void install_mbr(void)
{
	char pathname[PATH_MAX];
	char msg[PATH_MAX + 64];
	uint8_t ipl[446];
	uint8_t buf[512];
	ssize_t cnt;
	int d;
	int i;
	
	memset(ipl, 0, sizeof ipl);
	
	d = open(_PATH_I386_IPL, O_RDONLY);
	if (d < 0)
	{
		msgbox_perror(NULL, TITLE, "Cannot open " _PATH_I386_IPL, errno);
		fail();
	}
	cnt = read(d, ipl, sizeof ipl);
	if (cnt < 0)
	{
		msgbox_perror(NULL, TITLE, "Cannot read " _PATH_I386_IPL, errno);
		fail();
	}
	close(d);
	
	sprintf(pathname, "/dev/%s", disk_dev);
	d = open(pathname, O_RDWR);
	if (d < 0)
	{
		sprintf(msg, "Cannot open %s", pathname);
		msgbox_perror(NULL, TITLE, msg, errno);
		fail();
	}
	
	cnt = read(d, buf, sizeof buf);
	if (cnt < 0)
	{
		sprintf(msg, "Cannot read %s", pathname);
		msgbox_perror(NULL, TITLE, msg, errno);
		fail();
	}
	
	for (i = 0; i < sizeof ipl; i++)
		if (buf[i] != ipl[i])
			break;
	
	if (i == sizeof ipl && buf[510] == 0x55 && buf[511] == 0xaa)
	{
		close(d);
		return;
	}
	
	for (i = 0; i < sizeof ipl; i++)
		if (buf[i])
			break;
	
	if (i < sizeof ipl && msgbox_ask(NULL, TITLE, "Do you want to reinstall the IPL code in the MBR?") != MSGBOX_YES)
	{
		close(d);
		return;
	}
	
	lseek(d, 0, SEEK_SET);
	cnt = write(d, ipl, sizeof ipl);
	if (cnt < 0)
		goto werr;
	
	lseek(d, 510, SEEK_SET);
	cnt = write(d, (char []){ 0x55, 0xaa }, 2);
	if (cnt < 0 || close(d))
		goto werr;
	return;
werr:
	sprintf(msg, "Cannot write %s", pathname);
	msgbox_perror(NULL, TITLE, msg, errno);
	fail();
}

static void update_mods(void)
{
	char buf[PATH_MAX];
	int fb_present = 0;
	mode_t m;
	FILE *f;
	
	m = umask(077);
	f = fopen(MOUNT "/etc/modules", "a+");
	umask(m);
	if (f == NULL)
	{
		msgbox_perror(NULL, TITLE, "Cannot open /etc/modules", errno);
		fail();
	}
	rewind(f);
	
	while (fgets(buf, sizeof buf, f))
	{
		if (!strcmp(buf, "/lib/drv/framebuf.sys\n"))
		{
			fb_present = 1;
			continue;
		}
	}
	fseek(f, 0, SEEK_CUR);
	
	if (!fb_present)
	{
		_sysmesg("update_mods: adding /lib/drv/framebuf.sys\n");
		fputs("/lib/drv/framebuf.sys\n", f);
	}
	
	if (fclose(f))
	{
		msgbox_perror(NULL, TITLE, "Closing /etc/modules", errno);
		fail();
	}
}

void bootgen(void)
{
	mode_t m;
	FILE *f;
	int r;
	
	if (!access(MOUNT _PATH_E_BOOT_CONF, 0))
	{
		r = msgbox_ask4(NULL, TITLE, _PATH_E_BOOT_CONF " already exists on the target filesystem.\n\nOverwrite?", MSGBOX_NO);
		if (r != MSGBOX_YES)
			return;
	}
	
	m = umask(077);
	f = fopen(MOUNT _PATH_E_BOOT_CONF, "w");
	umask(m);
	fprintf(f, "rdev=%s\n", root_dev);
	fprintf(f, "bdev=%s\n", boot_dev ? boot_dev : root_dev);
	fprintf(f, "rtype=native\n");
	fclose(f);
}

void backdrop_redraw(void)
{
	win_color fg;
	win_color bg;
	int w, h;
	
	win_text_size(WIN_FONT_DEFAULT, &w, &h, COPYRIGHT);
	win_paint();
	
	win_rgb2color(&fg, 255, 255, 255);
	bg = wc_get(WC_DESKTOP);
	
	win_rect(backdrop_wd, bg, 0, 0, wsr.w, wsr.h);
	
	if (backdrop)
		bmp_draw(backdrop_wd, backdrop, 0, 0);
	
	win_text(backdrop_wd, fg, 0, wsr.h - h, COPYRIGHT);
	win_end_paint();
}

static void on_setmode()
{
	if (backdrop)
		bmp_conv(backdrop);
	wc_load_cte();
	wp_load();
}

static void on_resize()
{
	win_ws_getrect(&wsr);
	win_change(backdrop_wd, 1, wsr.x, wsr.y, wsr.w, wsr.h);
}

void backdrop_proc(struct event *e)
{
	switch (e->win.type)
	{
	case WIN_E_REDRAW:
		win_clip(e->win.wd, e->win.redraw_x, e->win.redraw_y,
				    e->win.redraw_w, e->win.redraw_h, 0, 0);
		backdrop_redraw();
		break;
	}
}

static int ec_setmode1(int w, int h)
{
	struct win_modeinfo mi;
	int m;
	
	m = 0;
	while (!win_modeinfo(m, &mi))
	{
		if (mi.width == w && mi.height == h && mi.ncolors == 16777216)
		{
			win_setmode(m, -1);
			win_desktop_size(&desk_w, &desk_h);
			return 0;
		}
		m++;
	}
	return -1;
}

static int ec_setmode(int hires)
{
	struct win_modeinfo mi;
	int m;
	
	win_getmode(&m);
	win_modeinfo(m, &mi);
	
	if (hires && !ec_setmode1(1024, 768))
		return 0;
	
	return ec_setmode1(mi.width, mi.height);
}

static void eyecandy(void)
{
	char pathname[PATH_MAX];
#if SYSINST_SETMODE
	ec_setmode(0);
#endif
	
	sprintf(pathname, "/lib/bg/bg%ix%i.pgm", desk_w, desk_h);
	
	backdrop = bmp_load(pathname);
	if (!backdrop)
		backdrop = bmp_load("/lib/bg/bg640x480.pgm");
	
	if (backdrop)
		bmp_conv(backdrop);
}

void create_dirs(const char *list)
{
	char buf[1024];
	char msg[256];
	FILE *f;
	
	if (!list)
		list = _PATH_E_SS_MKDIR;
	
	f = fopen(list, "r");
	if (!f)
	{
		sprintf(msg, "Unable to open " _PATH_E_SS_MKDIR ":\n\n%m");
		msgbox(NULL, TITLE, msg);
		fail();
	}
	while (fgets(buf, sizeof buf, f))
	{
		char path[PATH_MAX];
		char *n;
		mode_t mode;
		
		n = strchr(buf, '\n');
		if (n)
			*n = 0;
		if (!*buf || *buf == '#')
			continue;
		
		mode = strtoul(buf, &n, 8);
		while (*n == ' ')
			n++;
		
		strcpy(path, MOUNT "/");
		strcat(path, n);
		
		if (mkdir(path, mode) && errno != EEXIST)
		{
			sprintf(msg, "Unable to create directory %s", n);
			msgbox_perror(NULL, TITLE, msg, errno);
			fail();
		}
	}
	fclose(f);
}

static void cancel_click(struct gadget *g, int x, int y)
{
	if (msgbox_ask4(NULL, TITLE, "Do you really want to cancel the installation?\n\nThis will likely leave the system in a broken state.", MSGBOX_NO) != MSGBOX_YES)
		return;
	
	*(int *)g->p_data = 1;
}

void copy_files(const char *list, const char *title)
{
	struct gadget *prgbar;
	struct gadget *cncbtn;
	struct form *form;
	char buf[1024];
	char msg[256];
	char *n;
	FILE *f;
	int total = 0, cnt = 0;
	int do_cancel = 0;
	
	form   = form_load("/lib/forms/filecopy.frm");
	prgbar = gadget_find(form, "prgbar");
	cncbtn = gadget_find(form, "cncbtn");
	
	if (title)
		form_set_title(form, title);
	
	bargraph_set_labels(prgbar, "", "");
	
	button_on_click(cncbtn, cancel_click);
	cncbtn->p_data = &do_cancel;
	
	if (!list)
		list = _PATH_E_SS_COPY;
	
	f = fopen(list, "r");
	if (!f)
	{
		sprintf(msg, "Unable to open " _PATH_E_SS_COPY ":\n\n%m");
		msgbox(NULL, TITLE, msg);
		fail();
	}
	while (fgets(buf, sizeof buf, f))
	{
		
		n = strchr(buf, '\n');
		if (n)
			*n = 0;
		if (!*buf || *buf == '#')
			continue;
		
		total++;
	}
	bargraph_set_limit(prgbar, total);
	rewind(f);
	
	form_show(form);
	evt_idle();
	while (fgets(buf, sizeof buf, f) && !do_cancel)
	{
		char dpath[PATH_MAX];
		mode_t mode;
		
		n = strchr(buf, '\n');
		if (n)
			*n = 0;
		if (!*buf || *buf == '#')
			continue;
		
		mode = strtoul(buf, &n, 8);
		while (*n == ' ')
			n++;
		
		strcpy(dpath, MOUNT "/");
		strcat(dpath, n);
		copy_file(n, dpath, mode);
		bargraph_set_value(prgbar, ++cnt);
		evt_idle();
	}
	form_close(form);
	fclose(f);
	
	if (do_cancel)
	{
		msgbox(NULL, TITLE, "The operation was cancelled. The system will now halt.");
		shutdown();
	}
}

static void link_file(const char *oldname, const char *newname)
{
	struct stat ost, nst;
	char *msg;
	int se;
	
retry:
	if (!link(oldname, newname))
		return;
	se = errno;
	
	if (se == EEXIST)
	{
		if (stat(oldname, &ost))
		{
			msgbox_perror(NULL, TITLE, oldname, se);
			fail();
		}
		if (stat(newname, &nst))
		{
			msgbox_perror(NULL, TITLE, newname, se);
			fail();
		}
		if (ost.st_ino == nst.st_ino && ost.st_dev == nst.st_dev)
			return;
		
		if (unlink(newname))
		{
			if (asprintf(&msg, "Cannot unlink %s", newname) < 0)
				msgbox_perror(NULL, TITLE, "asprintf", errno);
			else
				msgbox_perror(NULL, TITLE, msg, se);
			free(msg);
			fail();
		}
		goto retry;
	}
	
	if (asprintf(&msg, "Cannot link %s -> %s", oldname, newname) < 0)
		msgbox_perror(NULL, TITLE, "asprintf", errno);
	else
		msgbox_perror(NULL, TITLE, msg, se);
	free(msg);
	fail();
}

void link_files(const char *list)
{
	char buf[1024];
	char msg[256];
	FILE *f;

	if (!list)
		list = _PATH_E_SS_LINK;

	f = fopen(list, "r");
	if (!f)
	{
		sprintf(msg, "Unable to open " _PATH_E_SS_LINK ":\n\n%m");
		msgbox(NULL, TITLE, msg);
		fail();
	}
	while (fgets(buf, sizeof buf, f))
	{
		char spath[PATH_MAX];
		char npath[PATH_MAX];
		char *s;
		char *n;

		n = strchr(buf, '\n');
		if (n)
			*n = 0;
		if (!*buf || *buf == '#')
			continue;

		s = buf;
		while (isspace(*s))
			s++;
		
		n = s;
		while (!isspace(*n))
		{
			if (!*n)
				goto badfmt;
			n++;
		}
		*n++ = 0;
		
		while (isspace(*n))
			n++;
		if (!*n)
			goto badfmt;
		
		strcpy(spath, MOUNT "/");
		strcat(spath, s);
		
		strcpy(npath, MOUNT "/");
		strcat(npath, n);
		
		link_file(spath, npath);
	}
	fclose(f);
	
	if (access(MOUNT "/etc/rc.local", 0))
		rename(MOUNT "/etc/rc.local.example", MOUNT "/etc/rc.local");
	return;
badfmt:
	msgbox(NULL, TITLE, _PATH_E_SS_LINK ": Bad file format");
	fail();
}

void create_acct(void)
{
	if (access(_PATH_E_PASSWD, 0))
	{
		mknod(_PATH_E_PASSWD,	S_IFREG | 0644, 0);
		mknod(_PATH_E_PASSWD_S,	S_IFREG | 0600, 0);
		mknod(_PATH_E_GROUP,	S_IFREG | 0644, 0);
		
		_newuser( "root", 0, 0);
		_setshell("root", _PATH_B_DEFSHELL);
		_sethome( "root", _PATH_H_ROOT);
		_setpass( "root", "");
		
		_newgroup("rootgrp", 0);
		_newgroup("users", 1000);
	}
}

static void init_font(const char *name, const char *pathname)
{
	int ftd;
	
	if (_boot_flags() & BOOT_VERBOSE)
	{
		_sysmesg("sysinstall: loading font ");
		_sysmesg(pathname);
		_sysmesg("\n");
	}
	
	if (win_load_font(&ftd, name, pathname))
	{
		perror(pathname);
		for (;;)
			pause();
	}
}

static void init_font_map(int hidpi)
{
	static const int map[] =
	{
		WIN_FONT_MONO_L,
		WIN_FONT_SYSTEM_L,
		WIN_FONT_MONO_L,
		WIN_FONT_SYSTEM_L,
		WIN_FONT_MONO_LN,
		WIN_FONT_MONO_LN,
	};
	
	if (hidpi)
	{
		win_set_font_map(map, sizeof map);
		win_set_dpi_class(WIN_DPI_HIGH);
	}
	else
	{
		win_set_font_map(map, 0);
		win_set_dpi_class(WIN_DPI_NORMAL);
	}
}

void init_desktop(void)
{
	init_font("mono",		_PATH_FONT_MONO);
	init_font("system",		_PATH_FONT_SYSTEM);
	init_font("mono-large",		_PATH_FONT_MONO_L);
	init_font("system-large",	_PATH_FONT_SYSTEM_L);
	init_font("mono-narrow",	_PATH_FONT_MONO_N);
	init_font("mono-large-narrow",	_PATH_FONT_MONO_LN);
	
	if (win_newdesktop("sysinstall"))
	{
		perror("win_newdesktop");
		for (;;)
			pause();
	}
	
	if (mkdir("/tmp/desktop", 022))
	{
		perror("/tmp/desktop");
		for (;;)
			pause();
	}
	putenv("DESKTOP_DIR=/tmp/desktop");
	
	init_font_map(DEFAULT_LARGE_UI);
}

void init_rd(void)
{
	blk_t size = RD_SIZE;
	pid_t pid;
	int st;
	int d;
	
	if (_mod_load("/lib/drv/rd.drv", NULL, 0) < 0)
	{
		perror("/lib/drv/rd.drv");
		for (;;)
			pause();
	}
	
	d = open("/dev/" RD, O_RDWR);
	if (d < 0)
	{
		perror("/dev/" RD ": open");
		for (;;)
			pause();
	}
	if (ioctl(d, RDIOCNEW, &size))
	{
		perror("/dev/" RD ": RDIOCNEW");
		for (;;)
			pause();
	}
	close(d);
	
	signal(SIGCHLD, SIG_DFL);
	pid = _newtaskl("/sbin/mkfs", "/sbin/mkfs", "-q", "/dev/" RD, RD_SIZE_STR, NULL);
	if (pid < 0)
	{
		perror("/sbin/mkfs");
		for (;;)
			pause();
	}
	while (wait(&st) != pid)
		evt_idle();
	if (st)
	{
		_sysmesg("/sbin/mkfs: nonzero status");
		for (;;)
			pause();
	}
	signal(SIGCHLD, sig_chld);
	
	if (_mount(RD_MOUNT, RD, "native", 0))
	{
		perror("_mount");
		for (;;)
			pause();
	}
}

void load_partitions(char *disk)
{
	int l = strlen(disk);
	struct bio_info bi;
	struct dirent *de;
	char msg[256];
	DIR *dir;
	int fd;

	dir = opendir(".");
	if (!dir)
	{
		sprintf(msg, "Unable to open /dev:\n\n%m");
		msgbox(NULL, TITLE, msg);
		fail();
	}
	while ((de = readdir(dir)))
	{
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;
		
		if (strncmp(de->d_name, disk, l) || !de->d_name[l] /* != '.' */)
			continue;
		
		fd = open(de->d_name, O_RDWR);
		if (!ioctl(fd, BIO_INFO, &bi) && bi.type == BIO_TYPE_HD_SECTION)
		{
			part[part_count].offset	 = bi.offset;
			part[part_count].size	 = bi.size;
			part[part_count].os_type = bi.os;
			strcpy(part[part_count].dev,  de->d_name);
			strcpy(part[part_count].disk, disk);
			sprintf(part[part_count].desc, "  %s (%ji MBytes)", de->d_name, (intmax_t)(bi.size / 2048));
			
			part_count++;
		}
		close(fd);
	}
	closedir(dir);
}

void load_disks(void)
{
	struct dirent *de;
	DIR *dir;
	int fd;
	char msg[256];
	struct bio_info bi;
	
	chdir("/dev");
	dir = opendir(".");
	if (!dir)
	{
		sprintf(msg, "Unable to open /dev:\n\n%m");
		msgbox(NULL, TITLE, msg);
		fail();
	}
	while ((de = readdir(dir)))
	{
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;
		
		fd = open(de->d_name, O_RDWR);
		if (!ioctl(fd, BIO_INFO, &bi) && bi.type == BIO_TYPE_HD)
		{
			part[part_count].offset	 = bi.offset;
			part[part_count].size	 = bi.size;
			part[part_count].os_type = bi.os;
			strcpy(part[part_count].dev, de->d_name);
			sprintf(part[part_count].desc, "%s (%ji MBytes)", de->d_name, (intmax_t)(bi.size / 2048));
			
			part_count++;
			load_partitions(de->d_name);
		}
		close(fd);
	}
	closedir(dir);
	chdir("/");
}

void find_devs(void)
{
	pid_t pid;
	int st;
	
	signal(SIGCHLD, SIG_DFL);
	signal(SIGEVT, sig_evt);
	pid = _newtaskl(_PATH_B_FIND_DEVS, _PATH_B_FIND_DEVS, "-ddesktop0", RD_MOUNT "/devices", (void *)NULL);
	if (pid < 0)
	{
		perror(_PATH_B_FIND_DEVS);
		for (;;)
			pause();
	}
	while (waitpid(pid, &st, 0) <= 0);
	if (st)
	{
		_sysmesg(_PATH_B_FIND_DEVS ": nonzero status\n");
		for (;;)
			pause();
	}
	signal(SIGCHLD, sig_chld);
	signal(SIGEVT, SIG_DFL);
}

void init_mods(void)
{
	if (_mod_load("/lib/drv/framebuf.sys", NULL, 0) < 0)
	{
		perror("/lib/drv/framebuf.sys");
		for (;;)
			pause();
	}
}

static void init_home(void)
{
	FILE *f;
	int st;
	
	signal(SIGCHLD, SIG_DFL);
	if (_newtaskl(_PATH_B_FILEMGR_SLAVE, _PATH_B_FILEMGR_SLAVE,
		"copy", "/root", "/tmp/home", (void *)NULL) < 0)
		perror(_PATH_B_FILEMGR_SLAVE);
	while (wait(&st) <= 1)
		evt_idle();
	signal(SIGCHLD, sig_chld);
	
	f = fopen("/tmp/home/.startup", "w");
	if (f == NULL)
	{
		msgbox_perror(NULL, "Error", "/tmp/home/.startup", errno);
		fail();
	}
	fputs("/bin/taskbar", f);
	fclose(f);
	
	putenv("HOME=/tmp/home");
}

static void edit_devs(void)
{
	void *clh;
	pid_t pid;
	int st;
	
	msgbox(NULL, TITLE, "In the following window you can investigate\n"
			    "detected devices and install device drivers\n"
			    "required to continue the installation.");
	
	clh = signal(SIGCHLD, SIG_DFL);
	pid = _newtaskl(_PATH_B_EDIT_DEVS, _PATH_B_EDIT_DEVS, "-i",
			RD_MOUNT "/devices", (void *)NULL);
	if (pid < 0)
	{
		msgbox_perror(NULL, TITLE, "Cannot run \"" _PATH_B_EDIT_DEVS "\"", errno);
		fail();
	}
	signal(SIGEVT, sig_evt);
	while (wait(&st) != pid);
	signal(SIGCHLD, clh);
	signal(SIGEVT, SIG_DFL);
}

static void delay(int t)
{
	time_t t0;
	
	time(&t0);
	while (t0 + t > time(NULL))
	{
		win_idle();
		sleep(1);
	}
}

static void hwclock(void)
{
	int st;
	
	signal(SIGCHLD, SIG_DFL);
	if (_newtaskl("/sbin/hwclock", "/sbin/hwclock", "r", (void *)NULL) < 0)
		perror("/sbin/hwclock");
	while (wait(&st) <= 1)
		evt_idle();
	signal(SIGCHLD, sig_chld);
}

static void fsck(const char *device)
{
	struct form *f;
	int st;
	
	f = form_load("/lib/forms/sysinstall.fsck.frm");
	form_show(f);
	evt_idle();
	
	signal(SIGCHLD, SIG_DFL);
	if (_newtaskl(_PATH_B_FSCK, _PATH_B_FSCK, "-qF", device, (void *)NULL) < 0)
		perror(_PATH_B_FSCK);
	while (wait(&st) <= 1)
		evt_idle();
	signal(SIGCHLD, sig_chld);
	
	form_close(f);
}

static void sig_shutdown(int nr)
{
	win_killdesktop(SIGKILL);
	usleep2(1000000, 0);
	sync();
	
	switch (nr)
	{
	case SIGUSR1:
		_shutdown(SHUTDOWN_HALT);
		break;
	case SIGUSR2:
		_shutdown(SHUTDOWN_POWER);
		break;
	case SIGINT:
		_shutdown(SHUTDOWN_REBOOT);
		break;
	}
}

void stage0(void)
{
	int copy_devices = 0;
	char msg[256];
	pid_t pid;
	int single = 0;
	int st;
	int fd;
	int i;
	
	signal(SIGUSR1, sig_shutdown);
	signal(SIGUSR2, sig_shutdown);
	signal(SIGINT, sig_shutdown);
	
	if (_boot_flags() & BOOT_TEXT)
		_panic("sysinstall: text mode is not supported");
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: setting up environment\n");
	setenv("SPEAKER", "/dev/pcspk", 1);
	setenv("HOME", "/tmp/home", 1);
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: initializing /tmp ram disk\n");
	init_rd();
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: initializing modules\n");
	init_mods();
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: initializing desktop\n");
	init_desktop();
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: creating windows\n");
	
	win_desktop_size(&desk_w, &desk_h);
	win_ws_getrect(&wsr);
	wc_load_cte();
	wp_load();
	
	win_creat(&backdrop_wd, 1, wsr.x, wsr.y, wsr.w, wsr.h, backdrop_proc, NULL);
	win_raise(backdrop_wd);
	win_on_setmode(on_setmode);
	win_on_resize(on_resize);
	
	win_setptr(backdrop_wd, WIN_PTR_BUSY);
	win_idle();
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: searching for devices\n");
	find_devs();
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: staring hwclock\n");
	hwclock();
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: preparing temporary home\n");
	init_home();
	
	if (_boot_flags() & BOOT_SINGLE)
		single = 1;
	
	if (!single && msgbox_ask(NULL, TITLE, WELCOME) == MSGBOX_NO)
		single = 1;
	win_idle();
	
	if (single)
	{
		int desktop = 0;
		
		if (msgbox_ask(NULL, TITLE, "Do you want to start the desktop environment?") == MSGBOX_YES)
		{
			struct win_rect wsr;
			
			desktop = 1;
			
			if (msgbox_ask4(NULL, TITLE, "Use the high DPI mode?", MSGBOX_NO) == MSGBOX_YES)
			{
				ec_setmode(1);
				win_ws_getrect(&wsr);
				
				if (wsr.w < 1024 || wsr.h < 768)
					msgbox(NULL, TITLE, "The high DPI mode cannot be used with the current display resolution.");
				else
					init_font_map(1);
			}
			else
				ec_setmode(0);
		}
		
		putenv("OSDEMO=1");
		
		for (;;)
		{
			win_killdesktop(SIGKILL);
			usleep2(1000000, 0);
			
			if (desktop)
				pid = _newtaskl(_PATH_B_IPROFILE, _PATH_B_IPROFILE, (void *)NULL);
			else
				pid = _newtaskl(_PATH_B_VTTY, _PATH_B_VTTY, "-c", "60", (void *)NULL);
			
			for (;;)
			{
				if (waitpid(pid, &st, WNOHANG))
					break;
				win_wait();
			}
		}
	}
	
	eyecandy();
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: starting processes\n");
#if SYSINST_DEBUG
	_newtaskl(_PATH_B_IPROFILE,	_PATH_B_IPROFILE, (void *)NULL);
	_newtaskl(_PATH_B_VTTY,		_PATH_B_VTTY, "-c", "60", (void *)NULL);
#endif
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: done initializing\n");
	
	delay(1);
	win_setptr(backdrop_wd, WIN_PTR_DEFAULT);
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: device editor\n");
	edit_devs();
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: loading disk information\n");
	load_disks();
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: partition selection\n");
	partition();
	
	if (msgbox_ask(NULL, TITLE, "This will install the system on selected partition.\n\n"
				    "Do you wish to continue?") != MSGBOX_YES)
		shutdown();
	win_idle();
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: checking target fs\n");
	fsck(root_dev);
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: mounting target\n");
	if (_mount(MOUNT, root_dev, "native", 0))
	{
		sprintf(msg, "Cannot mount root device:\n\n%m\n\n"
			     "Please make sure the device is connected properly\n"
			     "and contains a valid filesystem.");
		msgbox(NULL, TITLE, msg);
		fail();
	}
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: creating directories\n");
	create_dirs(NULL);
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: copying files\n");
	copy_files(NULL, NULL);
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: installing SDK\n");
	create_dirs("/etc/sdk.mkdir");
	copy_files("/etc/sdk.copy", "Installing SDK...");
	link_files("/etc/sdk.link");
	
	if (access(MOUNT "/etc/devices", 0))
		copy_devices = 1;
	
	if (!copy_devices && msgbox_ask(NULL, TITLE, "Overwrite device database on the target filesystem?") == MSGBOX_YES)
		copy_devices = 1;
	
	if (copy_devices)
		copy_file(RD_MOUNT "/devices", MOUNT "/etc/devices", 0600);
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: creating file links\n");
	link_files(NULL);
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: installing boot records\n");
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: updating /etc/modules\n");
	update_mods();
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: generating /etc/boot.conf\n");
	bootgen();
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: installing PBR\n");
	install_boot();
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: installing MBR\n");
	install_mbr();
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: done installing boot records\n");
	
	unlink(MOUNT "/etc/sysinstall");
	fd = x_open(MOUNT "/etc/sysinstall", O_WRONLY|O_CREAT|O_TRUNC, 0600);
	x_write(fd, "/sbin/sysinstall stage1", 23);
	x_close(fd);
	
	if (access(MOUNT "/etc/desktops/desktop0/autologin", 0) &&
	    msgbox_ask(NULL, TITLE, "Autologin root on the main desktop?") == MSGBOX_YES)
	{
		fd = x_open(MOUNT "/etc/desktops/desktop0/autologin", O_WRONLY | O_CREAT | O_TRUNC, 0600);
		x_write(fd, "root", 4);
		x_close(fd);
	}
	
	for (i = 0 ; i < OPEN_MAX; i++)
		close(i);
	
	_umount("/mnt");
	_umount(_PATH_M_PTY);
	_umount(_PATH_M_DEV);
	_umount("");
	sync();
	
	msgbox(NULL, TITLE, "The system is now installed and ready to run.\n\n"
			    "Click OK to reboot.");
	for (;;)
		_shutdown(SHUTDOWN_REBOOT);
}

void stage1(void)
{
	int i;
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: creating accounts\n");
	create_acct();
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: unmounting filesystems\n");
	
	for (i = 0; i < OPEN_MAX; i++)
		close(i);
	
	_umount(_PATH_M_PTY);
	_umount(_PATH_M_DEV);
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: restarting init\n");
	if (unlink("/etc/sysinstall"))
	{
		perror("/etc/sysinstall: unlink");
		for (;;)
			pause();
	}
	execl("/sbin/init", "/sbin/init", NULL);
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: could not restart init\n");
	
	sync();
	for (;;)
		pause();
}

/*
 * sig_chld removes zombie processes from process table
 * such handling is necessary when running as init
 *
 */

void sig_chld(int nr)
{
	int status;
	pid_t pid;
	
	while (pid = waitpid(-1, &status, WNOHANG), pid > 0)
		;
	signal(SIGCHLD, sig_chld);
}

int main(int argc, char **argv)
{
	int i;
	
	if (_boot_flags() & BOOT_VERBOSE)
		_sysmesg("sysinstall: booting...\n");
	for (i = 1; i <= NSIG; i++)
		if (i != SIGKILL && i != SIGTERM)
			signal(i, SIG_DFL);
	signal(SIGCHLD, sig_chld);
	for (i = 0; i < OPEN_MAX; i++)
		close(i);
	open("/dev/console", O_RDONLY);
	open("/dev/console", O_WRONLY);
	open("/dev/console", O_WRONLY);
	umask(0);
	
	if (argc == 2)
	{
		if (!strcmp(argv[1], "stage0"))
		{
			if (_boot_flags() & BOOT_VERBOSE)
				_sysmesg("sysinstall: stage0\n");
			stage0();
		}
		
		if (!strcmp(argv[1], "stage1"))
		{
			if (_boot_flags() & BOOT_VERBOSE)
				_sysmesg("sysinstall: stage1\n");
			stage1();
		}
	}
	
	if (!win_attach())
		msgbox(NULL, TITLE, "This program should not be called directly.");
	errx(255, "this program should not be called directly");
	return 255;
}
