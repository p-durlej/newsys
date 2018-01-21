/* Copyright (c) 2018, Piotr Durlej
 * All rights reserved.
 */

#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <err.h>

static struct opt
{
	const char *	name;
	int		flag;
} opt[] =
{
	{ "isig",	ISIG	},
	{ "icanon",	ICANON	},
	{ "echo",	ECHO	},
	{ "echonl",	ECHONL	},
};

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
	
	for (i = 0; i < sizeof opt / sizeof *opt; i++)
	{
		if (!(tio.c_lflag & opt[i].flag))
			putchar('-');
		
		puts(opt[i].name);
	}
}

static int setcc(const char *name, const char *value, int set)
{
	int c;
	int i;
	
	for (i = 0; i < sizeof opt / sizeof *opt; i++)
		if (!strcmp(opt[i].name, name))
		{
			c = opt[i].flag;
			if (set)
				tio.c_lflag |= c;
			else
				tio.c_lflag &= ~c;
			return 1;
		}
	
	if (!value)
	{
		warnx("%s: missing value", name);
		xit = 1;
		return 1;
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
			return 2;
		}
	
	warnx("%s: invalid option", name);
	xit = 1;
	return 2;
}

int main(int argc, char **argv)
{
	int cnt;
	
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
			cnt = setcc(argv[0], argv[1], 1);
			
			argv += cnt;
			argc -= cnt;
		}
		else
		{
			setcc(argv[0] + 1, "", 0);
			
			argv++;
			argc--;
		}
	}
	
	if (!xit && tcsetattr(0, TCSANOW, &tio))
		err(1, "stdin");
	return xit;
}
