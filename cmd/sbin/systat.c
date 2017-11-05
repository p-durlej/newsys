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

#include <systat.h>
#include <stdio.h>
#include <err.h>

int main(int argc, char **argv)
{
	struct systat st;
	
	if (_systat(&st))
		err(1, "_systat");
	
	printf("task_avail = %i\n",	(int)st.task_avail);
	printf("task_max   = %i\n",	(int)st.task_max);
	printf("core_avail = %lli\n",	(long long)st.core_avail);
	printf("core_max   = %lli\n",	(long long)st.core_max);
	printf("kva_avail  = %lli\n",	(long long)st.kva_avail);
	printf("kva_max    = %lli\n",	(long long)st.kva_max);
	printf("file_avail = %i\n",	(int)st.file_avail);
	printf("file_max   = %i\n",	(int)st.file_max);
	printf("fso_avail  = %i\n",	(int)st.fso_avail);
	printf("fso_max    = %i\n",	(int)st.fso_max);
	printf("blk_dirty  = %i\n",	(int)st.blk_dirty);
	printf("blk_valid  = %i\n",	(int)st.blk_valid);
	printf("blk_avail  = %i\n",	(int)st.blk_avail);
	printf("blk_max    = %i\n",	(int)st.blk_max);
	printf("sw_freq    = %i\n",	(int)st.sw_freq);
	printf("hz         = %i\n",	(int)st.hz);
	printf("uptime     = %i\n",	(int)st.uptime);
	printf("cpu        = %i\n",	(int)st.cpu);
	printf("cpu_max    = %i\n",	(int)st.cpu_max);
	printf("mod        = %i\n",	(int)st.mod);
	printf("mod_max    = %i\n",	(int)st.mod_max);
	
	return 0;
}
