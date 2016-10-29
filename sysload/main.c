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

#include <priv/copyright.h>

#include <sysload/i386-pc.h>
#include <sysload/console.h>
#include <sysload/kparam.h>
#include <sysload/flags.h>
#include <sysload/disk.h>
#include <sysload/itoa.h>
#include <sysload/atoi.h>
#include <sysload/main.h>
#include <sysload/mem.h>
#include <sysload/hw.h>
#include <sysload/fs.h>

#include <machine/utsname.h>
#include <machine/machdef.h>
#include <devices.h>
#include <string.h>

struct boot_params boot_params =
{
	.image	    = "/lib/sys/kern.bin",
	.dev_db	    = "",
	.mod_list   = "",
	.boot_dev   = "",
	.boot_fs    = "boot",
	.root_dev   = "ramdisk",
	.root_fs    = "boot",
	.root_flags = 0,
	.boot_flags = 0,
	.rd_size    = 2880,
	.delay	    = 5,
};

int bootfb_xres = -1;
int bootfb_yres = -1;
int bootfb_bpp  = -1;

static int single_flag;
static int text_flag;
static int hdet_flag;
static int verbose_flag;

#define T_INT	0
#define T_FLAG	(-1)

static struct field
{
	char	key;
	int	x, y, w;
	int	size;
	void *	buf;
	int	group;
} fields[] =
{
	{ 'i', 21,  5, 55, sizeof boot_params.image,	 boot_params.image	  },
	{ 'd', 21,  7, 55, sizeof boot_params.dev_db,	 boot_params.dev_db	  },
	{ 'm', 21,  9, 55, sizeof boot_params.mod_list,	 boot_params.mod_list	  },
	{ 'b', 21, 11, 16, sizeof boot_params.boot_dev,	 boot_params.boot_dev	  },
	{ 's', 53, 11, 16, sizeof boot_params.boot_fs,	 boot_params.boot_fs	  },
	{ 'r', 21, 13, 16, sizeof boot_params.root_dev,	 boot_params.root_dev	  },
	{ 'f', 53, 13, 16, sizeof boot_params.root_fs,	 boot_params.root_fs	  },
	{ 'k', 21, 15, 16, T_INT,			&boot_params.rd_size	  },
	{ 'u',  5, 17, 1,  T_FLAG,			&single_flag,		1 },
	{ 't', 26, 17, 1,  T_FLAG,			&text_flag,		1 },
	{ 'h', 40, 17, 1,  T_FLAG,			&hdet_flag,		1 },
	{ 'v', 60, 17, 1,  T_FLAG,			&verbose_flag,		  },
};

static struct disk *	dk;
static struct fs	fs;

static void *		rd_base;
static uint32_t		rd_size;

static struct kmodimg *	mod_list;
static void *		kern_base;

static int		need_conf;

static unsigned		splash_size;
static uint8_t *	splash;

#define min(a, b)	((a) < (b) ? (a) : (b))

static void fbconf(void);

void diskview(void);

void fail(const char *s, const char *s1)
{
	con_status(s, s1);
	halt();
}

static void copyright(void)
{
	con_gotoxy(0, 0);
	con_puts("Operating System Loader v1\n"
		 "Copyright (C) Piotr Durlej\n");
	
	con_gotoxy(81 - sizeof UTSNAME, 0);
	con_puts(UTSNAME);
}

static int bootmsg(void)
{
	struct file f;
	char *msg;
	int c;
	int i;
	
	if (fs_lookup(&f, &fs, "/etc/bootmsg"))
		return 1;
	
	msg = mem_alloc(f.size, MA_SYSLOAD);
	if (msg == NULL)
		fail("Memory allocation failed.", "");
	
	if (fs_load(&f, msg))
		fail("Cannot load /etc/bootmsg", "");
	
	con_setattr(0, 7, BOLD);
	con_gotoxy(0, 0);
	con_clear();
	for (i = 0; i < f.size; i++)
		con_putc(msg[i]);
	
	con_status("Press RETURN to continue or press ESC to cancel.", "");
	while (c = con_getch(), c != '\n' && c != 27)
		;
	return c == '\n';
}

