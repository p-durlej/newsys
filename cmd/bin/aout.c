
#include <unistd.h>
#include <err.h>

int main(int argc, char **argv)
{
	execv("a.out", argv);
	err(127, "a.out");
}
