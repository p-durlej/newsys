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

#include <arch/archdef.h>
#include <kern/page.h>
#include <sys/io.h>
#include <wingui_msgbox.h>
#include <wingui_form.h>
#include <wingui.h>
#include <newtask.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <os386.h>
#include <err.h>

struct form *main_form;

void segv_click(struct gadget *g, int x, int y)
{
	char *p = (void *)0x1234567;
	
	msgbox(main_form, "fault_test", "I will now touch an invalid memory location\n\n"
					"this should result in SIGSEGV signal\n"
					"(segmentation fault).");
	
	*p = 0;
}

void divz_click(struct gadget *g, int x, int y)
{
	int v = 1;
	
	msgbox(main_form, "fault_test", "I will now attempt to divide by zero\n\n"
					"this should result in SIGFPE signal.");
	
	v = v / 0;
}

void loop_click(struct gadget *g, int x, int y)
{
	msgbox(main_form, "fault_test", "I will now enter endless loop\n\n"
					"system slowdown may occur\n"
					"but you should be able to access\n"
					"other tasks.");
	
	for (;;)
		;
}

#ifdef __ARCH_I386__
void inb_click(struct gadget *g, int x, int y)
{
	msgbox(main_form, "fault_test", "I will now attempt to execute an INB instruction\n\n"
					"this should result in SIGILL signal\n"
					"(invalid instruction).");
	
	inb(0x70);
}

void tron_click(struct gadget *g, int x, int y)
{
	msgbox(main_form, "fault_test", "I will now enable single step mode\n"
					"this should result in SIGTRAP.");
	
	asm(
	"pushfl;"
	"popl	%eax;"
	"orl	$0x00000100, %eax;"
	"pushl	%eax;"
	"popfl;"
	);
}

void int3_click(struct gadget *g, int x, int y)
{
	msgbox(main_form, "fault_test", "I will now execute trap instruction\n"
					"this should result in SIGTRAP.");
	
	asm("int3");
}
#endif

void badf_click(struct gadget *g, int x, int y)
{
	char buf[1024];
	
	msgbox(main_form, "fault_test", "I will now try to read from bad descriptor\n"
					"nothing bad should happen (EBADF expected).");
	errno = 0;
	read(6, buf, sizeof buf);
	sprintf(buf, "%m\n");
	msgbox(main_form, "fault_test", buf);
}

void noti_click(struct gadget *g, int x, int y)
{
	msgbox(main_form, "fault_test", "I will now send wingui update request (\"notifications\") in a loop.");
	
	for (;;)
		win_update();
}

void badw_click(struct gadget *g, int x, int y)
{
	int fd;
	
	msgbox(main_form, "fault_test", "I will now try to write to a file with bad buffer pointer.");
	
	fd = open("/badw_test", O_CREAT|O_TRUNC|O_WRONLY, 0600);
	write(fd, NULL, 1048576);
	close(fd);
}

void newt_click(struct gadget *g, int x, int y)
{
	while (_newtaskl("/bin/calc", "/bin/calc", NULL) > 0);
}

void wcre_click(struct gadget *g, int x, int y)
{
	int i;
	
	form_creat(FORM_FRAME | FORM_TITLE, 1, 0, 0, 320, 200, "fault_test");
	for (i = 0; i < 200; i++)
		form_creat(FORM_EXCLUDE_FROM_LIST, 1, i * 2, 0, 1, 200, "fault_test");
}

void pgal_click(struct gadget *g, int x, int y)
{
	char *p = (void *)(PAGE_STACK * PAGE_SIZE);
	
	while (!_pg_alloc(PAGE_STACK, PAGE_STACK + 1))
		*p = 123;
	perror("pg_alloc");
}

void wstk_click(struct gadget *g, int x, int y)
{
	static char *p;
	
	const size_t sz = 16777216;
	int fd;
	
	if (p == NULL)
	{
		p = malloc(sz);
		if (p == NULL)
		{
			perror("malloc");
			return;
		}
	}
	memset(p, 'X', sz);
	
	fd = open("/dev/tty", O_WRONLY);
	if (fd < 0)
	{
		perror("dup2");
		return;
	}
	
	if (dup2(fd, 42) < 0)
	{
		perror("dup2");
		return;
	}
	
	if (write(42, p, sz) < 0)
		perror("write");
}

void wldr_click(struct gadget *g, int x, int y)
{
	int fd;
	
	fd = open("/sbin/init", O_RDONLY);
	if (fd < 0)
		warn("open: /sbin/init");
	
	if (read(fd, 0x7fc00000, 4096) < 0)
		warn("read: /sbin/init");
}

int main_form_close(struct form *f)
{
	exit(0);
}

int main(int argc, char **argv)
{
	if (win_attach())
		err(255, NULL);
	
	msgbox(main_form, "fault_test", "This program will test OS/386\n"
					"protection facilities.\n\n"
					"Exceptions will be generated\n"
					"intentionally.\n\n"
					"Note that this is only a test.\n\n"
					"No real damage should result\n"
					"as the kernel protects itself\n"
					"and other tasks.");
	
	main_form = form_creat(FORM_FRAME | FORM_TITLE | FORM_ALLOW_CLOSE | FORM_ALLOW_MINIMIZE, 1, -1, -1, 225, 190, "fault_test");
	form_on_close(main_form, main_form_close);
	button_creat(main_form,   5,  5,  50, 40, "SEGV",	segv_click);
	button_creat(main_form,  60,  5,  50, 40, "DIVZ",	divz_click);
	button_creat(main_form, 115,  5,  50, 40, "LOOP",	loop_click);
#ifdef __ARCH_I386__
	button_creat(main_form, 170,   5, 50, 40, "INB",	inb_click);
	
	button_creat(main_form,   5,  50, 50, 40, "TRON",	tron_click);
	button_creat(main_form,  60,  50, 50, 40, "INT 3",	int3_click);
#endif
	button_creat(main_form, 115,  50, 50, 40, "BADF",	badf_click);
	button_creat(main_form, 170,  50, 50, 40, "NOTIFY",	noti_click);
	
	button_creat(main_form,   5,  95, 50, 40, "BADWR",	badw_click);
	button_creat(main_form,  60,  95, 50, 40, "NEWTASK",	newt_click);
	button_creat(main_form, 115,  95, 50, 40, "WINDOWS",	wcre_click);
	button_creat(main_form, 170,  95, 50, 40, "PGAL",	pgal_click);
	
	button_creat(main_form,   5, 140, 50, 40, "WSTK",	wstk_click);
	button_creat(main_form,  60, 140, 50, 40, "WLDR",	wldr_click);
	
	for (;;)
		win_wait();
}
