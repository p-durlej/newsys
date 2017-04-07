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

#include <priv/natfs.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <mount.h>
#include <stdint.h>
#include <err.h>

#define BLK_SIZE		512

#define NAT_FAT_FREE		0
#define NAT_FAT_EOF		(-1)
#define NAT_FAT_SKIP		(-2)
#define NAT_FAT_BAD		(-3)

#define NAT_NAME_MAX		27
#define NAT_FILE_SIZE_MAX	(0x7fffffff)
#define NAT_DIR_SIZE_MAX	(0x00100000)

#define NAT_D_PER_BLOCK		(BLK_SIZE / sizeof(struct nat_dirent))

#define ICHECK_MAX		16

static struct _mtab *	mount;

static char *		dev_name;
static int		dev_fd;

static unsigned		icheck[ICHECK_MAX];
static int		icheck_cnt;

static int		enable_write;
static int		show_names;
static int		qflag;
static int		fix;

static uint8_t *	rbam;

static struct nat_super	sb;

static struct file
{
	struct	 nat_header hd;
	uint32_t first_block;
	uint32_t true_nlink;
	int	 checked;
} *file;

static int file_count;

static void	load_super(void);
static struct file *
		load_hdr(uint32_t first_block);
static void	save_hdr(struct file *file);
static uint32_t	check_bmap(struct file *file);
static void	check_file(unsigned first_block, int depth, char *name);
static void	check_dir(unsigned first_block, int depth, char *name);
static void	check_nlink(void);
static void	check_bam(void);
static void	find_mount(void);
static void	exit_fsck(int status);

static int blk_in_range(uint32_t bn)
{
	if (bn < sb.data_block)
		return 0;
	if (bn >= sb.data_block + sb.data_size)
		return 0;
	return 1;
}

static void bused(uint32_t bn)
{
	rbam[bn / 8] |= 1 << (bn & 7);
}

static int bdupref(uint32_t bn)
{
	if ((rbam[bn / 8] >> (bn & 7)) & 1)
		return 1;
	bused(bn);
	return 0;
}

static void load_super(void)
{
	size_t bdai, edai;
	uint32_t edata;
	size_t rbamsz;
	
	lseek(dev_fd, BLK_SIZE, SEEK_SET);
	errno = EINVAL;
	if (read(dev_fd, &sb, sizeof sb) != sizeof sb)
	{
		warn("reading superblock");
		exit_fsck(errno);
	}
	if (memcmp(sb.magic, NAT_MAGIC, sizeof NAT_MAGIC))
	{
		warnx("superblock: bad magic");
		if (fix)
		{
			memcpy(sb.magic, "OS386FS", 8);
			lseek(dev_fd, BLK_SIZE, SEEK_SET);
			if (write(dev_fd, &sb, sizeof sb) != sizeof sb)
			{
				warn("writing superblock");
				exit_fsck(errno);
			}
			warnx("fixed");
		}
		else
			exit_fsck(255);
	}
	
	rbamsz = sb.bam_size * BLK_SIZE;
	
	rbam = calloc(rbamsz, 1);
	if (!rbam)
	{
		warn("malloc");
		exit_fsck(errno);
	}
	
	edata = sb.data_block + sb.data_size;
	bdai  = sb.data_block / 8;
	edai  = edata / 8;
	
	if (bdai >= rbamsz || edai > rbamsz)
	{
		warnx("bdai = %u, edai = %u, rbamsz = %u", bdai, edai, rbamsz);
		warnx("superblock inconsistent");
		exit_fsck(255);
	}
	
	memset(rbam,	    255, bdai);
	memset(rbam + edai, 255, rbamsz - edai);
	
	if (sb.data_block & 7)
		rbam[bdai] =   255 >> (8 - (sb.data_block & 7));
	if (edata & 7)
		rbam[edai] = ~(255 >> (8 - (edata	  & 7)));
}

static struct file *load_hdr(uint32_t first_block)
{
	int cnt;
	int i;
	
