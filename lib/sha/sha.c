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

#include <arch/archdef.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sha.h>

struct sha256
{
	uint32_t a, b, c, d, e, f, g, h;
	uint32_t hash[8];
	uint64_t length;
	uint64_t resid;
};

static void sha256_step(struct sha256 *sha, const uint32_t *src);

static const uint32_t sha256_ihash[8] =
{
	0x6a09e667,
	0xbb67ae85,
	0x3c6ef372,
	0xa54ff53a,
	0x510e527f,
	0x9b05688c,
	0x1f83d9ab,
	0x5be0cd19
};

static const uint32_t sha256_cpfrac[64] =
{
	0x428a2f98,
	0x71374491,
	0xb5c0fbcf,
	0xe9b5dba5,
	0x3956c25b,
	0x59f111f1,
	0x923f82a4,
	0xab1c5ed5,
	0xd807aa98,
	0x12835b01,
	0x243185be,
	0x550c7dc3,
	0x72be5d74,
	0x80deb1fe,
	0x9bdc06a7,
	0xc19bf174,
	0xe49b69c1,
	0xefbe4786,
	0x0fc19dc6,
	0x240ca1cc,
	0x2de92c6f,
	0x4a7484aa,
	0x5cb0a9dc,
	0x76f988da,
	0x983e5152,
	0xa831c66d,
	0xb00327c8,
	0xbf597fc7,
	0xc6e00bf3,
	0xd5a79147,
	0x06ca6351,
	0x14292967,
	0x27b70a85,
	0x2e1b2138,
	0x4d2c6dfc,
	0x53380d13,
	0x650a7354,
	0x766a0abb,
	0x81c2c92e,
	0x92722c85,
	0xa2bfe8a1,
	0xa81a664b,
	0xc24b8b70,
	0xc76c51a3,
	0xd192e819,
	0xd6990624,
	0xf40e3585,
	0x106aa070,
	0x19a4c116,
	0x1e376c08,
	0x2748774c,
	0x34b0bcb5,
	0x391c0cb3,
	0x4ed8aa4a,
	0x5b9cca4f,
	0x682e6ff3,
	0x748f82ee,
	0x78a5636f,
	0x84c87814,
	0x8cc70208,
	0x90befffa,
	0xa4506ceb,
	0xbef9a3f7,
	0xc67178f2
};

void sha256_free(struct sha256 *sha)
{
	free(sha);
}

struct sha256 *sha256_creat(void)
{
	struct sha256 *p;
	
	p = calloc(sizeof *p, 1);
	if (!p)
		return NULL;
	return p;
}

void sha256_init(struct sha256 *sha, uint64_t len)
{
	uint32_t src32[16];
	
	memcpy(&sha->hash, &sha256_ihash, sizeof sha->hash);
	sha->length = sha->resid = len;
	
	if (!sha->length)
	{
		memset(src32, 0, sizeof src32);
		src32[0] = 0x80000000;
		sha256_step(sha, src32);
	}
}

static uint32_t sha_rotr32(uint32_t v, int cnt)
{
	return (v >> cnt) | (v << (32 - cnt));
}

static uint32_t sha256_Ch(uint32_t x, uint32_t y, uint32_t z)
{
	return (x & y) ^ (~x & z);
}

static uint32_t sha256_Maj(uint32_t x, uint32_t y, uint32_t z)
{
	return (x & y) ^ (x & z) ^ (y & z);
}

static uint32_t sha256_E0(uint32_t v)
{
	return sha_rotr32(v, 2) ^ sha_rotr32(v, 13) ^ sha_rotr32(v, 22);
}

static uint32_t sha256_E1(uint32_t v)
{
	return sha_rotr32(v, 6) ^ sha_rotr32(v, 11) ^ sha_rotr32(v, 25);
}

static uint32_t sha256_r0(uint32_t v)
{
	return sha_rotr32(v, 7) ^ sha_rotr32(v, 18) ^ (v >> 3);
}

static uint32_t sha256_r1(uint32_t v)
{
	return sha_rotr32(v, 17) ^ sha_rotr32(v, 19) ^ (v >> 10);
}

static void sha256_step(struct sha256 *sha, const uint32_t *src)
{
	uint32_t a, b, c, d, e, f, g, h;
	uint32_t t1, t2;
	uint32_t w[64];
	int i;
	
	memcpy(w, src, sizeof *w * 16);
	for (i = 16; i < 64; i++)
		w[i] = sha256_r1(w[i -  2]) + w[i -  7] + sha256_r0(w[i - 15]) + w[i - 16];
	a = sha->hash[0];
	b = sha->hash[1];
	c = sha->hash[2];
	d = sha->hash[3];
	e = sha->hash[4];
	f = sha->hash[5];
	g = sha->hash[6];
	h = sha->hash[7];
	for (i = 0; i < 64; i++)
	{
		t1 = h + sha256_E1(e) + sha256_Ch(e, f, g) + sha256_cpfrac[i] + w[i];
		t2 =	 sha256_E0(a) + sha256_Maj(a, b, c);
		
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}
	sha->hash[0] += a;
	sha->hash[1] += b;
	sha->hash[2] += c;
	sha->hash[3] += d;
	sha->hash[4] += e;
	sha->hash[5] += f;
	sha->hash[6] += g;
	sha->hash[7] += h;
}

#if defined __ARCH_I386__ || defined __ARCH_AMD64__
uint32_t betoh(uint32_t v)
{
	v = ((v & 0x00ff00ff) <<  8) | ((v & 0xff00ff00) >> 8);
	v = ((v & 0x0000ffff) << 16) |  (v >> 16);
	
	return v;
}
#else
#error Unknown arch
#endif

void sha256_feed(struct sha256 *sha, const void *src)
{
	uint32_t src32[16];
	int bsz;
	int i;
	
	if (sha->resid > SHA256_CHUNK)
		bsz = SHA256_CHUNK;
	else
		bsz = sha->resid;
	
	memcpy(src32, src, bsz);
	memset((char *)src32 + bsz, 0, SHA256_CHUNK - bsz);
	for (i = 0; i < 16; i++)
		src32[i] = betoh(src32[i]);
	if (bsz < SHA256_CHUNK)
	{
		if (bsz < SHA256_CHUNK - 8)
		{
			src32[15] = sha->length >> 29;
			src32[15] = sha->length << 3;
		}
		src32[bsz / 4] |= 0x80000000 >> (bsz % 4 * 8);
	}
	sha256_step(sha, src32);
	sha->resid -= bsz;
	if (!sha->resid && bsz >= SHA256_CHUNK - 8)
	{
		memset(src32, 0, sizeof src32);
		src32[14] = sha->length >> 29;
		src32[15] = sha->length << 3;
		if (bsz >= SHA256_CHUNK)
			src32[0] = 0x80000000;
		sha256_step(sha, src32);
	}
}

void sha256_read(struct sha256 *sha, uint8_t *buf)
{
	uint8_t *dp8;
	int i;
	
	for (dp8 = buf, i = 0; i < 8; i++)
	{
		*dp8++ = sha->hash[i] >> 24;
		*dp8++ = sha->hash[i] >> 16;
		*dp8++ = sha->hash[i] >> 8;
		*dp8++ = sha->hash[i];
	}
}
