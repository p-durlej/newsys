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

#include <priv/natfs.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

#define BLK_SIZE		512

static blk_t	reserved_count;
static blk_t	block_count;
static char *	dev_path;
static int	fd;

static void mkroot(struct nat_super *sb)
{
	struct nat_header root_hd;
	
	memset(&root_hd, 0, sizeof root_hd);
	
	/* hd.ctime = time(NULL);
	hd.atime = time(NULL);
	hd.mtime = time(NULL); */
	
	root_hd.size	  = 0;
	root_hd.mode	  = 030755;
	root_hd.nlink	  = 1;
	root_hd.blocks	  = 1;
	root_hd.ndirblks  = 114;
	root_hd.nindirlev = 1;
	
	lseek(fd, (long)sb->root_block * (long)BLK_SIZE, SEEK_SET);
	if (write(fd, &root_hd, sizeof root_hd) != sizeof root_hd)
	{
		fputc('\n', stderr);
		perror(dev_path);
		exit(errno);
	}
}

static void newbam(struct nat_super *sb)
{
	char zero[BLK_SIZE];
	blk_t edata;
	int bisplit, bysplit;
	int dbamb;
	int ebamb;
	int rbamb;
	int bami;
	int i;
	
	lseek(fd, sb->bam_block * BLK_SIZE, SEEK_SET);
	memset(zero, 255, BLK_SIZE);
	
	edata = sb->data_block + sb->data_size;
	rbamb = sb->root_block / BLK_SIZE / 8;
	dbamb = sb->data_block / BLK_SIZE / 8;
	ebamb = edata	       / BLK_SIZE / 8;
	bami  = 0;
	
	for (i = 0; i < sb->bam_size; i++)
	{
		if (i == dbamb)
		{
			bisplit = sb->data_block % (BLK_SIZE * 8);
			bysplit = bisplit / 8;
			
			memset(zero + bysplit, 0, sizeof zero - bysplit);
			zero[bysplit] = 255 >> (8 - (sb->data_block & 7));
		}
		else if (i == dbamb + 1)
			bzero(zero, sizeof zero);
		
		if (i == ebamb)
		{
			bisplit = edata % (BLK_SIZE * 8);
			bysplit = bisplit / 8;
			
			memset(zero + bysplit, 255, sizeof zero - bysplit);
			zero[bysplit] = ~(255 >> (8 - (edata & 7)));
		}
		else if (i == ebamb + 1)
			memset(zero, 255, sizeof zero);
		
		if (i == rbamb)
		{
			bami = (sb->root_block / 8) % BLK_SIZE;
			zero[bami] |= 1 << (sb->root_block & 7);
		}
		
		if (write(fd, zero, BLK_SIZE) != BLK_SIZE)
			err(errno, "%s: write", dev_path);
		
		if (i == rbamb)
			zero[bami] &= ~(1 << (sb->root_block & 7));
	}
}

static int mkfs(void)
{
	struct nat_super sb;
	
	warnx("opening %s", dev_path);
	fd = open(dev_path, O_RDWR);
	if (fd < 0)
		err(errno, "mkfs: %s: open", dev_path);
	
	bzero(&sb, sizeof sb);
	memcpy(sb.magic, NAT_MAGIC, 8);
	sb.bam_block	= reserved_count;
	sb.bam_size	= (block_count + 8 * BLK_SIZE - 1) / BLK_SIZE / 8;
	sb.data_block	= sb.bam_block + sb.bam_size;
	sb.data_size	= block_count - sb.data_block;
	sb.root_block	= sb.data_block;
	sb.ndirblks	= 110;
	sb.nindirlev	= 3;
	
	warnx("writing superblock");
	
	lseek(fd, BLK_SIZE, SEEK_SET);
	if (write(fd, &sb, sizeof sb) != sizeof sb)
		err(errno, "%s: write", dev_path);
	
	warnx("creating root dir");
	mkroot(&sb);
	
	warnx("initializing BAM");
	newbam(&sb);
	
	warnx("closing device");
	close(fd);
	
	warnx("command completed");
	return 0;
}

static void usage(void)
{
	fprintf(stderr, "usage:\n\n"
			" mkfs DEVICE NBLOCKS [RESERVED]\n");
	exit(255);
}

int main(int argc, char **argv)
{
	if (argc > 4 || argc < 3)
		usage();
	
	dev_path = argv[1];
	block_count = strtoul(argv[2], NULL, 0);
	
	if (argc >= 4)
		reserved_count = strtoul(argv[3], NULL, 0);
	else
		reserved_count = 2;
	
	mkfs();
	return 0;
}