static void draw_field(struct field *f)
{
	int w = f->w;
	
	if (f->size)
		w = min(f->w, f->size);
	
	switch (f->size)
	{
	case T_INT:
		con_setattr(0, 7, INVERSE);
		con_rect(f->x, f->y, w, 1);
		con_gotoxy(f->x, f->y);
		con_putn(*(int *)f->buf);
		break;
	case T_FLAG:
		con_setattr(1, 7, 0);
		
		if (*(int *)f->buf)
			con_putxy(f->x, f->y, 'X');
		else
			con_putxy(f->x, f->y, ' ');
		break;
	default:
		con_setattr(0, 7, INVERSE);
		con_rect(f->x, f->y, w, 1);
		con_gotoxy(f->x, f->y);
		con_puts(f->buf);
	}
}

static void print_params(void)
{
	int i;
	
	con_setattr(1, 7, BOLD);
	con_rect(2, 3, 76, 20);
	con_frame(2, 3, 76, 20);
	
	con_setattr(1, 7, 0);
	con_putsxy(4, 5,	"Image path:");
	con_putsxy(4, 7,	"Device database:");
	con_putsxy(4, 9,	"Module list:");
	con_putsxy(4, 11,	"Boot device:");
	con_putsxy(41, 11,	"FS type:");
	con_putsxy(4, 13,	"Root device:");
	con_putsxy(41, 13,	"FS type:");
	con_putsxy(4, 15,	"Ramdisk size:");
	
	con_putsxy(4, 17,	"[ ] Single user mode [ ] Text mode [ ] Detect hardware [ ] Verbose");
	
	con_setattr(1, 7, BOLD);
	con_putxy(4, 5,		'I');
	con_putxy(4, 7,		'D');
	con_putxy(4, 9,		'M');
	con_putxy(4, 11,	'B');
	con_putxy(42, 11,	'S');
	con_putxy(4, 13,	'R');
	con_putxy(41, 13,	'F');
	con_putxy(10, 15,	'k');
	
	con_putxy(15, 17,	'u');
	con_putxy(29, 17,	'T');
	con_putxy(50, 17,	'h');
	con_putxy(63, 17,	'V');
	
	for (i = 0; i < sizeof fields / sizeof *fields; i++)
		draw_field(&fields[i]);
	
	con_setattr(0, 7, INVERSE);
	con_rect(0, 24, 80, 1);
	con_putsxy(0, 24, "Memory size: ");
	
	if (mem_size / 1024000)
	{
		con_putn(mem_size / 1048576);
		con_putc('M');
	}
	else
	{
		con_putn(mem_size / 1024);
		con_putc('K');
	}
}

static int edit_buf(int x, int y, char *buf, size_t max, int clear)
{
	int pos;
	int ch;
	
	pos = strlen(buf);
	
	con_setattr(0, 7, INVERSE);
	con_gotoxy(x + pos, y);
	for (;;)
	{
		ch = con_getch();
		switch (ch)
		{
		case '\n':
			return 1;
		case 27:
			return 0;
		case '\b':
			if (pos)
			{
				con_putxy(x + --pos, y, ' ');
				buf[pos] = 0;
				con_gotoxy(x + pos, y);
			}
			break;
		case 'U' - 0x40:
			con_rect(x, y, max, 1);
			con_gotoxy(x, y);
			buf[0] = 0;
			pos = 0;
			break;
		default:
			if (clear)
			{
				con_rect(x, y, max, 1);
				con_gotoxy(x, y);
				buf[0] = 0;
				pos = 0;
			}
			
			if (pos >= max - 1)
				break;
			if (ch < 0x20 || ch >= 0x7e)
				break;
			buf[pos++] = ch;
			buf[pos] = 0;
			con_putc(ch);
		}
		clear = 0;
	}
}

