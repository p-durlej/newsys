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

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <paths.h>
#include <bioctl.h>
#include <err.h>

#define UNIX	0

static struct part
{
	unsigned long	start;
	unsigned long	end;
	unsigned	flags;
	unsigned	type;
} part[4];
static int part_cnt;

static unsigned long	blk_count;
static int		nr_cyl  = 1024;
static int		nr_head = 255;
static int		nr_sec  = 63;

static char *		disk_name;
static unsigned char	mbr[512];
#if !UNIX
static struct bio_info	disk_info;
#endif

static void load_mbr(void)
{
#if UNIX
	struct stat st;
#endif
	int cnt;
	int fd;
	int i;
	
#if UNIX
	fd = open(disk_name, O_RDONLY);
	if (fd < 0)
	{
		perror(disk_name);
		exit(errno);
	}
	fstat(fd, &st);
	blk_count = st.st_size / 512;
#else
	fd = open(disk_name, O_RDWR);
	if (fd < 0)
	{
		perror(disk_name);
		exit(errno);
	}
	if (ioctl(fd, BIO_INFO, &disk_info))
	{
		perror(disk_name);
		exit(errno);
	}
	nr_cyl	  = disk_info.ncyl;
	nr_head	  = disk_info.nhead;
	nr_sec	  = disk_info.nsect;
	blk_count = disk_info.size;
	if (!nr_cyl || !nr_head || !nr_sec)
	{
		fprintf(stderr, "%s: using fake geometry\n", disk_name);
		nr_head = 255;
		nr_sec  = 63;
		nr_cyl  = blk_count / nr_head / nr_sec;
	}
#endif
	
	cnt = read(fd, mbr, sizeof mbr);
	if (cnt < 0)
	{
		perror(disk_name);
		exit(errno);
	}
	if (cnt != 512)
	{
		fprintf(stderr, "%s: Short read\n", disk_name);
		exit(255);
	}
	close(fd);
	
	part_cnt = 0;
	memset(&part, 0, sizeof part);
	for (i = 0; i < 64; i += 16)
		if (mbr[0x1be + i + 4])
		{
			part[part_cnt].start  = mbr[0x1be + i +  8];
			part[part_cnt].start |= mbr[0x1be + i +  9] << 8;
			part[part_cnt].start |= mbr[0x1be + i + 10] << 16;
			part[part_cnt].start |= mbr[0x1be + i + 11] << 24;
			
			part[part_cnt].end  = mbr[0x1be + i + 12];
			part[part_cnt].end |= mbr[0x1be + i + 13] << 8;
			part[part_cnt].end |= mbr[0x1be + i + 14] << 16;
			part[part_cnt].end |= mbr[0x1be + i + 15] << 24;
			part[part_cnt].end += part[part_cnt].start;
			
			part[part_cnt].flags = mbr[0x1be + i];
			part[part_cnt].type  = mbr[0x1be + i + 4];
			part_cnt++;
		}
}

static void makecchs(unsigned long da, unsigned *cp, unsigned *hp, unsigned *sp)
{
	unsigned long c, h, s, t;
	
	s = da % nr_sec;
	t = da / nr_sec;
	h = t % nr_head;
	c = t / nr_head;
	if (c > 65535)
	{
		c = 65535;
		h = nr_head - 1;
		s = nr_sec  - 1;
	}
	*cp = c;
	*hp = h;
	*sp = s;
}

