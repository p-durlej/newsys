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

#include <wingui_dlg.h>
#include <libx.h>

static int (*pstrcmp)(const char *s1, const char *s2);
static void *(*lgetsym)(const char *name);

void *libc_entry;

static int xdlg_file(struct form *pf, const char *title, const char *button, char *pathname, int maxlen)
{
	return dlg_input5(pf, title, pathname, maxlen, DLG_INPUT_ALLOW_CANCEL) == 1;
}

static int xdlg_open(struct form *pf, const char *title, char *pathname, int maxlen)
{
	return dlg_file(pf, title, "Open", pathname, maxlen);
}

static int xdlg_save(struct form *pf, const char *title, char *pathname, int maxlen)
{
	return dlg_file(pf, title, "Save", pathname, maxlen);
}

static void *xgetsym(const char *name)
{
	if (!pstrcmp(name, "dlg_file")) return xdlg_file;
	if (!pstrcmp(name, "dlg_open")) return xdlg_open;
	if (!pstrcmp(name, "dlg_save")) return xdlg_save;
	
	return lgetsym(name);
}

static void progstart(const char *pathname, int argc, char **argv)
{
	_sysmesg("starting ");
	_sysmesg(pathname);
	_sysmesg("\n");
}

void entry(int sysmajver, int sysminver, struct libx *libx)
{
	pstrcmp	   = libx->lgetsym("strcmp");
	libc_entry = libx->lentry;
	
	if (getpid() == 1)
		_sysmesg("simple file open dialog box extension loaded\n");
	
	lgetsym = libx->lgetsym;
	libx->xgetsym = xgetsym;
	libx->progstart = progstart;
	
	return 0;
}
