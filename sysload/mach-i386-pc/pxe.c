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
#include <sysload/console.h>
#include <sysload/disk.h>
#include <sysload/mem.h>
#include <sys/types.h>
#include <errno.h>

#define PXE_IMAGE	"m/image"

static int pxe_read(struct disk *dk, blk_t blk, void *buf);

static struct disk pxe_disk =
{
	.name	= "pxe0",
	.read	= pxe_read,
};

static char *	pxe_bang;
static uint32_t	pxe_entry;
static char *	pxe_data;

static uint32_t	pxe_yip;
static uint32_t	pxe_gip;
static uint32_t	pxe_sip;

static int pxe_get_addrs(void)
{
	char *cpak;
	
	memset(bounce, 0, sizeof bounce);
	*(uint32_t *)(bounce + 2) = 3;
	
	fcp.stack[0]	= 0x71;
	fcp.stack[1]	= (uintptr_t)bounce;
	fcp.stack[2]	= 0;
	fcp.addr	= pxe_entry;
	farcall();
	
	if (fcp.eax & 0xffff)
		return EIO;
	
	cpak = (char *)(uintptr_t)*(uint16_t *)(bounce + 6) + (*(uint16_t *)(bounce + 8) << 4);
	
	pxe_yip = *(uint32_t *)(cpak + 16);
	pxe_sip = *(uint32_t *)(cpak + 20);
	pxe_gip = *(uint32_t *)(cpak + 24);
	
	return 0;
}

static int pxe_file_size(const char *name, long *sz)
{
	if (strlen(name) >= 128)
		return EINVAL;
	
	memset(bounce, 0, sizeof bounce);
	memcpy(bounce +  2, &pxe_sip, 4);
	strcpy(bounce + 10, name);
	
	fcp.stack[0]	= 0x25;
	fcp.stack[1]	= (uintptr_t)bounce;
	fcp.stack[2]	= 0;
	fcp.addr	= pxe_entry;
	farcall();
	
	if (fcp.eax & 0xffff)
		return EIO;
	
	*sz = *(uint32_t *)(bounce + 138);
	return 0;
}

static int pxe_tftp_open(const char *name)
{
	if (strlen(name) >= 128)
		return EINVAL;
	
	memset(bounce, 0, sizeof bounce);
	memcpy(bounce +  2, &pxe_sip, 4);
	memcpy(bounce +  6, &pxe_gip, 4);
	strcpy(bounce + 10, name);
	bounce[138] = 0;
	bounce[139] = 69;
	bounce[140] = 512;
	
	fcp.stack[0]	= 0x20;
	fcp.stack[1]	= (uintptr_t)bounce;
	fcp.stack[2]	= 0;
	fcp.addr	= pxe_entry;
	farcall();
	
	if (fcp.eax & 0xffff)
		return EIO;
	return 0;
}

static int pxe_tftp_close(void)
{
	memset(bounce, 0, sizeof bounce);
	
	fcp.stack[0]	= 0x21;
	fcp.stack[1]	= (uintptr_t)bounce;
	fcp.stack[2]	= 0;
	fcp.addr	= pxe_entry;
	farcall();
	
	if (fcp.eax & 0xffff)
		return EIO;
	return 0;
}

static size_t pxe_tftp_read(void *buf, size_t *sz)
{
	size_t cnt;
	
	memset(bounce, 0, sizeof bounce);
	
	*(uint16_t *)(bounce + 6) = (uintptr_t)(bounce + 16);
	
	fcp.stack[0]	= 0x22;
	fcp.stack[1]	= (uintptr_t)bounce;
	fcp.stack[2]	= 0;
	fcp.addr	= pxe_entry;
	farcall();
	
	if (fcp.eax & 0xffff)
		return EIO;
	
	cnt = *(uint16_t *)(bounce + 4);
	if (cnt > *sz)
		cnt = *sz;
	memcpy(buf, bounce + 16, cnt);
	*sz = cnt;
	return 0;
}

static int pxe_load_file_r(const char *name, void *buf, uint32_t size)
{
	uint32_t resid = size;
	char *p = buf;
	size_t cnt;
	int err;
	
	err = pxe_tftp_open(name);
	if (err)
		return err;
	
	while (resid)
	{
		cnt = resid;
		err = pxe_tftp_read(p, &cnt);
		if (err)
		{
			pxe_tftp_close();
			return err;
		}
		resid -= cnt;
		p += cnt;
		if (!cnt)
			break;
	}
	
	return pxe_tftp_close();
}

/* static int pxe_load_file(const char *name, void *buf, uint32_t size)
{
	memset(buf, 0xcc, size);
	
	memset(bounce, 0, sizeof bounce);
	strcpy(bounce +   2, name);
	memcpy(bounce + 130, &size, 4);
	memcpy(bounce + 134, &buf,  4);
	memcpy(bounce + 138, (uint32_t []){ PXE_SERVER  }, 4);
	memcpy(bounce + 142, (uint32_t []){ PXE_GATEWAY }, 4);
	memcpy(bounce + 146, (uint32_t []){ PXE_MCAST   }, 4);
	bounce[150] = 8;
	bounce[151] = 8;
	bounce[153] = 69;
	bounce[154] = bounce[156] = 2;
	
	fcp.stack[0] = 0x23;
	fcp.stack[1] = bounce;
	fcp.stack[2] = 0;
	farcall();
	
	if (fcp.eax & 0xffff)
		return EIO;
	
	if (bounce[0] || bounce[1])
	{
		con_gotoxy(0, 3);
		con_putx8(bounce[1]);
		con_putx8(bounce[0]);
		for (;;);
	}
	
	return 0;
} */

static int pxe_read(struct disk *dk, blk_t blk, void *buf)
{
	if (blk >= dk->size)
		return EIO;
	
	memcpy(buf, pxe_data + blk * 512, 512);
	return 0;
}

void pxe_init(void)
{
	long sz;
	long t;
	
	if (!bangpxe)
		return;
	pxe_bang  = (char *)(uintptr_t)(bangpxe & 0xffff);
	pxe_bang += ((bangpxe >> 12) & 0xffff0);
	pxe_entry = *(uint32_t *)(pxe_bang + 16);
	
	con_status("Initializing PXE...", "");
	
	if (pxe_get_addrs())
		return;
	
	if (pxe_file_size(PXE_IMAGE, &sz))
		return;
	if (sz <= 0)
		return;
	pxe_disk.size = sz / 512;
	
	pxe_data = mem_alloc(sz, MA_SYSLOAD);
	if (pxe_data == NULL)
		return;
	
	con_status("Downloading " PXE_IMAGE "...", "");
	
	if (pxe_load_file_r(PXE_IMAGE, pxe_data, sz))
		fail("pxe_load_file failed"); // return;
	
	pxe_disk.next = disk_list;
	disk_list = &pxe_disk;
	disk_boot = &pxe_disk;
	
	con_status("", "");
}