	for (i = 0; i < file_count; i++)
		if (file[i].first_block == first_block)
			return &file[i];
	
	file_count++;
	file = realloc(file, file_count * sizeof(struct file));
	if (!file)
	{
		warn("realloc");
		exit_fsck(errno);
	}
	
	lseek(dev_fd, first_block * BLK_SIZE, SEEK_SET);
	cnt = read(dev_fd, &file[file_count - 1].hd, sizeof(struct nat_header));
	if (cnt < 0)
	{
		warn("reading header of %i", first_block);
		exit_fsck(errno);
	}
	file[file_count - 1].first_block = first_block;
	file[file_count - 1].true_nlink  = 0;
	file[file_count - 1].checked     = 0;
	return &file[file_count - 1];
}

static void save_hdr(struct file *file)
{
	lseek(dev_fd, file->first_block * BLK_SIZE, SEEK_SET);
	if (write(dev_fd, &file->hd, sizeof file->hd) != sizeof file->hd)
	{
		warn("writing header of %i", file->first_block);
		exit_fsck(errno);
	}
}

static uint32_t check_imap(uint32_t mbn, int indl)
{
	uint32_t imap[BLK_SIZE / 4];
	uint32_t bcnt = 0;
	uint32_t bn;
	int dirty;
	int i;
	
	lseek(dev_fd, mbn * BLK_SIZE, SEEK_SET);
	if (read(dev_fd, imap, sizeof imap) < 0)
	{
		warn("reading block %i", (int)mbn);
		exit_fsck(errno);
	}
	
	for (i = 0; i < BLK_SIZE / 4; i++)
	{
		bn = imap[i];
		if (!bn)
			continue;
		
		if (!blk_in_range(bn))
		{
			warnx("incorrect bn %lu in level %i map %lu",
				(unsigned long)bn, indl,
				(unsigned long)mbn);
			if (fix)
			{
				imap[i] = 0;
				dirty = 1;
				warnx("punched a hole");
				continue;
			}
		}
		
		if (bdupref(bn))
		{
			warnx("block %lu in level %i map %lu is referenced elsewhere",
				(unsigned long)bn, indl,
				(unsigned long)file->first_block);
			if (fix)
			{
				imap[i] = 0;
				warnx("punched a hole");
			}
			dirty = 1;
			continue;
		}
		
		bcnt++;
		if (!indl)
			continue;
		bcnt += check_imap(bn, indl - 1);
	}
	
	if (dirty)
	{
		lseek(dev_fd, mbn * BLK_SIZE, SEEK_SET);
		if (write(dev_fd, imap, sizeof imap) < 0)
		{
			warn("writing block %i", (int)mbn);
			exit_fsck(errno);
		}
	}
	return bcnt;
}

static uint32_t check_bmap(struct file *file)
{
	uint32_t bcnt = 1;
	uint32_t bn;
	int i;
	
	for (i = 0; i < sizeof file->hd.bmap / 4; i++)
	{
		bn = file->hd.bmap[i];
		if (!bn)
			continue;
		if (!blk_in_range(bn))
		{
			warnx("incorrect bn %lu in file %lu",
				(unsigned long)bn,
				(unsigned long)file->first_block);
			if (fix)
			{
				file->hd.bmap[i] = 0;
				warnx("punched a hole");
				save_hdr(file);
				continue;
			}
		}
		if (bdupref(bn))
		{
			warnx("block %lu in file %lu is referenced elsewhere",
				(unsigned long)bn,
				(unsigned long)file->first_block);
			if (fix)
			{
				file->hd.bmap[i] = 0;
				save_hdr(file);
				warnx("punched a hole");
			}
			continue;
		}
		bcnt++;
		
		if (i < file->hd.ndirblks)
			continue;
		bcnt += check_imap(bn, file->hd.nindirlev - 1);
	}
	return bcnt;
}

