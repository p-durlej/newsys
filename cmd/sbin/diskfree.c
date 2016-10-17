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

#include <wingui_msgbox.h>
#include <wingui_form.h>
#include <fmthuman.h>
#include <unistd.h>
#include <stdlib.h>
#include <statfs.h>
#include <string.h>
#include <stdio.h>
#include <mount.h>
#include <errno.h>
#include <err.h>

int on_close(struct form *f)
{
	exit(0);
}

int main(int argc, char **argv)
{
	struct _mtab mtab[MOUNT_MAX];
	struct statfs st;
	struct form *f;
	char msg[256];
	int cnt = 0;
	int w, h;
	int y;
	int i;
	
	if (win_attach())
		err(255, NULL);
	
	memset(mtab, 0, sizeof mtab);
	chdir("/");
	
	if (_mtab(mtab, sizeof mtab))
	{
		sprintf(msg, "Cannot retrieve mount table:\n\n%m");
		msgbox(NULL, "Disk Space", msg);
		return errno;
	}
	
	for (i = 0; i < MOUNT_MAX; i++)
		if (mtab[i].mounted)
			cnt++;
	
	win_chr_size(WIN_FONT_DEFAULT, &w, &h, 'X');
	
	f = form_creat(FORM_TITLE | FORM_FRAME | FORM_ALLOW_CLOSE | FORM_ALLOW_MINIMIZE, 1,
		       -1, -1, w * 53, (h + 2) * (cnt + 1) + 4, "Disk Space");
	form_on_close(f, on_close);
	
	label_creat(f, w /  3, 2, "PREFIX");
	label_creat(f, w * 13, 2, "DEVICE");
	label_creat(f, w * 23, 2, "FS TYPE");
	label_creat(f, w * 33, 2, "USED");
	label_creat(f, w * 43, 2, "TOTAL");
	
	y = h + 4;
	for (i = 0; i < MOUNT_MAX; i++)
		if (mtab[i].mounted)
		{
			const char *p = mtab[i].prefix;
			
			if (!*p)
				p = "/";
			
			label_creat(f, w /  6, y, p);
			label_creat(f, w * 13, y, mtab[i].device);
			label_creat(f, w * 23, y, mtab[i].fstype);
			y += h + 2;
			
			win_idle();
		}
	
	y = h + 4;
	for (i = 0; i < MOUNT_MAX; i++)
		if (mtab[i].mounted)
		{
			char total[32] = "-";
			char used[32] = "-";
			
			if (!_statfs(mtab[i].prefix, &st))
			{
				sprintf(used,  "%s", fmthumanoff((st.blk_total - st.blk_free) * 512, 1));
				sprintf(total, "%s", fmthumanoff(st.blk_total * 512, 1));
			}
			
			label_creat(f, w * 33, y, used);
			label_creat(f, w * 43, y, total);
			y += h + 2;
			
			win_idle();
		}
	
	form_wait(f);
	return 0;
}
