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

#include <wingui_bitmap.h>
#include <wingui.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>

struct sbmp_head
{
	char magic[8];
	int width;
	int height;
};

struct bitmap
{
	int			width;
	int			height;
	win_color *		pixels;
	struct win_rgba *	rgba;
	int			hotx, hoty;
};

static void bmp_meta(int *pox, int *poy, const char *meta)
{
	const char *p = meta;
	const char *p1;
	int ox, oy;
	
	while (isspace(*p) && *p != '\n')
		p++;
	
	if (strncmp(p, "NAMELESS-OS: ", 12))
		return;
	p += 12;
	
	ox = strtoul(p, (char **)&p1, 0);
	if (*p1 != ',')
		return;
	p = p1 + 1;
	
	while (isspace(*p) && *p != '\n')
		p++;
	oy = strtoul(p, (char **)&p1, 0);
	
	while (isspace(*p1) && *p1 != '\n')
		p1++;
	if (*p1 != '\n')
		return;
	
	*pox = ox;
	*poy = oy;
}

static struct bitmap *load_pnm(int fd)
{
	struct bitmap *b;
	struct stat st;
	char *data;
	char *p;
	int cnt;
	int ox = -1, oy = -1;
	int w, h;
	int i;
	
	lseek(fd, 0L, SEEK_SET);
	fstat(fd, &st);
	
	data = malloc(st.st_size);
	if (!data)
	{
		close(fd);
		return NULL;
	}
	
	cnt = read(fd, data, st.st_size);
	close(fd);
	if (cnt < 0)
	{
		free(data);
		return NULL;
	}
	if (cnt != st.st_size)
	{
		_set_errno(EINVAL);
		free(data);
		return NULL;
	}
	
	p = strchr(data, '\n');
	if (!p)
	{
		_set_errno(EINVAL);
		free(data);
		return NULL;
	}
	p++;
	
	if (*p == '#')
	{
		bmp_meta(&ox, &oy, p + 1);
		
		p = strchr(p, '\n');
		if (!p)
		{
			_set_errno(EINVAL);
			free(data);
			return NULL;
		}
		p++;
	}
	
	w = strtoul(p, &p, 0);
	h = strtoul(p, &p, 0);
	
	p = strchr(p, '\n');
	if (!p)
	{
		_set_errno(EINVAL);
		free(data);
		return NULL;
	}
	p++;
	
	p = strchr(p, '\n');
	if (!p)
	{
		_set_errno(EINVAL);
		free(data);
		return NULL;
	}
	p++;
	
	b = malloc(sizeof(struct bitmap) + (sizeof(win_color) + sizeof(struct win_rgba)) * w * h);
	if (!b)
	{
		free(data);
		return NULL;
	}
	
	b->pixels = (void *)(b + 1);
	b->rgba	  = (void *)(b->pixels + w * h);
	b->width  = w;
	b->height = h;
	b->hotx	  = ox;
	b->hoty	  = oy;
	
	for (i = 0; i < w * h; i++)
	{
		unsigned char ri, gi, bi;
		
		ri = *(p++);
		gi = *(p++);
		bi = *(p++);
		
		b->rgba[i].r = ri;
		b->rgba[i].g = gi;
		b->rgba[i].b = bi;
		
		if (ri == 255 && gi == 0 && bi == 255)
			b->rgba[i].a = 0;
		else
			b->rgba[i].a = 255;
	}
	free(data);
	
	return b;
}

static struct bitmap *load_pbm(int fd)
{
	struct bitmap *b;
	struct stat st;
	char *data;
	char *p;
	int cnt;
	int ox = -1, oy = -1;
	int w, h;
	int pb;
	int i;
	
	lseek(fd, 0L, SEEK_SET);
	fstat(fd, &st);
	
	data = malloc(st.st_size);
	if (!data)
	{
		close(fd);
		return NULL;
	}
	
	cnt = read(fd, data, st.st_size);
	close(fd);
	if (cnt < 0)
	{
		free(data);
		return NULL;
	}
	if (cnt != st.st_size)
	{
		_set_errno(EINVAL);
		free(data);
		return NULL;
	}
	
	p = strchr(data, '\n');
	if (!p)
	{
		_set_errno(EINVAL);
		free(data);
		return NULL;
	}
	p++;
	
