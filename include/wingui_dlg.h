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

#ifndef _WINGUI_DLG_H
#define _WINGUI_DLG_H

#include <sys/types.h>
#include <wingui.h>

#define DLG_INPUT_ALLOW_CANCEL	1

#define DLG_FILE_WANT_ANY		1
#define DLG_FILE_WANT_REG		2
#define DLG_FILE_WANT_DIR		4
#define DLG_FILE_WANT_CHR		8
#define DLG_FILE_WANT_BLK		16
#define DLG_FILE_CONFIRM_OVERWRITE	(0x80000000)

#define DLG_DISK_ANY		1
#define DLG_DISK_FLOPPY		2
#define DLG_DISK_HARD		4
#define DLG_DISK_PARTITION	8

struct form;

int  dlg_file6(struct form *pf, const char *title, const char *button, char *pathname, int maxlen, int flags);
int  dlg_file(struct form *pf, const char *title, const char *button, char *pathname, int maxlen);
int  dlg_open(struct form *pf, const char *title, char *pathname, int maxlen);
int  dlg_save(struct form *pf, const char *title, char *pathname, int maxlen);

int  dlg_color(struct form *pf, const char *title, struct win_rgba *rgba);

int  dlg_newpass(struct form *pf, char *pass, int maxlen);
int  dlg_uid(struct form *pf, const char *title, uid_t *uid);
int  dlg_gid(struct form *pf, const char *title, gid_t *gid);

void dlg_about7(struct form *pf,
	const char *title, const char *item, const char *product,
	const char *author, const char *contact, const char *icon);
void dlg_about6(struct form *pf,
	const char *title, const char *item, const char *product,
	const char *author, const char *contact);
void dlg_about(struct form *pf, const char *title, const char *desc);

int dlg_input5(struct form *pf, const char *title, char *buf, int maxlen, int flags);
int dlg_input(struct form *pf, const char *title, char *buf, int maxlen);

void dlg_reboot(struct form *pf, const char *title);
void dlg_newsess(struct form *pf, const char *title);

const char *dlg_disk(struct form *pf, const char *title, const char *pathname, int flags);

#endif
