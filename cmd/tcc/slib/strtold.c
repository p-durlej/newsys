
#include <stdlib.h>

static void nofloat(void)
{
	tcc_error("floating-point support is disabled");
}

long double strtold(const char *nptr, char **endptr)
{
	nofloat();
}

double strtod(const char *nptr, char **endptr)
{
	nofloat();
}

float strtof(const char *nptr, char **endptr)
{
	nofloat();
}

double ldexp(double x, int exp)
{
	nofloat();
}
