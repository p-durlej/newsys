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

#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sha.h>
#include <err.h>

extern char *__progname;

static int err;

static void proc_file(struct sha256 *sha, const char *pathname)
{
	char buf[SHA256_CHUNK];
	uint8_t hash[32];
	struct stat st;
	size_t pos;
	FILE *f;
	int i;
	
	f = fopen(pathname, "r");
	if (!f)
	{
		err = errno;
		warn("%s: %s\n", pathname, strerror(errno));
		return;
	}
	if (fstat(fileno(f), &st))
	{
		err = errno;
		warn("%s: %s\n", pathname, strerror(errno));
		goto clean;
	}
	sha256_init(sha, st.st_size);
	for (pos = 0; pos < st.st_size; pos += sizeof buf)
	{
		if (fread(buf, 1, sizeof buf, f) < 1)
		{
			if (ferror(f))
			{
				err = errno;
				warn("%s: %s\n", pathname, strerror(errno));
				goto clean;
			}
			err = 255;
			warnx("%s: Unexpected EOF\n", pathname);
			goto clean;
		}
		sha256_feed(sha, buf);
	}
	sha256_read(sha, hash);
	for (i = 0; i < sizeof hash; i++)
		printf("%02x", hash[i]);
	printf(" %s\n", pathname);
clean:
	fclose(f);
}

int main(int argc, char **argv)
{
	struct sha256 *sha;
	int i;
	
	sha = sha256_creat();
	if (!sha)
		err(errno, "sha_creat");
	
	for (i = 1; i < argc; i++)
		proc_file(sha, argv[i]);
	return err;
}
