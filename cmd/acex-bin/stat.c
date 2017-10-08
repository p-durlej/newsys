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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <time.h>
#include <err.h>

static int xit;

static void do_stat(const char *pathname)
{
	struct stat st;
	
	if (stat(pathname, &st))
	{
		warn("%s", pathname);
		xit = 1;
		return;
	}
	
	printf("%s: st_ino     = %llu\n",	pathname, (unsigned long long)st.st_ino);
	printf("%s: st_dev     = %u, %u\n",	pathname, (unsigned)major(st.st_dev), (unsigned)minor(st.st_dev));
	printf("%s: st_rdev    = %u, %u\n",	pathname, (unsigned)major(st.st_rdev), (unsigned)minor(st.st_rdev));
	printf("%s: st_mode    = %05lo\n",	pathname, (long)st.st_mode);
	printf("%s: st_nlink   = %u\n",		pathname, (unsigned)st.st_nlink);
	printf("%s: st_uid     = %u\n",		pathname, (unsigned)st.st_uid);
	printf("%s: st_gid     = %u\n",		pathname, (unsigned)st.st_gid);
	printf("%s: st_size    = %llu\n",	pathname, (unsigned long long)st.st_size);
	printf("%s: st_blksize = %u\n",		pathname, (unsigned)st.st_blksize);
	printf("%s: st_blocks  = %llu\n",	pathname, (unsigned long long)st.st_blocks);
	printf("%s: st_atime   = %s",		pathname, ctime(&st.st_atime));
	printf("%s: st_ctime   = %s",		pathname, ctime(&st.st_ctime));
	printf("%s: st_mtime   = %s",		pathname, ctime(&st.st_mtime));
	puts("");
}

int main(int argc, char **argv)
{
	int i;
	
	for (i = 1; i < argc; i++)
		do_stat(argv[i]);
	if (argc < 2)
		do_stat(".");
	return xit;
}
