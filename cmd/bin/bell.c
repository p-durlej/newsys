
#include <dev/speaker.h>
#include <stdlib.h>
#include <err.h>

int main(int argc, char **argv)
{
	int a1 = 400, a2 = 500;
	
	if (argc >= 2)
		a1 = atoi(argv[1]);
	if (argc >= 3)
		a2 = atoi(argv[2]);
	
	if (spk_tone(-1, a1, a2))
		warn("spk_tone");
	spk_close(-1);
	
	return 0;
}