static void edit_num_field(struct field *f)
{
	char buf[33]; /* XXX */
	char *p;
	int v;
	
	itoa(*(int *)f->buf, buf);
retry:
	edit_buf(f->x, f->y, buf, min(f->w, sizeof buf), 0);
	for (v = 0, p = buf; *p; p++)
	{
		if (*p < '0' || *p > '9')
			goto retry;
		v *= 10;
		v += *p - '0';
	}
	*(int *)f->buf = v;
	
	itoa(v, buf);
	con_putsxy(f->x, f->y, buf);
}

static void edit_field(struct field *f)
{
	int i;
	
	switch (f->size)
	{
	case T_INT:
		edit_num_field(f);
		break;
	case T_FLAG:
		con_setattr(1, 7, 0);
		
		if (f->group)
			for (i = 0; i < sizeof fields / sizeof *fields; i++)
			{
				if (fields[i].group != f->group)
					continue;
				if (fields[i].size != T_FLAG)
					continue;
				if (&fields[i] == f)
					continue;
				
				con_putxy(fields[i].x, fields[i].y, ' ');
				*(int *)fields[i].buf = 0;
			}
		
		if ((*(int *)f->buf = !*(int *)f->buf))
			con_putxy(f->x, f->y, 'X');
		else
			con_putxy(f->x, f->y, ' ');
		break;
	default:
		edit_buf(f->x, f->y, f->buf, min(f->w, f->size), 0);
	}
}

static void setmode(void)
{
	int x, y, b;
	char xres[7];
	char yres[7];
	char bpp[3];
	
	fbconf();
	
	x = bootfb_xres > 0 ? bootfb_xres : 640;
	y = bootfb_yres > 0 ? bootfb_yres : 480;
	b = bootfb_bpp  > 0 ? bootfb_bpp  : 8;
	
	itoa(x, xres);
	itoa(y, yres);
	itoa(b, bpp);
	
	con_setattr(0, 7, 0);
	con_rect(25, 9, 28, 8);
	
	con_setattr(4, 7, BOLD);
	con_rect(24, 8, 28, 8);
	con_frame(24, 8, 28, 8);
	con_putsxy(30, 8, " Display mode ");
	
	con_putsxy(27, 10, "Width");
	con_putsxy(36, 10, "Height");
	con_putsxy(45, 10, "BPP");
	
	con_setattr(0, 7, INVERSE);
	con_rect(27, 11, 7, 1);
	con_rect(36, 11, 7, 1);
	con_rect(45, 11, 3, 1);
	
	con_putsxy(27, 11, xres);
	con_putsxy(36, 11, yres);
	con_putsxy(45, 11, bpp);
	
retry:
	con_setattr(4, 7, 0);
	con_putsxy(27, 13, "OK? ");
	
	con_status("Press RETURN for next field or press ESC to cancel.", "");
	
	if (!edit_buf(27, 11, xres, sizeof xres, 1))
		return;
	
	if (!edit_buf(36, 11, yres, sizeof yres, 1))
		return;
	
	if (!edit_buf(45, 11, bpp,  sizeof bpp, 1))
		return;
	
	con_status("Press RETURN to confirm or press ESC to cancel.", "");
	
	con_setattr(4, 7, BOLD);
	con_putsxy(27, 13, "OK? ");
	
	for (;;)
		switch (con_getch())
		{
		case '\n':
		case 'y':
			bootfb_xres = atoi(xres);
			bootfb_yres = atoi(yres);
			bootfb_bpp  = atoi(bpp);
			return;
		case 27:
		case 'n':
			goto retry;
		default:
			;
		}
}