	if (*p == '#')
	{
		bmp_meta(&ox, &oy, p + 1);
		
		p = strchr(p, '\n');
		if (!p)
		{
			_set_errno(EINVAL);
			free(data);
			return NULL;
		}
		p++;
	}
	
	w = strtoul(p, &p, 0);
	h = strtoul(p, &p, 0);
	
	p = strchr(p, '\n');
	if (!p)
	{
		_set_errno(EINVAL);
		free(data);
		return NULL;
	}
	p++;
	
	b = malloc(sizeof(struct bitmap) + (sizeof(win_color) + sizeof(struct win_rgba)) * w * h);
	if (!b)
	{
		free(data);
		return NULL;
	}
	
	b->pixels = (void *)(b + 1);
	b->rgba	  = (void *)(b->pixels + w * h);
	b->width  = w;
	b->height = h;
	b->hotx	  = ox;
	b->hoty	  = oy;
	
	cnt = 0;
	for (i = 0; i < w * h; i++)
	{
		int v;
		
		if (!cnt)
		{
			cnt = 8;
			pb  = *p++;
		}
		
		v = (pb & 128) ? 0 : 255;
		pb <<= 1;
		cnt--;
		
		b->rgba[i].r = v;
		b->rgba[i].g = v;
		b->rgba[i].b = v;
		b->rgba[i].a = 255;
	}
	free(data);
	
	return b;
}

static struct bitmap *load_pgm(int fd)
{
	struct bitmap *b;
	struct stat st;
	char *data;
	char *p;
	int cnt;
	int ox = -1, oy = -1;
	int w, h;
	int i;
	
	lseek(fd, 0L, SEEK_SET);
	fstat(fd, &st);
	
	data = malloc(st.st_size);
	if (!data)
	{
		close(fd);
		return NULL;
	}
	
	cnt = read(fd, data, st.st_size);
	close(fd);
	if (cnt < 0)
	{
		free(data);
		return NULL;
	}
	if (cnt != st.st_size)
	{
		_set_errno(EINVAL);
		free(data);
		return NULL;
	}
	
	p = strchr(data, '\n');
	if (!p)
	{
		_set_errno(EINVAL);
		free(data);
		return NULL;
	}
	p++;
	
	if (*p == '#')
	{
		bmp_meta(&ox, &oy, p + 1);
		
		p = strchr(p, '\n');
		if (!p)
		{
			_set_errno(EINVAL);
			free(data);
			return NULL;
		}
		p++;
	}
	
	w = strtoul(p, &p, 0);
	h = strtoul(p, &p, 0);
	
	p = strchr(p, '\n');
	if (!p)
	{
		_set_errno(EINVAL);
		free(data);
		return NULL;
	}
	p++;
	
	p = strchr(p, '\n');
	if (!p)
	{
		_set_errno(EINVAL);
		free(data);
		return NULL;
	}
	p++;
	
	b = malloc(sizeof(struct bitmap) + (sizeof(win_color) + sizeof(struct win_rgba)) * w * h);
	if (!b)
	{
		free(data);
		return NULL;
	}
	
	b->pixels = (void *)(b + 1);
	b->rgba	  = (void *)(b->pixels + w * h);
	b->width  = w;
	b->height = h;
	b->hotx	  = ox;
	b->hoty	  = oy;
	
	for (i = 0; i < w * h; i++)
	{
		int v;
		
		v = *p++;
		
		b->rgba[i].r = v;
		b->rgba[i].g = v;
		b->rgba[i].b = v;
		b->rgba[i].a = 255;
	}
	free(data);
	
	return b;
}

struct bitmap *bmp_load(const char *pathname)
{
	struct sbmp_head hd;
	struct bitmap *b;
	int size;
	int cnt;
	int fd;
	
	fd = open(pathname, O_RDONLY);
	if (fd < 0)
		return NULL;
	
	switch (read(fd, &hd, sizeof hd))
	{
	case -1:
		close(fd);
		return NULL;
	case sizeof hd:
		break;
	default:
		close(fd);
		_set_errno(EINVAL);
		return NULL;
	}
	
	if (!memcmp(hd.magic, "P6", 2))
		return load_pnm(fd);
	
	if (!memcmp(hd.magic, "P5", 2))
		return load_pgm(fd);
	
