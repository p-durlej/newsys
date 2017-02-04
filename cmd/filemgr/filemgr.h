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

#include <limits.h>
#include <paths.h>

#define SHUTDOWN_FORM	"/lib/forms/shutdown.frm"

#define DEFAULT_ICON	"/lib/icons/file.pnm"
#define BACKDROP_PATH	"/lib/bg/paper.pnm"
#define VTTY		_PATH_B_VTTY
#define PASSWD		_PATH_B_CPWD
#define SHUTDOWN	_PATH_B_SHUTDOWN
#define SLAVE		_PATH_B_FILEMGR_SLAVE
#define EDITOR		_PATH_B_EDIT

#define SORT_NAME	0
#define SORT_SIZE	1
#define SORT_TYPE	2

struct form;

struct file_type
{
	char type[32];
	char desc[64];
	char icon[PATH_MAX];
	char exec[PATH_MAX];
};

struct fm_config
{
	int show_dotfiles;
	int form_w;
	int form_h;
	int sort_order;
	int large_icons;
	int show_path;
	int win_desk;
};

extern struct fm_config config;
extern int	desktop;

extern int	backdrop_tile;
extern void *	backdrop;

void load_dir();
void launch(const char *app, int follow_link);
void get_info(struct form *pf, const char *name);

int ft_getinfo(struct file_type *ft, const char *pathname);

int file_cpmv(char *src, char *dst, int move);
int file_remove(char *name);

void mkmstr(char *buf, mode_t mode);
void mkostr(char *ubuf, char *gbuf, uid_t uid, gid_t gid);
char *mktstr(const time_t *t);
