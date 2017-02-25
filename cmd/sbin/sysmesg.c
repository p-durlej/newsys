
#include <os386.h>

int main(int argc, char **argv)
{
	int i;
	
	if (argc > 1)
	{
		for (i = 1; i < argc - 1; i++)
		{
			_sysmesg(argv[i]);
			_sysmesg(" ");
		}
		_sysmesg(argv[i]);
		_sysmesg("\n");
	}
	
	return 0;
}
