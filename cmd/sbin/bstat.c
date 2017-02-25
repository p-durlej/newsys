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

#include <bioctl.h>
#include <stdio.h>
#include <err.h>

int main(int argc, char **argv)
{
	int cnt;
	int i;
	
	cnt = _bdev_max();
	if (cnt < 0)
		err(1, NULL);
	
	struct bdev_stat bs[cnt];
	
	if (_bdev_stat(bs))
		err(1, NULL);
	
	puts("NAME             READS    WRITES    ERRORS");
	puts("------------ --------- --------- ---------");
	
	for (i = 0; i < cnt; i++)
		if (*bs[i].name)
		{
			if (!bs[i].read_cnt && !bs[i].write_cnt && !bs[i].error_cnt)
				continue;
			
			printf("%-12s %9ji %9ji %9ji\n",
				bs[i].name,
				(uintmax_t)bs[i].read_cnt,
				(uintmax_t)bs[i].write_cnt,
				(uintmax_t)bs[i].error_cnt);
		}
	
	return 0;
}