static int edit_params(void)
{
	int ch;
	int i;
	
	single_flag	= 0;
	text_flag	= 0;
	hdet_flag	= 0;
	verbose_flag	= 0;
	
	if (boot_params.boot_flags & BOOT_SINGLE)
		single_flag = 1;
	if (boot_params.boot_flags & BOOT_TEXT)
		text_flag = 1;
	if (boot_params.boot_flags & BOOT_DETECT)
		hdet_flag = 1;
	if (boot_params.boot_flags & BOOT_VERBOSE)
		verbose_flag = 1;
	
redraw:
	con_setattr(1, 7, BOLD);
	con_clear();
	copyright();
	
	print_params();
	for (;;)
	{
		con_setattr(1, 7, 0);
		con_putsxy(4, 20, "Press RETURN to continue booting or "
				  "press ESC to abort.");
		con_setattr(1, 7, BOLD);
		con_putsxy(10, 20, "RETURN");
		con_putsxy(46, 20, "ESC");
		con_gotoxy(75, 20);
		
		ch = con_getch();
		switch (ch)
		{
		case '!':
			diskview();
			goto redraw;
		case '@':
			setmode();
			goto redraw;
		case 'L' - 0x40:
			goto redraw;
		case '\n':
		case 27:
			goto fini;
		default:
			;
		}
		for (i = 0; i < sizeof fields / sizeof *fields; i++)
			if (fields[i].key == ch)
			{
				con_rect(4, 20, 60, 1);
				edit_field(&fields[i]);
			}
	}
fini:
	boot_params.boot_flags = 0;
	
	if (single_flag)
		boot_params.boot_flags |= BOOT_SINGLE;
	if (text_flag)
		boot_params.boot_flags |= BOOT_TEXT;
	if (hdet_flag)
		boot_params.boot_flags |= BOOT_DETECT;
	if (verbose_flag)
		boot_params.boot_flags |= BOOT_VERBOSE;
	
	con_setattr(1, 7, 0);
	con_rect(2, 3, 76, 20);
	return ch == '\n';
}

static void mount_fs(void)
{
	struct fstype *fst;
	
	dk = disk_find(boot_params.boot_dev);
	if (!dk)
		fail("Incorrect boot device ", "");
	
	fst = fs_find(boot_params.boot_fs);
	if (!fst)
		fail("Incorrect boot file system type", "");
	
	if (fs_mount(&fs, fst, dk))
		fail("Unable to mount boot file system", "");
}

static void load_kern(void)
{
	struct khead kh;
	struct file f;
	int err;
	
	con_status("Loading ", boot_params.image);
	if (err = fs_lookup(&f, &fs, boot_params.image), err)
		fail("Unable to open ", boot_params.image);
	
	if (fs_load3(&f, &kh, sizeof kh))
		fail("Unable to load ", boot_params.image);
	
	if (kh.size < f.size)
		fail("Invalid kernel image ", boot_params.image);
	
	kern_base = mem_alloc(kh.size, MA_KERN);
	if (fs_load(&f, kern_base))
		fail("Unable to load ", boot_params.image);
	
	con_status("", "");
}

static void load_dev(struct device *dev)
{
	struct kmodimg *desc;
	struct kmodimg *last;
	struct file f;
	char *image;
	char *args;
	const char *p, *p1;
	size_t len;
	
	con_status("Loading ", dev->driver);
	
	if (fs_lookup(&f, &fs, dev->driver))
		fail("Unable to open ", dev->driver);
	
	image = mem_alloc(f.size, MA_KERN);
	args  = mem_alloc(sizeof *dev, MA_KERN);
	desc  = mem_alloc(sizeof *desc, MA_KERN);
	
	memset(desc, 0, sizeof *desc);
	
	if (fs_load(&f, image))
		fail("Unable to load ", dev->driver);
	memcpy(args, dev, sizeof *dev);
	desc->next	= NULL;
	desc->image	= image;
	desc->args	= args;
	desc->args_size	= sizeof *dev;
	
	p1 = p;
	for (p = dev->driver; *p; p++)
		if (*p == '/')
			p1 = p + 1;
	
	len = strlen(p1);
	if (len >= sizeof desc->name)
		len = sizeof desc->name - 1;
	
	memcpy(desc->name, p1, len);
	
	if (mod_list == NULL)
		mod_list = desc;
	else
	{
		last = mod_list;
		while (last->next != NULL)
			last = last->next;
		last->next = desc;
	}
	
	con_status("", "");
}

