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

#include <arch/archdef.h>
#include <kern/machine/machine.h>
#include <kern/arch/page.h>
#include <priv/libc_globals.h>
#include <priv/libc.h>
#include <priv/exec.h>
#include <priv/sys.h>
#include <priv/elf.h>
#include <sys/utsname.h>
#include <string.h>
#include <module.h>
#include <unistd.h>
#include <wingui.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <event.h>
#include <errno.h>
#include <os386.h>
#include <sysver.h>
#include <paths.h>
#include <libx.h>

#include "elo/elo.h"

#ifdef __ARCH_I386__
#define XMAGIC	"\0EXE"
#elif defined __ARCH_AMD64__
#define XMAGIC	"\0X64"
#else
#error Unknown arch
#endif

extern const struct
{
	char *name;
	void *addr;
} exports[];

typedef int main_t(int argc, char **argv, char **environ, void *libc_entry);

void *lgetsym(const char *name);

struct libxhdr *	libxh;
struct libx		libx = { .lentry = libc_entry, .lgetsym = lgetsym };

main_t *	p_main;
void *		p_environ;
void *		p_errno;
char *		image;
unsigned	v_end;	/* XXX */

void redraw_msgwin(struct event *e, int win_w, int win_h, const char *msg, int bdown)
{
	static const char *bt = "Terminate";
	
	win_color hi1, hi2, sh1, sh2;
	win_color button;
	win_color frame1;
	win_color frame2;
	win_color fg;
	win_color bg;
	int bx, by, bw, bh;
	int w, h;
	
	win_rgb2color(&fg,	  0,   0,   0);
	win_rgb2color(&bg,	255, 255, 255);
	win_rgb2color(&frame1,	  0,   0,   0);
	win_rgb2color(&frame2,	  0,   0, 128);
	win_rgb2color(&button,  203, 200, 192);
	
	win_rgb2color(&hi1, 255, 255, 255);
	win_rgb2color(&hi2, 224, 224, 224);
	win_rgb2color(&sh1,  64,  64,  64);
	win_rgb2color(&sh2, 128, 128, 128);
	
	if (e->win.type == WIN_E_REDRAW)
		win_clip(e->win.wd, e->win.redraw_x, e->win.redraw_y, e->win.redraw_w, e->win.redraw_h, 0, 0);
	else
		win_clip(e->win.wd, 0, 0, win_w, win_h, 0, 0);
	win_paint();
	
	win_rect(e->win.wd, frame1, 0, 0, win_w, win_h);
	win_rect(e->win.wd, frame2, 1, 1, win_w - 2, win_h - 2);
	win_rect(e->win.wd, bg, 7, 7, win_w - 14, win_h - 14);
	
	win_text_size(WIN_FONT_DEFAULT, &w, &h, __libc_progname);
	win_text(e->win.wd, fg, (win_w - w) / 2, win_h * 3 / 12, __libc_progname);
	
	win_text_size(WIN_FONT_DEFAULT, &w, &h, msg);
	win_text(e->win.wd, fg, (win_w - w) / 2, win_h * 5 / 12, msg);
	
	win_text_size(WIN_FONT_DEFAULT, &w, &h, bt);
	
	bw = w + h * 2;
	bh = h * 3;
	bx = (win_w - bw) >> 1;
	by =  win_h * 7 / 12;
	
	win_rect(e->win.wd, fg, bx - 1, by - 1, bw + 2, bh + 2);
	if (bdown)
		form_draw_rect3d(e->win.wd, bx, by, bw, bh, sh1, sh2, hi1, hi2, button);
	else
		form_draw_rect3d(e->win.wd, bx, by, bw, bh, hi1, hi2, sh1, sh2, button);
	
	win_btext(e->win.wd, button, fg, bx + h, by + h, bt);
	
	win_end_paint();
}

void redraw_appwin(struct event *e)
{
	win_color bg;
	
	win_rgb2color(&bg, 255, 255, 255);
	win_clip(e->win.wd, e->win.redraw_x, e->win.redraw_y, e->win.redraw_w, e->win.redraw_h, 0, 0);
	
	win_paint();
	win_rect(e->win.wd, bg, 0, 0, e->win.redraw_w, e->win.redraw_h);
	win_end_paint();
}

void fail(char *msg, int err) __attribute__((noreturn));

