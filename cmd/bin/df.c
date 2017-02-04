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

#include <fmthuman.h>
#include <string.h>
#include <unistd.h>
#include <statfs.h>
#include <mount.h>
#include <errno.h>
#include <stdio.h>

static int hflag;

int main(int argc, char **argv)
{
	struct _mtab mtab[MOUNT_MAX];
	struct statfs st;
	int st_valid;
	int c;
	int i;
	
	bzero(mtab, sizeof mtab);
	
	if (_mtab(mtab, sizeof mtab))
	{
		perror("_mtab");
		return errno;
	}
	
	while (c = getopt(argc, argv, "h"), c >= 0)
		switch (c)
		{
		case 'h':
			hflag = 1;
			break;
		default:
			return 1;
		}
	
	if (hflag)
		printf("Device       Prefix             Total       Used  Available Use%%\n");
	else
		printf("Device       Prefix       512b-blocks       Used  Available Use%%\n");
	
	for (i = 0; i < MOUNT_MAX; i++)
	{
		if (!mtab[i].mounted)
			continue;
		
		if (!*mtab[i].prefix)
			strcpy(mtab[i].prefix, "/");
		
		st_valid = !_statfs(mtab[i].prefix, &st);
		
		printf("%-12s",  mtab[i].device);
		printf(" %-12s", mtab[i].prefix);
		if (!st_valid)
			printf("           -          -          -");
		else if (hflag)
		{
			printf(" %11s", fmthumanoffs(st.blk_total * 512, 1));
			printf(" %10s", fmthumanoffs((st.blk_total - st.blk_free) * 512, 1));
			printf(" %10s", fmthumanoffs(st.blk_free * 512, 1));
		}
		else
		{
			printf(" %11i", st.blk_total);
			printf(" %10i", st.blk_total - st.blk_free);
			printf(" %10i", st.blk_free);
		}
		if (st_valid && st.blk_total)
			printf(" %3i%%\n", 100 * (st.blk_total - st.blk_free) / st.blk_total);
		else
			printf("\n");
	}
	
	return 0;
}
