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

#include <priv/stdio.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>

int __libc_stdio_init(void)
{
	FILE *stdout = _get_stdout();
	FILE *stderr = _get_stderr();
	int i;
	
	for (i = 0; i < OPEN_MAX; i++)
		files[i].fd = -1;
	
	fdopen(0, "r");
	fdopen(1, "w");
	fdopen(2, "w");
	stdout->buf_mode = _IOLBF;
	stderr->buf_mode = _IONBF;
	stderr->fputc	 = __libc_putc_nbf;
	stderr->fgetc	 = __libc_getc_nbf;
	return 0;
}

void __libc_stdio_cleanup(void)
{
	int i;
	
	for (i = 0; i < OPEN_MAX; i++)
		if (files[i].fd != -1 && files[i].buf_write)
			fflush(&files[i]);
}

FILE *_get_stdin(void)
{
	return &files[0];
}

FILE *_get_stdout(void)
{
	return &files[1];
}

FILE *_get_stderr(void)
{
	return &files[2];
}
