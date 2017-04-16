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
#include <wingui.h>
#include <sys/stat.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

const char *dlg_disk(struct form *pf, const char *title, const char *pathname, int flags)
{
	static char buf[PATH_MAX];
	
	char msg[PATH_MAX + 64];
	struct stat st;
	
	if (!pathname)
		pathname = "/dev/fd0,0";
	
	if (strlen(pathname) >= sizeof buf)
		pathname = "/dev/fd0,0";
	
	strcpy(buf, pathname);
	
	for (;;)
	{
		dlg_input(pf, title, buf, sizeof buf);
		if (!*buf)
			return NULL;
		
		if (stat(buf, &st))
		{
			int se = _get_errno();
			
			sprintf(msg, "Cannot stat %s", buf);
			msgbox_perror(pf, title, msg, se);
			continue;
		}
		
		if (!S_ISBLK(st.st_mode))
		{
			msgbox(pf, title, "Not a block device.");
			continue;
		}
		
		break;
	}
	
	return buf;
}
