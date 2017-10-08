
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>

int main(int argc, char **argv)
{
	struct stat st1, st2;
	
	if (!stat("/bin/aout", &st1) && !stat("a.out", &st2) && st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino)
	{
		errno = EPERM;
		err(127, "a.out");
	}
	
	execv("a.out", argv);
	err(127, "a.out");
}
