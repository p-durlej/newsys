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

#include <wingui_bitmap.h>
#include <priv/pointer.h>
#include <priv/wingui.h>
#include <stdlib.h>
#include <confdb.h>

static void wp_load1(int id, const char *path, const char *name)
{
	struct bitmap *b = NULL;
	struct win_rgba *sp;
	unsigned *mask = NULL;
	unsigned *dp;
	char pathname[PATH_MAX];
	int cnt;
	int hx, hy;
	int w, h;
	
	if (strlen(path) + strlen(name) + 1 >= sizeof pathname)
		return;
	strcpy(pathname, path);
	strcat(pathname, "/");
	strcat(pathname, name);
	
	b = bmp_load(pathname);
	if (!b)
		return;
	bmp_hotspot(b, &hx, &hy);
	bmp_size(b, &w, &h);
	bmp_conv(b);
	
	cnt = w * h;
	if (cnt / w != h)
		goto clean;
	
	mask = calloc(sizeof *mask, cnt);
	if (!mask)
		goto clean;
	
	sp = bmp_rgba(b);
	dp = mask;
	for (; cnt--; sp++, dp++)
		if (sp->a)
			*dp = 1;
	win_insert_ptr(id, bmp_pixels(b), mask, w, h, hx, hy);
clean:
	bmp_free(b);
	free(mask);
}

void wp_load(void)
{
	char pathname[PATH_MAX];
	struct ptr_conf pc;
	
	if (c_load("/user/ptr_conf", &pc, sizeof pc))
		strcpy(pc.ptr_path, "/lib/pointers");
	else if (strlen(pc.ptr_path) >= PATH_MAX - 6)
		strcpy(pc.ptr_path, "/lib/pointers");
	
	wp_load1(WIN_PTR_ARROW,	pc.ptr_path, "arrow");
	wp_load1(WIN_PTR_BUSY,	pc.ptr_path, "busy");
	wp_load1(WIN_PTR_NW_SE,	pc.ptr_path, "nw_se");
	wp_load1(WIN_PTR_TEXT,	pc.ptr_path, "text");
	wp_load1(WIN_PTR_DRAG,	pc.ptr_path, "drag");
}