static void wfail(const char *msg)
{
	struct event e;
	int bdown = 0;
	int kdown = 0;
	int desk_w, desk_h;
	int win_w, win_h;
	int win_x, win_y;
	int exe_w, exe_h;
	int msg_w, msg_h;
	int wd;
	
	if (win_attach())
		return;
	
	if (win_desktop_size(&desk_w, &desk_h))
		return;
	
	if (desk_w < 2 || desk_h < 2)
		return;
	
	win_text_size(WIN_FONT_DEFAULT, &exe_w, &exe_h, __libc_progname);
	win_text_size(WIN_FONT_DEFAULT, &msg_w, &msg_h, msg);
	
	win_w = desk_w - 200;
	win_h = msg_h * 12;
	if (win_w < 300)
		win_w = 300;
	win_x = (desk_w - win_w) >> 1;
	win_y = (desk_h - win_h) >> 1;
	
	if (win_creat(&wd, 1, win_x, win_y, win_w, win_h, NULL, NULL))
		return;
	win_set_title(wd, __libc_progname);
	win_raise(wd);
	win_focus(wd);
	
	for (;;)
	{
		if (_evt_wait(&e) || e.type != E_WINGUI)
			continue;
		
		switch (e.win.type)
		{
		case WIN_E_KEY_DOWN:
			if (e.win.wd != wd)
				break;
			if (e.win.ch == ' ' || e.win.ch == '\n')
			{
				redraw_msgwin(&e, win_w, win_h, msg, bdown = 1);
				kdown = 1;
			}
			break;
		case WIN_E_KEY_UP:
			if (e.win.wd != wd || !kdown)
				break;
			if (e.win.ch == ' ' || e.win.ch == '\n')
				return;
			break;
		case WIN_E_PTR_DOWN:
			if (e.win.wd == wd)
				redraw_msgwin(&e, win_w, win_h, msg, bdown = 1);
			break;
		case WIN_E_PTR_UP:
			if (e.win.wd == wd || !bdown)
				return;
			break;
		case WIN_E_REDRAW:
			if (e.win.wd == wd)
				redraw_msgwin(&e, win_w, win_h, msg, bdown);
			else
				redraw_appwin(&e);
			break;
		}
	}
}

void fail(char *msg, int err)
{
	const char *errstr = NULL;
	
	_sysmesg("user.bin: ");
	_sysmesg(__libc_progname);
	_sysmesg(": ");
	_sysmesg(msg);
	_sysmesg("\n");
	
	if (err)
	{
		errstr = strerror(err);
		
		_sysmesg("user.bin: ");
		_sysmesg(__libc_progname);
		_sysmesg(": ");
		_sysmesg(errstr);
		_sysmesg("\n");
	}
	
	fprintf(_get_stderr(), "user.bin: %s: %s\n", __libc_progname, msg);
	if (err)
		fprintf(_get_stderr(), "user.bin: %s: %s\n", __libc_progname, strerror(err));
	
	wfail(msg);
	_exit(255);
}

void ck_exec(unsigned *p, size_t size)
{
	unsigned cksum = 0;
	
	size  += 3;
	size >>= 2;
	while (size--)
		cksum += *p++;
	
	if (cksum)
		fail("Invalid checksum", 0);
}

void load_elf(void)
{
	struct elo_data ed;
	struct stat st;
	
	fstat(__libc_progfd, &st);
	if (!S_ISREG(st.st_mode))
		fail("Not a regular file", 0);
	
	memset(&ed, 0, sizeof ed);
	ed.pathname = __libc_progname;
	ed.fd	    = __libc_progfd;
	ed.absolute = 1;
	
	elo_load(&ed);
	p_main = (void *)ed.entry;
}

void _minver(int major, int minor)
{
	if (major > SYSVER_MAJOR || (major == SYSVER_MAJOR && minor > SYSVER_MINOR))
		fail("This program requires a newer OS version", 0);
}