static void save_mbr(void)
{
	int fd;
		int i;
	
	memset(&mbr[0x1be], 0, 64);
	for (i = 0; i < part_cnt; i++)
	{
		unsigned c, h, s;
		
		mbr[0x1be + i * 16     ] = part[i].flags;
		mbr[0x1be + i * 16 +  4] = part[i].type;
		
		mbr[0x1be + i * 16 +  8] = part[i].start;
		mbr[0x1be + i * 16 +  9] = part[i].start >> 8;
		mbr[0x1be + i * 16 + 10] = part[i].start >> 16;
		mbr[0x1be + i * 16 + 11] = part[i].start >> 24;
		
		mbr[0x1be + i * 16 + 12] =  part[i].end - part[i].start;
		mbr[0x1be + i * 16 + 13] = (part[i].end - part[i].start) >> 8;
		mbr[0x1be + i * 16 + 14] = (part[i].end - part[i].start) >> 16;
		mbr[0x1be + i * 16 + 15] = (part[i].end - part[i].start) >> 24;
		
		makecchs(part[i].start, &c, &h, &s);
		mbr[0x1be + i * 16 + 1]  = h;
		mbr[0x1be + i * 16 + 2]  = s + 1;
		mbr[0x1be + i * 16 + 2] |= (c >> 2) & 0xc0;
		mbr[0x1be + i * 16 + 3]  = c;
		
		makecchs(part[i].end - 1, &c, &h, &s);
		mbr[0x1be + i * 16 + 5]  = h;
		mbr[0x1be + i * 16 + 6]  = s + 1;
		mbr[0x1be + i * 16 + 6] |= (c >> 2) & 0xc0;
		mbr[0x1be + i * 16 + 7]  = c;
	}
	
	fd = open(disk_name, O_WRONLY);
	if (fd < 0)
	{
		perror(disk_name);
		return;
	}
	if (write(fd, mbr, sizeof mbr) != sizeof mbr)
	{
		perror(disk_name);
		return;
	}
	close(fd);
	printf("Written, reboot needed.\n");
}

static void print_part(struct part *p)
{
	if (!p->type)
		return;
	
	printf("%2i ", (int)(p - part));
	if (p->flags == 128)
		printf("*    ");
	else if (p->flags)
		printf("0x%02x ", p->flags);
	else
		printf("     ");
	printf("0x%02x %-10li %-10li\n", p->type, p->start, p->end);
}

static void print_tab(void)
{
	size_t len;
	size_t i;
	
	printf("\nDisk %s: %i cylinders, %i heads, %i sectors,\n", disk_name, nr_cyl, nr_head, nr_sec);
	len = strlen(disk_name);
	for (i = 0; i < len; i++)
		putchar(' ');
	printf("       %li blocks, %li MB.\n\n", blk_count, blk_count / 2048);
	
	printf("Nr Boot Type Start      End\n");
	for (i = 0; i < part_cnt; i++)
		print_part(&part[i]);
	fputc('\n', stdout);
}

static int read_ulong(const char *prompt, unsigned long min, unsigned long max,  unsigned long *vp)
{
	unsigned long v;
	char buf[16];
	char *p;
	int cnt;
	int ov;
	
	for (;;)
	{
		fputs(prompt, stdout);
		fflush(stdout);
		
		cnt = read(0, buf, sizeof buf - 1);
		if (cnt < 0)
		{
			perror("stdin");
			return -1;
		}
		if (!cnt)
			return -1;
		buf[cnt] = 0;
		
		ov = 0;
		for (p = buf; isspace(*p); p++);
		for (; *p == '!'; p++)
			ov = 1;
		v = strtoul(p, &p, 0);
		
		if (*p && *p != '\n')
			continue;
		if (!ov && (v < min || v > max))
			continue;
		break;
	}
	
	*vp = v;
	return 0;
}