static void check_file(unsigned first_block, int depth, char *name)
{
	struct file *file;
	uint32_t size;
	int i;
	
	if (show_names)
	{
		for (i = 0; i < depth; i++)
			fputc(' ', stderr);
		fprintf(stderr, "%s (%i)\n", name, first_block);
	}
	
	for (i = 0; i < icheck_cnt; i++)
		if (icheck[i] == first_block)
		{
			for (i = 0; i < depth; i++)
				fputc(' ', stderr);
			fprintf(stderr, "%s (%i)\n", name, first_block);
		}
	
	file = load_hdr(first_block);
	file->true_nlink++;
	
	if (file->checked)
		return;
	file->checked = 1;
	
	if (file->hd.nindirlev < 1 || file->hd.nindirlev > 8)
	{
		warnx("%lu bad block map indirection level %i -- cannot fix", first_block, (int)file->hd.nindirlev);
		return;
	}
	
	if (bdupref(first_block))
	{
		warnx("%lu shared with a data block -- cannot fix", first_block);
		return;
	}
	
	switch (file->hd.mode & 070000)
	{
	case 030000:
		check_dir(first_block, depth, name);
		file = load_hdr(first_block);
		return;
	case 040000:
	case 020000:
	case 010000:
		return;
	case 0:
		break;
	default:
		warnx("%li has incorrect file type in mode field", (long)first_block);
		if (fix)
		{
			file->hd.mode &= ~070000;
			save_hdr(file);
			warnx("converted to regular file");
			break;
		}
		/* return; */
	}
	
	size = check_bmap(file);
	if (file->hd.blocks != size)
	{
		warnx("%i has incorrect block count (is %i, should be %i)", first_block, file->hd.blocks, size);
		if (fix)
		{
			file->hd.blocks = size;
			save_hdr(file);
			warnx("fixed");
		}
	}
}

static uint32_t bmap(struct file *f, uint32_t log)
{
	uint32_t imap[128];
	uint32_t bn;
	uint32_t i;
	int shift;
	
	if (log < f->hd.ndirblks)
	{
		if (log >= sizeof f->hd.bmap / sizeof *f->hd.bmap)
			goto too_big;
		return f->hd.bmap[log];
	}
	
	shift = 7 * f->hd.nindirlev;
	i     = f->hd.ndirblks + (log >> shift);
	if (i >= sizeof f->hd.bmap / sizeof *f->hd.bmap)
		goto too_big;
	bn = f->hd.bmap[i];
	shift -= 7;
	
	while (bn && shift >= 0)
	{
		lseek(dev_fd, bn * BLK_SIZE, SEEK_SET);
		if (read(dev_fd, imap, sizeof imap) < 0)
		{
			warnx("error reading block %lu",
				(unsigned long)bn);
			exit_fsck(errno);
		}
		
		bn = imap[(bn >> shift) & 127];
		shift -= 7;
	}
	return bn;
too_big:
	warnx("log bn %lu too big, file %lu",
		(unsigned long)log, (unsigned long)file->first_block);
	exit_fsck(errno);
	return 0;
}

