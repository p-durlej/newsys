
#include <bioctl.h>
#include <stdlib.h>
#include <err.h>

int main(int argc, char **argv)
{
	if (argc != 2)
		errx(1, "wrong number of arguments");
	
	if (_blk_add(strtoul(argv[1], NULL, 0)))
		err(1, "addblk");
	
	return 0;
}