static int read_end(const char *prompt, unsigned long start, unsigned long *vp)
{
	unsigned long v;
	char buf[16];
	char *p;
	int cnt;
	int rel;
	int ov;
	
retry:
	for (;;)
	{
		fputs(prompt, stdout);
		fflush(stdout);
		
		cnt = read(0, buf, sizeof buf - 1);
		if (cnt < 0)
		{
			perror("stdin");
			return -1;
		}
		if (!cnt)
			return -1;
		buf[cnt] = 0;
		
		rel = ov = 0;
		for (p = buf; isspace(*p) || *p == '!' || *p == '+'; p++)
			switch (*p)
			{
			case '!':
				ov = 1;
				continue;
			case '+':
				rel = 1;
				continue;
			}
		v = strtoul(p, &p, 0);
		
		while (*p)
			switch (*p)
			{
			case '\n':
				p++;
				break;
			case 'K':
				v <<= 1;
				p++;
				break;
			case 'M':
				v <<= 11;
				p++;
				break;
			case 'G':
				v <<= 21;
				p++;
				break;
			default:
				goto retry;
			}
		
		if (*p)
			continue;
		
		if (rel)
			v += start;
		
		if (!ov && v > blk_count)
			continue;
		break;
	}
	
	*vp = v;
	return 0;
}

static int read_uint(const char *prompt, unsigned min, unsigned max, unsigned *vp)
{
	unsigned long v;
	
	do
		if (read_ulong(prompt, min, max, &v))
			return -1;
	while ((unsigned)v != v);
	
	*vp = v;
	return 0;
}

static int read_yn(const char *prompt, int *vp)
{
	char buf[16];
	char *p;
	int cnt;
	
	for (;;)
	{
		fputs(prompt, stdout);
		fflush(stdout);
		
		cnt = read(0, buf, sizeof buf - 1);
		if (cnt < 0)
		{
			perror("stdin");
			return -1;
		}
		if (!cnt)
			return -1;
		buf[cnt] = 0;
		
		p = strchr(buf, '\n');
		if (p)
			*p = 0;
		
		if (!strcmp(buf, "y") || !strcmp(buf, "yes"))
		{
			*vp = 1;
			return 0;
		}
		if (!strcmp(buf, "n") || !strcmp(buf, "no"))
		{
			*vp = 0;
			return 0;
		}
	}
}

static void delete_part(void)
{
	unsigned long i;
	
	if (read_ulong("Partition number: ", 0, 3, &i) || i < 0 || i >= part_cnt)
		return;
	
	memmove(&part[i], &part[i + 1], (part_cnt - i - 1) * sizeof *part);
	part_cnt--;
	printf("Deleted.\n");
}

static void check_start(unsigned long start)
{
	int i;
	
	if (start >= blk_count)
		return;
	
	for (i = 0; i < part_cnt; i++)
		if (start >= part[i].start && start < part[i].end)
			return;
	
	printf("%li ", start);
}

static void add_part(void)
{
	unsigned long start, end, type, flags;
	unsigned long end_hint;
	int i;
	
	if (part_cnt >= 4)
	{
		fprintf(stderr, "Maximum of 4 partitons reached.\n");
		return;
	}
	
	printf("Hints: ");
	check_start(64);
	for (i = 0; i < part_cnt; i++)
		check_start(part[i].end);
	printf("\n");
	if (read_ulong("  Start: ", 1, blk_count - 1, &start))
		return;
	
	end_hint = blk_count;
	for (i = 0; i < part_cnt; i++)
		if (part[i].start >= start && part[i].start < end_hint)
			end_hint = part[i].start;
	printf("Hints: %li, +size, +sizeK, +sizeM, +sizeG\n", end_hint);
	if (read_end("  End: ", start, &end))
		return;
	
	printf("Hint: 0xcc\n");
	if (read_ulong("  Type: ", 1, 255, &type))
		return;
	
	printf("Hints: 0 (inactive), 128 (active)\n");
	fflush(stdout);
	read_ulong("  Flags: ", 0, 255, &flags);
	
	printf("Created.\n");
	part[part_cnt].start = start;
	part[part_cnt].end   = end;
	part[part_cnt].type  = type;
	part[part_cnt].flags = flags;
	part_cnt++;
}