static void check_dir(unsigned first_block, int depth, char *name)
{
	struct nat_dirent dir[NAT_D_PER_BLOCK];
	struct file *file;
	uint32_t log, phys, cnt;
	uint32_t size;
	
	file = load_hdr(first_block);
	size = check_bmap(file);
	
	if (file->hd.blocks != size)
	{
		warnx("%i has incorrect block count", first_block);
		if (fix)
		{
			file->hd.blocks = size;
			save_hdr(file);
			warnx("fixed");
		}
	}
	
	cnt = (file->hd.size + BLK_SIZE - 1) / BLK_SIZE;
	for (log = 0; log < cnt; log++)
	{
		int i;
		
		phys = bmap(file, log);
		if (!phys)
			continue;
		
		lseek(dev_fd, phys * BLK_SIZE, SEEK_SET);
		errno = EINVAL;
		if (read(dev_fd, &dir, sizeof dir) != sizeof dir)
		{
			warn("reading directory");
			exit_fsck(errno);
		}
		
		for (i = 0; i < NAT_D_PER_BLOCK; i++)
		{
			if (!dir[i].first_block)
				continue;
			
			if (dir[i].name[NAT_NAME_MAX])
			{
				warnx("directory entry name too long");
				if (fix)
				{
					dir[i].name[NAT_NAME_MAX] = 0;
					
					lseek(dev_fd, phys * BLK_SIZE, SEEK_SET);
					if (write(dev_fd, &dir, sizeof dir) < 0)
					{
						warn("writing directory");
						exit_fsck(errno);
					}
					warnx("name truncated");
				}
			}
			
			if (strchr(dir[i].name, '/'))
			{
				char *p;
				
				warnx("slash character in file name");
				if (fix)
				{
					while ((p = strchr(dir[i].name, '/')))
						*p = '_';
					lseek(dev_fd, phys * BLK_SIZE, SEEK_SET);
					if (write(dev_fd, &dir, sizeof dir) < 0)
					{
						warn("writing directory");
						exit_fsck(errno);
					}
					warnx("replaced with underscore");
				}
			}
			
			if (!*dir[i].name)
			{
				warnx("directory entry with no name");
				if (fix)
				{
					memset(dir[i].name, 0, sizeof dir[i].name);
					
					lseek(dev_fd, phys * BLK_SIZE, SEEK_SET);
					if (write(dev_fd, &dir, sizeof dir) < 0)
					{
						warn("writing directory");
						exit_fsck(errno);
					}
					warnx("cleared");
				}
				continue;
			}
			
			if (dir[i].first_block < sb.data_block ||
			    dir[i].first_block >= sb.data_block + sb.data_size)
			{
				warnx("directory entry has bad "
				       "first_block member");
				if (fix)
				{
					memset(dir[i].name, 0, sizeof dir[i].name);
					
					lseek(dev_fd, phys * BLK_SIZE, SEEK_SET);
					errno = EINVAL;
					if (write(dev_fd, &dir, sizeof dir) < 0)
					{
						warn("writing directory");
						exit_fsck(errno);
					}
					warnx("cleared");
				}
				continue;
			}
			
			check_file(dir[i].first_block, depth + 1, dir[i].name);
			file = load_hdr(first_block);
		}
	}
}

static void check_nlink(void)
{
	int i;
	
	for (i = 0; i < file_count; i++)
		if (file[i].true_nlink != file[i].hd.nlink)
		{
			warnx("%i has incorrect link count", file[i].first_block);
			if (fix)
			{
				file[i].hd.nlink = file[i].true_nlink;
				save_hdr(&file[i]);
				warnx("fixed");
			}
		}
}

static void comp_bam(uint8_t *rbam, uint8_t *dbam, uint32_t bbn)
{
	uint32_t dbn;
	uint8_t xor;
	int i, n;
	
	for (i = 0; i < BLK_SIZE; i++)
	{
		if (rbam[i] == dbam[i])
			continue;
		xor = rbam[i] ^ dbam[i];
		
		n = 0;
		while (xor)
		{
			if (xor & 1)
			{
				dbn  = bbn * BLK_SIZE * 8;
				dbn |= i * 8;
				dbn |= n;
				
				if ((dbam[i] >> n) & 1)
					warnx("block %lu is marked as used", (unsigned long)dbn);
				else
					warnx("block %lu is not marked as used", (unsigned long)dbn);
			}
			xor >>= 1;
			n++;
		}
	}
}

static void check_bam(void)
{
	uint8_t dbam[BLK_SIZE];
	uint8_t *p;
	uint32_t i;
	int bad = 0;
	
	lseek(dev_fd, sb.bam_block * BLK_SIZE, SEEK_SET);
	p = rbam;
	for (i = 0; i < sb.bam_size; i++)
	{
		if (read(dev_fd, dbam, sizeof dbam) < 0)
		{
			warn("reading BAM");
			exit_fsck(255);
		}
		
		if (memcmp(p, dbam, sizeof dbam))
		{
			if (fix)
			{
				lseek(dev_fd, (sb.bam_block + i) * BLK_SIZE, SEEK_SET);
				if (write(dev_fd, p, sizeof dbam) < 0)
				{
					warnx("bad BAM");
					warn("writing BAM");
					exit_fsck(255);
				}
			}
			if (show_names)
				comp_bam(p, dbam, i);
			bad = 1;
		}
		p += sizeof dbam;
	}
	if (bad)
	{
		warnx("bad BAM");
		if (fix)
			warnx("fixed");
	}
}