	if (!memcmp(hd.magic, "P4", 2))
		return load_pbm(fd);
	
	if (memcmp(hd.magic, "SIMPLEBM", 8))
	{
		close(fd);
		_set_errno(EINVAL);
		return NULL;
	}
	
	size = (sizeof(win_color) + sizeof(struct win_rgba)) * hd.width * hd.height + sizeof(struct bitmap);
	
	b = malloc(size);
	if (!b)
	{
		close(fd);
		return NULL;
	}
	
	b->width  = hd.width;
	b->height = hd.height;
	b->rgba	  = (void *)(b + 1);
	b->pixels = (void *)(b->rgba + hd.width * hd.height);
	b->hotx	  = -1;
	b->hoty	  = -1;
	
	cnt = read(fd, b->rgba, hd.width * hd.height * sizeof(struct win_rgba));
	if (cnt < 0)
	{
		free(b);
		close(fd);
		return NULL;
	}
	
	if (cnt != hd.width * hd.height * sizeof(struct win_rgba))
	{
		free(b);
		close(fd);
		_set_errno(EINVAL);
		return NULL;
	}
	
	close(fd);
	return b;
}

struct win_rgba *bmp_rgba(struct bitmap *bmp)
{
	return bmp->rgba;
}

const win_color *bmp_pixels(struct bitmap *bmp)
{
	return bmp->pixels;
}

int bmp_hotspot(struct bitmap *bmp, int *x, int *y)
{
	*x = bmp->hotx;
	*y = bmp->hoty;
	
	return 0;
}

int bmp_size(struct bitmap *bmp, int *w, int *h)
{
	struct bitmap *b = bmp;
	
	*w = b->width;
	*h = b->height;
	
	return 0;
}

int bmp_free(struct bitmap *bmp)
{
	free(bmp);
	return 0;
}

int bmp_conv(struct bitmap *bmp)
{
	struct bitmap *b = bmp;
	
	if (win_bconv(b->pixels, b->rgba, b->width * b->height))
		return -1;
	return 0;
}

int bmp_draw(int wd, struct bitmap *bmp, int x, int y)
{
	struct bitmap *b = bmp;
	
	return win_bitmap(wd, b->pixels, x, y, b->width, b->height);
}

void bmp_set_bg(struct bitmap *bmp, int r, int g, int b, int a)
{
	struct win_rgba *p, *e;
	
	p = bmp->rgba;
	e = p + bmp->width * bmp->height;
	while (p < e)
	{
		p->r = (p->r * p->a + r * (255 - p->a)) / 255;
		p->g = (p->g * p->a + g * (255 - p->a)) / 255;
		p->b = (p->b * p->a + b * (255 - p->a)) / 255;
		p->a = (p->a * p->a + a * (255 - p->a)) / 255;
		p++;
	}
}

struct bitmap *bmp_scale(struct bitmap *bmp, int w, int h)
{
	struct win_rgba *nrgba;
	struct win_rgba *orgba;
	struct win_rgba *p;
	struct bitmap *nbmp;
	win_color *npix;
	size_t size;
	int ow, oh;
	int x, y;
	
	size  = w * h * sizeof *nbmp->rgba;
	size += w * h * sizeof *nbmp->pixels;  /* XXX overflow */
	size += sizeof *nbmp;
	nbmp  = malloc(size);
	if (!nbmp)
		return NULL;
	
	if (bmp->width == w && bmp->height == h)
	{
		memcpy(nbmp, bmp, size);
		nbmp->pixels = (void *)((char *)nbmp + ((char *)bmp->pixels - (char *)bmp));
		nbmp->rgba   = (void *)((char *)nbmp + ((char *)bmp->rgba   - (char *)bmp));
		return nbmp;
	}
	nrgba = (struct win_rgba *)(nbmp + 1);
	npix  = (win_color *)(nrgba + w * h);
	orgba = bmp->rgba;
	ow    = bmp->width;
	oh    = bmp->height;
	for (p = nrgba, y = 0; y < h; y++)
		for (x = 0; x < w; x++, p++)
			*p = orgba[x * ow / w + y * oh / h * ow];
	nbmp->pixels = npix;
	nbmp->rgba   = nrgba;
	nbmp->width  = w;
	nbmp->height = h;
	return nbmp;
}
