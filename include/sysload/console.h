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

#ifndef _SYSLOAD_CONSOLE_H
#define _SYSLOAD_CONSOLE_H

#define INVERSE		1
#define UNDERLINE	2
#define BOLD		4
#define BLINKING	8

#define CON_TRACE	do { con_trace(__FILE__, __func__, __LINE__); } while(0)

extern int con_w, con_h;

void con_init(void);
void con_fini(void);

void con_showlogo(void *data, int size, int width);
void con_hidelogo();

void con_setattr(int bg, int fg, int attr);
void con_gotoxy(int x, int y);
void con_clear(void);
void con_putxy(int x, int y, int c);
void con_putc(unsigned char c);
void con_update(void);

void con_progress(int x, int y, int w, int val, int max);
void con_frame(int x, int y, int w, int h);
void con_rect(int x, int y, int w, int h);

void con_putsxy(int x, int y, const char *s);
void con_puts(const char *s);

void con_status(const char *s, const char *s1);

int con_kbhit(void);
int con_getch(void);

void con_putx32(int v);
void con_putx16(int v);
void con_putx8(int v);
void con_putx4(int v);
void con_putn(int v);

void con_trace(const char *file, const char *func, int line);

#endif