static void load_devs(void)
{
	struct device *devs;
	struct file f;
	int lcnt, bcnt;
	int i, cnt;
	
	if (!*boot_params.dev_db)
		return;
	
	con_status("Loading ", boot_params.dev_db);
	
	if (fs_lookup(&f, &fs, boot_params.dev_db))
		fail("Unable to open ", boot_params.dev_db);
	if (!f.size)
		return;
	if (f.size % sizeof *devs)
		fail("Invalid device database ", boot_params.dev_db);
	cnt = f.size / sizeof *devs;
	
	devs = mem_alloc(f.size, MA_SYSLOAD);
	if (fs_load(&f, devs))
		fail("Unable to load ", boot_params.dev_db);
	
	con_status("", "");
	
	for (bcnt = 0, i = 0; i < cnt; i++)
		if (!*devs[i].desktop_name)
			bcnt++;
	
	for (lcnt = 0, i = 0; i < cnt; i++)
		if (!*devs[i].desktop_name)
		{
			load_dev(&devs[i]);
			if (!boot_params.rd_size)
			{
				con_setattr(0, 7, BOLD);
				con_progress(2, 22, 76, ++lcnt + 1, bcnt + 1);
			}
		}
	if (!boot_params.rd_size)
	{
		con_setattr(0, 7, BOLD);
		con_progress(2, 22, 76, 1, 1);
	}
	memset(devs, 0x55, f.size);
}

static void load_rd(void)
{
	blk_t i;
	char *p;
	
	if (!boot_params.rd_size)
		return;
	
	rd_base = mem_alloc(boot_params.rd_size * 512, MA_SYSTEM);
	rd_size = boot_params.rd_size;
	if (!rd_base)
		fail("Ramdisk too big to fit in memory.", "");
	
	con_status("Loading ramdisk ...", "");
	for (p = rd_base, i = 0; i < rd_size; p += 512, i++)
	{
		if (i % 36 == 35)
		{
			con_setattr(0, 7, BOLD);
			con_progress(2, 22, 76, i, rd_size);
		}
		if (disk_read(dk, i, p))
			fail("Unable to load ramdisk.", "");
	}
	con_setattr(0, 7, BOLD);
	con_progress(2, 22, 76, 1, 1);
	con_status("Ramdisk loaded.", "");
}

static void load_splash(void)
{
	const char *pathname = "/lib/splash/splash.pnm";
	struct file f;
	void *buf;
	
	if (boot_params.boot_flags)
		pathname = "/lib/splash/splash_mnt.pnm";
	
	if (fs_lookup(&f, &fs, pathname))
		return;
	if (!f.size)
		return;
	
	buf = mem_alloc(f.size, MA_SYSLOAD);
	
	if (fs_load(&f, buf))
		return;
	
	splash_size = f.size;
	splash	    = buf;
}

static void show_splash(struct kparam *kp)
{
	struct kfb *fb;
	uint8_t *buf, *end;
	uint8_t *p;
	uint32_t *pp;
	uint32_t c;
	int x0, y0;
	int x, y;
	int w, h;
	
	for (fb = kp->fb; fb; fb = fb->next)
	{
		if (fb->type != KFB_GRAPH)
			continue;
		
		if (fb->bpp != 32)
			continue;
		
		break;
	}
	
	buf = splash;
	end = buf + splash_size;
	
	if (memcmp(buf, "P6\n", 3))
		return;
	p = buf + 3;
	
	if (*p == '#')
	{
		while (p < end && *p != '\n')
			p++;
		if (*p++ != '\n')
			return;
	}
	
	w = atoi(p);
	while (p < end && *p >= '0' && *p < '9')
		p++;
	while (p < end && *p == ' ')
		p++;
	
	h = atoi(p);
	while (p < end && *p >= '0' && *p < '9')
		p++;
	if (*p++ != '\n')
		return;
	
	while (p < end && *p != '\n')
		p++;
	if (*p++ != '\n')
		return;
	
	c  = (uint32_t)p[0] << 16;
	c |= (uint32_t)p[1] << 8;
	c |= (uint32_t)p[2];
	
	for (y = 0; y < fb->yres; y++)
	{
		pp = (char *)fb->base + fb->bytes_per_line * y;
		
		for (x = 0; x < fb->xres; x++)
			*pp++ = c;
	}
	
	if (w > fb->xres || h > fb->yres)
		return;
	
	x0 = (fb->xres - w) / 2;
	y0 = (fb->yres - h) / 2;
	for (y = 0; y < h; y++)
	{
		pp = (char *)fb->base + fb->bytes_per_line * (y0 + y) + x0 * 4;
		
		for (x = 0; x < w; x++)
		{
			c  = *p++ << 16;
			c |= *p++ << 8;
			c |= *p++;
			
			*pp++ = c;
		}
	}
}

