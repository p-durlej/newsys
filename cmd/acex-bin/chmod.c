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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <err.h>

static void parse_mode(const char *mspec, mode_t *set, mode_t *clr, mode_t *bX)
{
	mode_t mode_bigX = 0;
	mode_t mode_s = 0;
	mode_t mode_c = 0;
	mode_t r = 0;
	mode_t w = 0;
	mode_t x = 0;
	mode_t s = 0;
	const char *p;
	mode_t *mp;
	
	if (isdigit(*mspec))
	{
		*set = strtoul(mspec, NULL, 8);
		*clr = ~*set;
		return;
	}
	
	p = mspec;
	do
	{
		r = w = x = s = 0;
		
		if (*p != 'a' && *p != 'u' && *p != 'g' && *p != 'o')
		{
			r = umask(0777);
			umask(r);
			
			w = ~r & 0222;
			x = ~r & 0111;
			r = ~r & 0444;
		}
		else
		while (*p == 'a' || *p == 'u' || *p == 'g' || *p == 'o')
		{
			switch (*p)
			{
			case 'a':
				r = S_IRUSR | S_IRGRP | S_IROTH;
				w = S_IWUSR | S_IWGRP | S_IWOTH;
				x = S_IXUSR | S_IXGRP | S_IXOTH;
				s = S_ISUID | S_ISGID;
				break;
			case 'u':
				r |= S_IRUSR;
				w |= S_IWUSR;
				x |= S_IXUSR;
				s |= S_ISUID;
				break;
			case 'g':
				r |= S_IRGRP;
				w |= S_IWGRP;
				x |= S_IXGRP;
				s |= S_ISGID;
				break;
			case 'o':
				r |= S_IROTH;
				w |= S_IWOTH;
				x |= S_IXOTH;
				s |= S_ISGID;
				break;
			}
			p++;
		}
		
		switch (*p)
		{
		case '+':
			mp = &mode_s;
			break;
		case '-':
			mp = &mode_c;
			break;
		case '=':
			mode_c = r | w | x;
			mp = &mode_s;
			break;
		default:
			goto bad_mstr;
		}
		p++;
		
		while (*p && *p != ',')
		{
			switch(*p)
			{
			case 'r':
				*mp |= r;
				break;
			case 'w':
				*mp |= w;
				break;
			case 'x':
				*mp |= x;
				break;
			case 'X':
				mode_bigX |= x;
				break;
			case 's':
				*mp |= s;
				break;
			case '+':
				mp = &mode_s;
				break;
			case '-':
				mp = &mode_c;
				break;
			default:
				goto bad_mstr;
			}
			p++;
		}
		
		if (*p == ',')
			p++;
	} while (*p);
	
	*bX  = mode_bigX;
	*set = mode_s;
	*clr = mode_c;
	return;
bad_mstr:
	errx(255, "bad mode string");
}

int main(int argc, char **argv)
{
	struct stat st;
	mode_t set, clr, bigX = 0;
	int err = 0;
	int fd;
	int i;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		printf("\nUsage: chmod [PERM] FILE...\n"
			"       chmod [a][u][g][o](+|-|=)[r][w][x][X][s][,...] FILE...\n\n"
			"Set permission bits on FILEs to PERM.\n"
			"Set (+) specified permission bits (and preserver other bits) on FILEs.\n"
			"Clear (-) specified permission bits (and preserve other bits) on FILEs.\n"
			"Clear mode bits according to [a][u][g][o], then set (=) specified permission\n"
			"bits (and reset other bits) on FILEs.\n\n"
			);
		return 0;
	}
	
	if (argc < 3)
		errx(255, "too few arguments");
	
	parse_mode(argv[1], &set, &clr, &bigX);
	for (i = 2; i < argc; i++)
	{
		fd = open(argv[i], O_NOIO);
		if (fd < 0)
		{
			err = errno;
			warn("%s", argv[i]);
			continue;
		}
		
		if (fstat(fd, &st))
		{
			err = errno;
			warn("%s", argv[i]);
			close(fd);
			continue;
		}
		
		if ((st.st_mode & 0111) || S_ISDIR(st.st_mode))
			st.st_mode |= bigX;
		st.st_mode |=  set;
		st.st_mode &= ~clr;
		
		if (fchmod(fd, st.st_mode))
		{
			err = errno;
			perror(argv[i]);
		}
		close(fd);
	}
	return err;
}
