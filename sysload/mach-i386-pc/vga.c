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

#include <sysload/i386-pc.h>
#include <sysload/kparam.h>
#include <sysload/flags.h>
#include <sysload/main.h>
#include <kern/errno.h>

#define MODE_MAX	64

static struct mode
{
	struct kfb info;
	int nr;
} vesa_modes[MODE_MAX];

static struct kfbmode kfbmodes[MODE_MAX];

static int vesa_mode = -1;
static int vesa_mcnt = 0;

static struct kfb vga_fb;

static int vesa_trymode(struct mode *m)
{
	unsigned attr;
	
	bcp.eax	 = 0x4f01;
	bcp.ecx	 = m->nr;
	bcp.edi	 = (int)bounce;
	bcp.es	 = 0;
	bcp.intr = 0x10;
	bioscall();
	if ((bcp.eax & 0xffff) != 0x004f)
		return EINVAL;
	
	attr = *(uint16_t *)bounce;
	if ((attr & 0x93) != 0x93)
		return EINVAL;
	
	m->info.xres		= *(uint16_t *)(bounce + 0x12);
	m->info.yres		= *(uint16_t *)(bounce + 0x14);
	m->info.bpp		= *(uint8_t  *)(bounce + 0x19);
	m->info.bytes_per_line	= *(uint16_t *)(bounce + 0x10);
	m->info.type		= KFB_GRAPH;
	m->info.base		= *(uint32_t *)(bounce + 0x28);
	
	return 0;
}

static int vesa_setmode(struct mode *m)
{
	bcp.intr = 0x10;
	bcp.eax = 0x4f02;
	bcp.ebx = m->nr;
	bcp.es  = (unsigned)bounce >> 4;
	bcp.edi = (unsigned)bounce & 0x000f;
	bioscall();
	if (bcp.eax != 0x004f)
		return EINVAL;
	
	vga_fb = m->info;
	return 0;
}

static void vesa_scan(void)
{
	int mi = 0;
	int bpp;
	int i;
	
	for (i = 0x4100; i < 0x4200 && vesa_mcnt < MODE_MAX; i++)
	{
		vesa_modes[vesa_mcnt].nr = i;
		
		if (vesa_trymode(&vesa_modes[vesa_mcnt]))
			continue;
		
		bpp = vesa_modes[vesa_mcnt].info.bpp;
		if (bpp != 32 && bpp != 8)
			continue;
		
		kfbmodes[vesa_mcnt].xres = vesa_modes[vesa_mcnt].info.xres;
		kfbmodes[vesa_mcnt].yres = vesa_modes[vesa_mcnt].info.yres;
		kfbmodes[vesa_mcnt].bpp  = vesa_modes[vesa_mcnt].info.bpp;
		kfbmodes[vesa_mcnt].type = KFB_GRAPH;
		
		vesa_mcnt++;
	}
}

static int vesa_find_and_set(int xres, int yres, int bpp)
{
	struct mode *m;
	int i;
	
	for (i = 0; i < vesa_mcnt; i++)
	{
		m = &vesa_modes[i];
		
		if (m->info.xres == xres && m->info.yres == yres && m->info.bpp == bpp)
		{
			vesa_setmode(m);
			return 0;
		}
	}
	return ENODEV;
}

struct kfb *vga_init(void)
{
	uint16_t *p, *e;
	
	if (!(boot_params.boot_flags & BOOT_TEXT))
	{
		vesa_scan();
		
		if (!vesa_find_and_set(bootfb_xres, bootfb_yres, bootfb_bpp))
			goto fini;
	}
	
	bcp.eax	 = 0x0f00;
	bcp.intr = 0x10;
	bioscall();
	
	if ((bcp.eax & 255) != 3)
	{
		bcp.eax	 = 0x0003;
		bioscall();
	}
	
	bcp.eax	 = 0x0100;
	bcp.ecx	 = 0x2000;
	bioscall();
	
	vga_fb.xres    = 80;
	vga_fb.yres    = 25;
	vga_fb.bpp     = 2;
	vga_fb.type    = KFB_TEXT;
	vga_fb.base    = 0xb8000;
	vga_fb.io_base = 0x3c0;
fini:
	vga_fb.modes	= kfbmodes;
	vga_fb.mode_cnt	= vesa_mcnt;
	
	return &vga_fb;
}