static void run_kern(void)
{
	static struct kparam kp;
	
	void (*start)(struct kparam *kp) = kern_base;
	
	con_gotoxy(0, 0);
	con_update();
	
	memset(&kp, 0, sizeof kp);
	kp.mod_list	 = mod_list;
	kp.mem_map	 = mem_map;
	kp.mem_cnt	 = mem_cnt;
	strcpy(kp.root_dev, boot_params.root_dev);
	strcpy(kp.root_type, boot_params.root_fs);
	kp.root_flags	 = boot_params.root_flags;
	kp.boot_flags	 = boot_params.boot_flags;
	kp.rd_base	 = (uintptr_t)rd_base;
	kp.rd_blocks	 = rd_size;
	mach_fill_kparam(&kp);
	show_splash(&kp);
	start(&kp);
}

static void mem_sortmap(void)
{
	struct kmapent tmp;
	int sorted;
	int i;
	
	do
	{
		sorted = 1;
		
		for (i = 0; i < mem_cnt - 1; i++)
		{
			if (mem_map[i].base <= mem_map[i + 1].base)
				continue;
			
			tmp = mem_map[i + 1];
			
			mem_map[i + 1] = mem_map[i];
			mem_map[i    ] = tmp;
			
			sorted = 0;
		}
	} while (!sorted);
}

static void load(void)
{
	con_setattr(0, 7, BOLD);
	con_clear();
	copyright();
	con_putsxy(2, 20, "Loading operating system ...");
	con_progress(2, 22, 76, 0, 1);
	con_status("Please wait ...", "");
	con_gotoxy(78, 24);
	con_update();
	
	mount_fs();
	load_splash();
	load_kern();
	load_devs();
	load_rd();
	
	con_setattr(0, 7, BOLD);
	con_rect(2, 20, 76, 1);
	con_progress(2, 22, 76, 1, 1);
	con_status("Booting the kernel ...", "");
	mem_adjmap();
	mem_sortmap();
	con_fini();
	hw_fini();
	run_kern();
	
	con_setattr(0, 7, BOLD);
	con_rect(2, 20, 76, 1);
	con_putsxy(2, 20, "Operating system startup failed. ");
}

static void autoconf_fs(void)
{
	union { char buf[512]; struct nat_super sb; } u;
	int err;
	
	err = disk_read(disk_boot, 1, u.buf);
	if (err)
	{
		need_conf = 1;
		return;
	}
	if (!memcmp(u.sb.magic, NAT_MAGIC, sizeof u.sb.magic))
	{
		strcpy(boot_params.boot_fs, "native");
		return;
	}
	strcpy(boot_params.boot_fs, "boot");
	return;
}