static void find_mount(void)
{
	static struct _mtab m[MOUNT_MAX];
	int i;
	
	if (_mtab(m, sizeof m))
		err(errno, "_mtab");
	
	for (i = 0; i < MOUNT_MAX; i++)
		if (m[i].mounted && !strcmp(m[i].device, dev_name))
		{
			mount = &m[i];
			return;
		}
}

static void find_root(void)
{
	static struct _mtab m[MOUNT_MAX];
	int i;
	
	if (_mtab(m, sizeof m))
		err(errno, "_mtab");
	
	for (i = 0; i < MOUNT_MAX; i++)
		if (m[i].mounted && !*m[i].prefix)
		{
			dev_name = m[i].device;
			mount = &m[i];
			return;
		}
	errx(1, "root not mounted");
}

static void exit_fsck(int status)
{
	if (mount)
		_mount(mount->prefix, mount->device, mount->fstype, mount->flags);
	exit(status);
}

static void procopt(int argc, char **argv)
{
	int opt;
	
	while (opt = getopt(argc, argv, "Fvwqr"), opt > 0)
		switch (opt)
		{
		case 'F':
			fix = 1;
			break;
		case 'v':
			show_names = 1;
			break;
		case 'q':
			qflag = 1;
			break;
		case 'w':
			enable_write = 1;
			break;
		default:
			exit(255);
		}
}

static void sig_int(int nr)
{
	if (mount)
		_mount(mount->prefix, mount->device, mount->fstype, mount->flags);
	
	signal(nr, SIG_DFL);
	raise(nr);
}

int main(int argc, char **argv)
{
	int i;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		fprintf(stderr, "Usage: fsck [OPTIONS] [--] DEVICE [INODE]...\n\n"
				"Check and repair filesystem.\n\n"
				"    -F   try to repair the filesystem\n"
				"    -v   be verbose\n"
				"    -q   quiet mode\n"
				"    -w   remount the filesystem read/write after successfull check\n\n"
				"Default is not to modify the filesystem even if errors are detected.\n\n"
				"The filesystem is not accessible during a check.\n\n");
		return 0;
	}
	procopt(argc, argv);
	
	if (chdir("/dev"))
		err(errno, "/dev");
	
	if (optind < argc)
		dev_name = argv[optind++];
	else
		find_root();
	
	dev_fd	 = open(dev_name, O_RDWR);
	if (dev_fd < 0)
		err(errno, "%s", dev_name);
	
	for (i = optind; i < argc; i++)
	{
		if (icheck_cnt >= ICHECK_MAX)
		{
			errx(255, "to many i-nodes requested");
			return 255;
		}
		icheck[icheck_cnt++] = atol(argv[i]);
	}
	
	find_mount();
	
	signal(SIGQUIT, sig_int);
	signal(SIGINT, sig_int);
	
	if (mount && _umount(mount->prefix))
		err(errno, "_umount");
	
	if (!qflag)
		warnx("loading superblock");
	load_super();
	
	if (!qflag)
		warnx("checking files and directories");
	check_file(sb.root_block, 0, "/");
	
	if (!qflag)
		warnx("checking link counts");
	check_nlink();
	
	if (!qflag)
		warnx("checking BAM for orphaned blocks");
	check_bam();
	
	if (!qflag)
		warnx("%i files checked", file_count);
	
	if (mount && enable_write)
		mount->flags &= ~MF_READ_ONLY;
	if (mount)
		_mount(mount->prefix, mount->device, mount->fstype, mount->flags);
	return 0;
}
