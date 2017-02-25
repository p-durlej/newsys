
#include <os386.h>
#include <err.h>

int main(int argc, char **argv)
{
	if (argc != 2)
		errx(1, "wrong nr of args");
	
	if (_panic(argv[1]))
		err(1, NULL);
	
	return 0;
}
