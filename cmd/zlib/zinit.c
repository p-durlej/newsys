/* Copyright (c) 2017, Piotr Durlej
 * All rights reserved.
 */

#include <sys/types.h>
#include <dev/rd.h>
#include <string.h>
#include <unistd.h>
#include <mount.h>
#include <fcntl.h>
#include <paths.h>
#include <os386.h>
#include <zlib.h>

#define RD_DRV	"/lib/drv/rd.drv"
#define RD	"rd7"
#define IMAGE	"/lib/sys/root"

typedef void outfunc(const void *data, size_t sz);

static off_t image_size;
static int rd_fd;

static void fail(const char *msg)
{
	_sysmesg("zinit: ");
	_sysmesg(msg);
	_sysmesg("\n");
	
	for (;;)
		pause();
}

static void zread(const char *pathname, outfunc *out)
{
	static char ibuf[32768];
	static char obuf[32768];
	
	z_stream zs;
	ssize_t cnt;
	int fd;
	
	fd = open(pathname, O_RDONLY);
	if (fd < 0)
		fail("cannot open compressed image");
	
	bzero(&zs, sizeof zs);
	
	if (inflateInit(&zs))
		fail("cannot initialize zlib");
	
	while (cnt = read(fd, ibuf, sizeof ibuf), cnt)
	{
		if (cnt < 0)
			fail("cannot read compressed image");
		
		zs.next_in  = (void *)ibuf;
		zs.avail_in = cnt;
		
		for (;;)
		{
			zs.avail_out = sizeof obuf;
			zs.next_out  = (void *)obuf;
			
			switch (inflate(&zs, Z_NO_FLUSH))
			{
			case Z_STREAM_END:
			case Z_OK:
				break;
			default:
				fail("cannot inflate");
			}
			
			if (zs.avail_out < sizeof obuf)
				out(obuf, sizeof obuf - zs.avail_out);
			
			if (!zs.avail_in)
				break;
		}
	}
	close(fd);
}

static void count(const void *buf, size_t sz)
{
	image_size += sz;
}

static void init(const void *buf, size_t sz)
{
	if (write(rd_fd, buf, sz) != sz)
		fail("cannot write to ramdisk");
}

int main(int argc, char **argv)
{
	blk_t bcnt;
	int i;
	
	for (i = 0; i < OPEN_MAX; i++)
		close(i); // XXX
	
	if (_mod_load(RD_DRV, NULL, 0))
		fail("cannot load " RD_DRV);
	
	zread(IMAGE, count);
	_cprintf("zinit: image size: %li bytes\n", (long)image_size);
	
	if (image_size & 511)
		fail("image size not multiple of 512");
	bcnt = image_size / 512;
	
	if (_mount(_PATH_M_DEV, "nodev", "dev", 0))
		fail("cannot mount /dev");
	
	rd_fd = open("/dev/rdsk/" RD, O_RDWR);
	if (rd_fd < 0)
		fail("cannot open /dev/rdsk/" RD);
	if (ioctl(rd_fd, RDIOCNEW, &bcnt))
		fail("cannot allocate ramdisk memory");
	zread(IMAGE, init);
	close(rd_fd);
	
	if (_umount(""))
		fail("cannot unmount old root");
	_umount("/dev");
	
	if (_mount("", RD, "boot", 0))
		fail("cannot mount new root");
	
	execl("/sbin/init", "/sbin/init", (void *)NULL);
	fail("cannot exec init");
	return 1;
}
