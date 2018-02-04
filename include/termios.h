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

#ifndef _TERMIOS_H
#define _TERMIOS_H

#define ISIG		1
#define ICANON		2
#define ECHO		4
#define ECHONL		8
#define ECHOCTL		16

#define VINTR		0
#define VQUIT		1
#define VERASE		2
#define VKILL		3
#define VEOF		4
#define VWERASE		5
#define VSTATUS		6
#define VREPRINT	7

#define NCCS		8

#define TCSANOW		0
#define TCSADRAIN	1
#define TCSAFLUSH	2

#define TCGETS		0x7401
#define TCSETS		0x7402
#define TCSETSW		0x7403
#define TCSETSF		0x7404

typedef int	tcflag_t;
typedef char	cc_t;

struct termios
{
	tcflag_t	c_iflag;
	tcflag_t	c_oflag;
	tcflag_t	c_lflag;
	tcflag_t	c_cflag;
	cc_t		c_cc[NCCS];
};

int tcsetattr(int d, int a, const struct termios *ti);
int tcgetattr(int d, struct termios *ti);

void cfmakeraw(struct termios *ti);

#endif
