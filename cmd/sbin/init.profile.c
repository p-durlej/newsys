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

#include <priv/pointer.h>
#include <priv/wingui.h>

#include <wingui_msgbox.h>
#include <sys/signal.h>
#include <unistd.h>
#include <wingui.h>
#include <confdb.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <paths.h>
#include <time.h>
#include <err.h>

#define FILEMGR_ARGV0	"-filemgr"

static int lflag;

static void run_filemgr(void)
{
	if (access(_PATH_B_FILEMGR, R_OK | X_OK))
		goto fail;
	
	if (lflag)
		execl(_PATH_B_FILEMGR, FILEMGR_ARGV0, "-d", (void *)NULL);
	else
		execl(_PATH_B_FILEMGR, FILEMGR_ARGV0, (void *)NULL);
	
fail:
	msgbox_perror(NULL, "init.profile", "Could not run " _PATH_B_FILEMGR, errno);
	exit(errno);
}

int main(int argc, char **argv)
{
	struct ptr_conf pc;
	int c;
	int i;
	
	while (c = getopt(argc, argv, "a:l"), c > 0)
		switch (c)
		{
		case 'l':
			lflag = 1;
			break;
		default:
			return 255;
		}
	
	if (win_attach())
		err(255, NULL);
	
	for (i = 1; i <= NSIG; i++)
		signal(i, SIG_DFL);
	signal(SIGCHLD, SIG_IGN);
	
	if (!c_load("ptr_conf", &pc, sizeof pc))
	{
		win_map_buttons(pc.map, sizeof pc.map / sizeof *pc.map);
		win_set_ptr_speed(&pc.speed);
	}
	wc_load_cte();
	wp_load();
	
	run_filemgr();
	return 0;
}
