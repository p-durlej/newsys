/* Copyright (c) 2017, Piotr Durlej
 * All rights reserved.
 */

#include <unistd.h>
#include <os386.h>

int main(int argc, char **argv)
{
	_sysmesg("zinit: not yet\n");
	for (;;)
		pause();
}
