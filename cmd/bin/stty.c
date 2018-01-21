/* Copyright (c) 2018, Piotr Durlej
 * All rights reserved.
 */

#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <err.h>

static struct cc
{
	const char *	name;
	int		i;
	int		dflt;
} cc[] =
{
	{ "intr",	VINTR,		'C'  & 31 },
	{ "quit",	VQUIT,		'\\' & 31 },
	{ "erase",	VERASE,		'H'  & 31 },
	{ "kill",	VKILL,		'U'  & 31 },
	{ "eof",	VEOF,		'D'  & 31 },
	{ "werase",	VWERASE,	'W'  & 31 },
	{ "status",	VSTATUS,	'T'  & 31 },
	{ "reprint",	VREPRINT,	'R'  & 31 },
};

static struct termios tio;
static int xit;

static void show(void)
{
	int c;
	int i;
	
	for (i = 0; i < sizeof cc / sizeof *cc; i++)
	{
		c = tio.c_cc[cc[i].i] & 255;
		
		printf("%-8s ", cc[i].name);
		switch (c)
		{
		case 255:
			puts("none");
			break;
		case 127:
			puts("^?");
			break;
		default:
			if (iscntrl(c))
				printf("^%c\n", (c & 31) + '@');
			else
				printf("%c\n", c);
		}
	}
}

static void setcc(const char *name, const char *value)
{
	int c;
	int i;
	
	if (!value)
	{
		warnx("%s: missing value", name);
		xit = 1;
		return;
	}
	
	if (*value == '^' && value[1])
		c = value[1] == '?' ? 127 : value[1] & 31;
	else if (!*value)
		c = 255;
	else
		c = *value;
	
	for (i = 0; i < sizeof cc / sizeof *cc; i++)
		if (!strcmp(cc[i].name, name))
		{
			tio.c_cc[cc[i].i] = c;
			return;
		}
	
	warnx("%s: invalid option", name);
	xit = 1;
}

int main(int argc, char **argv)
{
	if (tcgetattr(0, &tio))
		err(1, "stdin");
	
	if (argc < 2)
	{
		show();
		return 0;
	}
	
	argv++;
	argc--;
	
	while (argc > 0)
	{
		if (*argv[0] != '-')
		{
			setcc(argv[0], argv[1]);
			
			argv += 2;
			argc -= 2;
		}
		else
		{
			setcc(argv[0] + 1, "");
			
			argv++;
			argc--;
		}
	}
	
	if (!xit && tcsetattr(0, TCSANOW, &tio))
		err(1, "stdin");
	return xit;
}
