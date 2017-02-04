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

#ifndef OVERFLOW_H
#define OVERFLOW_H

/* ov_* are suitable for unsigned types only */

#define ov_add(type, a, b)	((type)(a) + (type)(b) < (type)(a))
#define ov_mul(type, a, b)	((b) && (type)(a) * (type)(b) / (type)(b) != (type)(a))
#define ov_neg(type, v)		(((type)(v) > 0 && -(type)(v) > 0) || ((type)(v) < 0 && -(type)(v) < 0))
#define ov_cast(type, v)	((type)(v) != (v))

#define ov_add_size(a, b)	ov_add(size_t, a, b)
#define ov_add_uoff(a, b)	ov_add(uoff_t, a, b)
#define ov_add_u(a, b)		ov_add(unsigned, a, b)
#define ov_add_ul(a, b)		ov_add(unsigned long, a, b)

#define ov_mul_size(a, b)	ov_mul(size_t, a, b)
#define ov_mul_uoff(a, b)	ov_mul(uoff_t, a, b)
#define ov_mul_u(a, b)		ov_mul(unsigned, a, b)
#define ov_mul_ul(a, b)		ov_mul(unsigned long, a, b)

#endif