static void autoconf(void)
{
	struct fstype *fst;
	struct disk *dk;
	struct file f;
	char *p, *p1;
	char *cf;
	
	if (*bdev)
		disk_boot = disk_find(bdev);
	
	if (!disk_boot)
	{
		need_conf = 1;
		return;
	}
	
	strcpy(boot_params.boot_dev, disk_boot->name);
	autoconf_fs();
	
	con_status("Loading configuration ...", "");
	
	dk = disk_find(boot_params.boot_dev);
	if (!dk)
		goto bad_conf;
	fst = fs_find(boot_params.boot_fs);
	if (!fst)
		goto bad_conf;
	if (fs_mount(&fs, fst, dk))
		goto bad_conf;
	boot_params.rd_size = dk->size;
	
	if (fs_lookup(&f, &fs, "/etc/boot.conf"))
		return;
	strcpy(boot_params.dev_db,	"/etc/devices");
	strcpy(boot_params.mod_list,	"/etc/boot_modules");
	boot_params.rd_size = 0;
	con_gotoxy(0, 8);
	
	cf = mem_alloc(f.size + 1, MA_SYSLOAD);
	cf[f.size] = 0;
	if (fs_load(&f, cf))
		goto bad_conf;
	
	for (p = cf; ; p = p1 + 1)
	{
		while (*p == '\n')
			p++;
		if (!*p)
			break;
		
		p1 = strchr(p, '\n');
		if (!p1)
			p1 = cf + f.size;
		else
			*p1 = 0;
		
		if (!strncmp(p, "bflags=", 7))
		{
			boot_params.boot_flags = atoi(p + 7);
			continue;
		}
		
		if (!strncmp(p, "rflags=", 7))
		{
			boot_params.root_flags = atoi(p + 7);
			continue;
		}
		
		if (!strncmp(p, "delay=", 6))
		{
			boot_params.delay = atoi(p + 6);
			continue;
		}
		
		if (!strncmp(p, "rtype=", 6))
		{
			if (p1 - p - 6 >= sizeof boot_params.root_fs)
				goto bad_conf;
			strcpy(boot_params.root_fs, p + 6);
			continue;
		}
		
		if (!strncmp(p, "rdev=", 5))
		{
			if (p1 - p - 5 >= sizeof boot_params.root_dev)
				goto bad_conf;
			strcpy(boot_params.root_dev, p + 5);
			continue;
		}
		
		if (!strncmp(p, "kern=", 5))
		{
			if (p1 - p - 5 >= sizeof boot_params.image)
				goto bad_conf;
			strcpy(boot_params.image, p + 5);
			continue;
		}
		
		if (!strncmp(p, "bdev=", 5))
			continue;
		
		goto bad_conf;
	}
	con_status("", "");
	return;
bad_conf:
	need_conf = 1;
}

static void fbconf(void)
{
	static int done;
	
	struct file f;
	char *p, *p1;
	char *cf;
	
	if (done)
		return;
	done = 1;
	
	if (boot_params.boot_flags & (BOOT_SINGLE | BOOT_TEXT))
		return;
	
	con_status("Loading display configuration ...", "");
	
	if (fs_lookup(&f, &fs, "/etc/bootfb.conf"))
		goto fini;
	
	cf = mem_alloc(f.size + 1, MA_SYSLOAD);
	cf[f.size] = 0;
	if (fs_load(&f, cf))
		goto fini;
	
	for (p = cf; ; p = p1 + 1)
	{
		while (*p == '\n')
			p++;
		if (!*p)
			break;
		
		p1 = strchr(p, '\n');
		if (!p1)
			p1 = cf + f.size;
		else
			*p1 = 0;
		
		switch (*p)
		{
		case 'W':
			bootfb_xres = atoi(p + 1);
			break;
		case 'H':
			bootfb_yres = atoi(p + 1);
			break;
		case 'B':
			bootfb_bpp = atoi(p + 1);
			break;
		}
	}
	
fini:
	con_status("", "");
}

static void dump(uint8_t *p, size_t sz, size_t off)
{
	int i, n;
	
	for (i = 0; i < sz; i += 16)
	{
		int o = off + i;
		
		con_setattr(0, 7, BOLD);
		con_putc(' ');
		con_putx4(o >> 8);
		con_putx8(o);
		con_puts(": ");
		
		for (n = 0; n < 16; n++)
		{
			if (n & 1)
				con_setattr(0, 5, BOLD);
			else
				con_setattr(0, 3, BOLD);
			con_putx8(p[i + n]);
			
			con_setattr(0, 7, 0);
			if (n == 7)
				con_putc('-');
			else
				con_putc(' ');
		}
		con_putc(' ');
		
		for (n = 0; n < 16; n++)
		{
			int c = p[i + n];
			
			if (c < 32 || c >= 128)
			{
				con_setattr(0, 7, BOLD);
				con_putc('.');
			}
			else
			{
				con_setattr(0, 7, 0);
				con_putc(c);
			}
		}
		con_putc('\n');
	}
}