static void set_type(void)
{
	unsigned type;
	unsigned i;
	
	if (read_uint("Partition number: ", 0, 3, &i) || i < 0 || i >= part_cnt)
		return;
	if (read_uint("Type: ", 1, 255, &type))
		return;
	
	part[i].type = type;
	printf("Changed.\n");
}

static void set_flags(void)
{
	unsigned flags;
	unsigned i, n;
	char msg[64];
	int b;
	
	if (read_uint("Partition number: ", 0, 3, &i) || i < 0 || i >= part_cnt)
		return;
	if (read_uint("Flags: ", 0, 255, &flags))
		return;
	if (flags & 128)
		for (n = 0; n < part_cnt; n++)
		{
			if (i == n || !part[n].flags)
				continue;
			
			sprintf(msg, "Deactivate %i? ", n);
			if (!read_yn(msg, &b) && b)
				part[n].flags = 0;
		}
	part[i].flags = flags;
	printf("Changed.\n");
}

static void install_ipl(void)
{
	int cnt;
	int fd;
	
	fd = open(_PATH_I386_IPL, O_RDONLY);
	if (fd < 0)
	{
		perror(_PATH_I386_IPL);
		return;
	}
	if (cnt = read(fd, mbr, 448), cnt < 0)
		perror(_PATH_I386_IPL);
	close(fd);
	mbr[510] = 0x55;
	mbr[511] = 0xaa;
	printf("IPL installed.\n");
}

static void init_disk(void)
{
	if (part_cnt)
	{
		puts("Partitions already exist.");
		return;
	}
	
	part[0].start = 64;
	part[0].end   = blk_count - 64;
	part[0].type  = 0xcc;
	part[0].flags = 0x80;
	part_cnt = 1;
	
	puts("Partition created.");
	
	install_ipl();
	save_mbr();
}

static void help(void)
{
	puts("a  new partition");
	puts("b  partition flags");
	puts("B  install IPL");
	puts("d  delete partition");
	puts("i  initialize with defaults");
	puts("p  partition list");
	puts("q  quit");
	puts("t  partition type");
	puts("w  write table");
}

int main(int argc, char **argv)
{
	char buf[80];
	char *p;
	int iflag = 0;
	int cnt;
	int c;
	
	if (!strcmp(argv[1], "--help"))
	{
		fprintf(stderr, "Usage: fdisk DEVICE\n\n"
				"View and edit disk partitions.\n\n");
		return 0;
	}
	
	while (c = getopt(argc, argv, "i"), c > 0)
		switch (c)
		{
		case 'i':
			iflag = 1;
			break;
		default:
			return 1;
		};
	
	argv += optind;
	argc -= optind;
	
	if (argc != 1)
	{
		fprintf(stderr, "fdisk: wrong number of arguments\n");
		return 255;
	}
	
	disk_name = argv[0];
	load_mbr();
	
	print_tab();
	
	if (iflag)
	{
		init_disk();
		return 0;
	}
	
	if (part_cnt)
		puts("Type 'h' for help.");
	else
		puts("Type 'h' for help or type 'i' to initialize the disk.");
	
	for (;;)
	{
		write(1, "fdisk> ", 7);
		
		cnt = read(0, buf, sizeof buf);
		if (cnt < 0)
		{
			perror("stdin");
			return errno;
		}
		if (!cnt)
			continue;
		p = strchr(buf, '\n');
		if (p)
			*p = 0;
		if (!*buf)
			continue;
		
		switch (*buf)
		{
		case 'B':
			install_ipl();
			break;
		case 'b':
			set_flags();
			break;
		case 't':
			set_type();
			break;
		case 'a':
			add_part();
			break;
		case 'd':
			delete_part();
			break;
		case 'p':
			print_tab();
			break;
		case 'w':
			save_mbr();
			break;
		case 'i':
			init_disk();
			break;
		case 'h':
		case '?':
			help();
			break;
		case 'q':
			return 0;
		default:
			fprintf(stderr, "%c: invalid command\n", *buf);
		}
	}
}