void load_exec(void)
{
	struct exehdr xhdr;
	struct stat st;
	
	fstat(__libc_progfd, &st);
	if (!S_ISREG(st.st_mode))
		fail("Not a regular file", 0);
	
	_set_errno(0);
	if (read(__libc_progfd, &xhdr, sizeof xhdr) != sizeof xhdr)
		fail("Unable to read image file header", _get_errno());
	
	if (memcmp(xhdr.magic, XMAGIC, 4))
		fail("Bad exec format", 0);
	
	_minver(xhdr.os_major, xhdr.os_minor);
	
	if (_pg_alloc(xhdr.base >> 12, (xhdr.end + 4095) >> 12))
		fail("Unable to allocate memory for program image", _get_errno());
	
	if (lseek(__libc_progfd, 0L, SEEK_SET))
		fail("Seek failed on program image", _get_errno());
	
	_set_errno(0);
	if (read(__libc_progfd, (void *)(uintptr_t)xhdr.base, st.st_size) != st.st_size)
		fail("Unable to read program image", _get_errno());
	
	ck_exec((void *)(uintptr_t)xhdr.base, st.st_size);
	_csync((void *)(uintptr_t)xhdr.base, st.st_size);
	
	p_environ = (void *)(uintptr_t)xhdr.environ;
	p_errno	  = (void *)(uintptr_t)xhdr.errno;
	p_main	  = (void *)(uintptr_t)xhdr.entry;
	v_end	  = xhdr.end;
}

void *lgetsym(const char *name)
{
	char msg[256];
	int i;
	
	for (i = 0; exports[i].name; i++)
		if (!strcmp(exports[i].name, name))
			return exports[i].addr;
	
	sprintf(msg, "unresolved symbol: %s", name);
	fail(msg, 0);
}

void *get_sym(const char *name)
{
	if (libx.xgetsym)
		return libx.xgetsym(name);
	
	return lgetsym(name);
}

#ifdef __ARCH_I386__
void import_sym(char *code_ptr)
{
	unsigned delta;
	char *p = code_ptr - 6;
	
	delta = (unsigned)get_sym(code_ptr) - (unsigned)p - 5;
	
	*(p++) = 0xe9;
	*(p++) = delta;
	*(p++) = delta >> 8;
	*(p++) = delta >> 16;
	*(p++) = delta >> 24;
}
#elif defined __ARCH_AMD64__
void import_sym(char *code_ptr)
{
	unsigned delta;
	char *p = code_ptr - 7;
	
	delta = (uintptr_t)get_sym(code_ptr) - (uintptr_t)p - 5;
	
	*(p++) = 0xe9;
	*(p++) = delta;
	*(p++) = delta >> 8;
	*(p++) = delta >> 16;
	*(p++) = delta >> 24;
}
#else
#error Unknown arch
#endif

static void load_interp(void)
{
#define SB_BUF	4096
	ssize_t cnt;
	char **nav;
	char *intn;
	char *inta;
	char *sb;
	char *p;
	int eac = 1;
	
	if (getuid() != geteuid() || getgid() != getegid())
		fail("setuid is not supported for interpreted programs", 0);
	
	sb = malloc(SB_BUF);
	if (sb == NULL)
		fail("unable to allocate memory", _get_errno());
	
	if (lseek(__libc_progfd, 2L, SEEK_SET) != 2)
		fail("seek failed on program image", _get_errno());
	
	cnt = read(__libc_progfd, sb, SB_BUF - 1);
	if (cnt < 0)
		fail("read error", _get_errno());
	sb[cnt] = 0;
	
	p = strchr(sb, '\n');
	if (p == NULL)
		fail("invalid interpreter specification", 0);
	cnt = p - sb;
	
	while (cnt && isspace(sb[cnt - 1]))
		cnt--;
	sb[cnt] = 0;
	
	p = realloc(sb, cnt + 1);
	if (p != NULL)
		sb = p;
	
	intn = sb;
	while (isspace(*intn))
		intn++;
	
	inta = intn;
	while (*inta && !isspace(*inta))
		inta++;
	if (*inta)
	{
		*inta++ = 0;
		while (isspace(*inta))
			inta++;
		if (*inta)
			eac = 2;
	}
	
	nav = realloc(__libc_argv, sizeof *nav * (__libc_argc + eac));
	if (nav == NULL)
		fail("unable to reallocate argv", _get_errno());
	
	nav[0] = __libc_progname;
	memmove(nav + eac, nav, sizeof *nav * __libc_argc);
	nav[0] = intn;
	if (eac > 1)
		nav[1] = inta;
	
	__libc_argv  = nav;
	__libc_argc += eac;
	
	__libc_progname = intn;
	__libc_progfd = open(intn, O_RDONLY);
	if (__libc_progfd < 0)
		fail("cannot open interpreter", _get_errno());
#undef SB_BUF
}

static void noargs(void)
{
	char **env;
	
	__libc_argv = malloc(sizeof *__libc_argv * 2);
	__libc_argc = 1;
	if (!__libc_argv)
		__libc_panic("load: cannot allocate argv\n");
	__libc_argv[0] = __libc_progname;
	__libc_argv[1] = NULL;
	
	env = malloc(sizeof *env);
	if (!env)
		__libc_panic("load: cannot allocate environ\n");
	_set_environ(env);
	env[0] = NULL;
	
	__libc_umask = 077;
}