void diskview(void)
{
	static char buf[512];
	struct disk *dk;
	blk_t blk = 0;
	int off = 0;
	
	con_setattr(0, 7, BOLD);
	con_clear();
	copyright();
	
	dk = disk_find(*bdev);
	if (dk == NULL)
	{
		dk = disk_list;
		if (dk == NULL)
			return;
	}
	for (;;)
	{
		if (blk >= dk->size)
			blk = dk->size - 1;
		
		con_setattr(0, 7, BOLD);
		con_gotoxy(0, 3);
		con_puts("Disk ");
		con_puts(dk->name);
		con_puts(", sector ");
		con_putn(blk);
		con_puts(", total ");
		con_putn(dk->size);
		con_puts(", offset ");
		con_putn(off);
		con_puts("     \n\n");
		con_setattr(0, 7, 0);
		memset(buf, 0xff, sizeof buf);
		disk_read(dk, blk, buf);
		dump(buf + off, 256, off);
		
		con_setattr(0, 7, 0);
		con_puts("\nPress ");
		con_setattr(0, 7, BOLD);
		con_puts("x");
		con_setattr(0, 7, 0);
		con_puts(" to exit viewer, ");
		con_setattr(0, 7, BOLD);
		con_puts("< > [ ]");
		con_setattr(0, 7, 0);
		con_puts(" to change sector, ");
		con_setattr(0, 7, BOLD);
		con_puts(", .");
		con_setattr(0, 7, 0);
		con_puts(" to scroll\nor ");
		con_setattr(0, 7, BOLD);
		con_puts("n");
		con_setattr(0, 7, 0);
		con_puts(" for next disk. ");
		
		switch (con_getch())
		{
		case '<':
			off = 0;
			if (blk)
				blk--;
			break;
		case '>':
			off = 0;
			blk++;
			break;
		case '[':
			off = 0;
			if (blk >= 100)
				blk -= 100;
			else
				blk = 0;
			break;
		case ']':
			off = 0;
			blk += 100;
			break;
		case ',':
			if (off)
				off -= 16;
			break;
		case '.':
			if (off < 256)
				off += 16;
			break;
		case 'n':
			dk = dk->next;
			if (dk == NULL)
				dk = disk_list;
			blk = 0;
			break;
		case 'x':
			return;
		default:
			;
		}
	}
}

static void showlogo(void)
{
	struct file f;
	char *buf;
	int i;
	
	if (fs_lookup(&f, &fs, "/lib/boot.pbm"))
		return;
	
	buf = mem_alloc(f.size, MA_SYSLOAD);
	if (buf == NULL)
		return;
	
	if (fs_load(&f, buf))
		return;
	
	i = 0;
	while (i < f.size && buf[i++] != '\n');
	if (buf[i] == '#')
		while (i < f.size && buf[i++] != '\n');
	while (i < f.size && buf[i++] != '\n');
	
	con_showlogo(buf + i, f.size - i, 640);
}

int main()
{
	long t, lt, et;
	
	con_init();
	con_setattr(0, 7, BOLD);
	con_clear();
	copyright();
	con_status("Memory ...", "");
	mem_init();
	
	if (mem_size < 15 * 1048576) // complain below 15 MiB
		fail(SYS_PRODUCT " requires at least 16 MiB of RAM memory.", "");
	
	con_status("Disks ...", "");
	disk_init();
	con_status("File systems ...", "");
	fs_init();
	con_status("Autoconfiguring ...", "");
	autoconf();
	con_status("", "");
	
	if (!need_conf && !bootmsg())
		return 0;
	con_setattr(0, 7, BOLD);
	con_clear();
	copyright();
	con_status("", "");
	
	if (!need_conf)
	{
		showlogo();
		
		con_status("Press ESC to edit boot parameters and options.", "");
		et = clock_time() + boot_params.delay * clock_hz();
		while (t = clock_time(), t <= et && !con_kbhit())
			;
		while (con_kbhit())
			if (con_getch() == 27)
				need_conf = 1;
	}
	con_hidelogo();
	
	if (!need_conf || edit_params())
	{
		fbconf();
		load();
	}
	return 0;
}