static void load_libx(void)
{
#ifdef __ARCH_I386__
	const char *isa = "i386";
#elif defined __ARCH_AMD64__
	const char *isa = "amd64";
#else
#error Unknown arch
#endif
	const char *msg = NULL;
	struct utsname un;
	struct stat st;
	int npg;
	int fd;
	
	fd = open(_PATH_L_LIBX, O_RDONLY);
	if (fd < 0)
		return;
	
	if (fstat(fd, &st))
	{
		msg = "Cannot stat";
		goto clean;
	}
	npg = (st.st_size + PAGE_SIZE - 1) >> PAGE_SHIFT;
	
	if (_pg_alloc(PAGE_LIBX, PAGE_LIBX + npg))
	{
		msg = "Cannot allocate memory";
		goto clean;
	}
	
	if (read(fd, (void *)(PAGE_LIBX << PAGE_SHIFT), st.st_size) != st.st_size)
	{
		msg = "Cannot read";
		goto clean;
	}
	
	uname(&un);
	
	libxh = (void *)(PAGE_LIBX << PAGE_SHIFT);
	
	if (strcmp(libxh->arch, isa))
	{
		_sysmesg(libxh->arch);
		_sysmesg("\n");
		
		msg = "ISA mismatch";
		goto clean;
	}
	
	if (libxh->baseaddr != libxh)
	{
		msg = "Base address mismatch";
		goto clean;
	}
	
	if (libxh->sysmajver  > SYSVER_MAJOR || (libxh->sysmajver == SYSVER_MAJOR && libxh->sysminver > SYSVER_MINOR))
	{
		msg = "Newer OS version required";
		goto clean;
	}
	
	libxh->entry(SYSVER_MAJOR, SYSVER_MINOR, &libx);
clean:
	if (getpid() == 1 && msg)
	{
		_sysmesg(_PATH_L_LIBX);
		_sysmesg(": ");
		_sysmesg(msg);
		_sysmesg("\n");
	}
	close(fd);
}

void load_main(char *arg, int arg_len, char *progname, int progfd)
{
	int	interp = 0;
	char	magic[4];
	char **	environ;
	int	errno;
	
	_set_errno_ptr(&errno);
	
	__libc_arg	= arg;
	__libc_arg_len	= arg_len;
	__libc_progname	= progname;
	__libc_progfd	= progfd;
	
	_set_environ_ptr(&environ);
	
	if (!__libc_progname)
	{
		__libc_progname = "/sbin/init";
		__libc_progfd = open(__libc_progname, O_RDONLY);
		if (__libc_progfd < 0)
		{
			_sysmesg("load: error opening /sbin/init\n");
			for (;;)
				pause();
		}
	}
	
	if (getpid() != 1)
	{
		if (fcntl(0, F_GETFD) < 0 && open("/dev/null", O_RDONLY))
			_exit(255);
		if (fcntl(1, F_GETFD) < 0 && open("/dev/null", O_RDONLY) != 1)
			_exit(255);
		if (fcntl(2, F_GETFD) < 0 && open("/dev/null", O_RDONLY) != 2)
			_exit(255);
	}
	
	__libc_malloc_init();
	__libc_stdio_init();
	__libc_rand_init();
	form_init();
	
	if (arg)
		__libc_initargenv((void *)arg, arg_len);
	else
		noargs();
	
	load_libx();
retry:
	_set_errno(0);
	if (read(__libc_progfd, magic, sizeof magic) != sizeof magic)
		fail("can't read executable file", _get_errno());
	if (lseek(__libc_progfd, 0L, SEEK_SET))
		fail("seek failed on program image", _get_errno());
	if (!interp && magic[0] == '#' && magic[1] == '!')
	{
		load_interp();
		interp = 1;
		goto retry;
	}
	else if (!memcmp(magic, "\177ELF", 4))
		load_elf();
	else
		load_exec();
	close(__libc_progfd);
	
	if (p_environ)
		_set_environ_ptr(p_environ);
	if (p_errno)
		_set_errno_ptr(p_errno);
	
	if (libx.progstart)
		libx.progstart(__libc_progname, __libc_argc, __libc_argv);
	
	exit(p_main(__libc_argc, __libc_argv, _get_environ(), libc_entry));
}
