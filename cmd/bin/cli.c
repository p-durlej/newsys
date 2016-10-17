/* Copyright (c) 2016, Piotr Durlej
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <priv/copyright.h>

#include <sys/utsname.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/io.h>
#include <newtask.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <os386.h>
#include <bioctl.h>
#include <ctype.h>
#include <time.h>
#include <dev/speaker.h>
#include <systat.h>
#include <err.h>

static void cmd_path(int argc, char **argv);
static void cmd_addblk(int argc, char **argv);
static void cmd_exit(int argc, char **argv);
static void cmd_cd(int argc, char **argv);
static void cmd_list(int argc, char **argv);
static void cmd_exec(int argc, char **argv);
static void cmd_echo(int argc, char **argv);
static void cmd_sysmesg(int argc, char **argv);
static void cmd_copy(int argc, char **argv);
static void cmd_ren(int argc, char **argv);
static void cmd_link(int argc, char **argv);
static void cmd_del(int argc, char **argv);
static void cmd_dir(int argc, char **argv);
static void cmd_help(int argc, char **argv);
static void cmd_mkdir(int argc, char **argv);
static void cmd_dmesg(int argc, char **argv);
static void cmd_cat(int argc, char **argv);
static void cmd_clr(int argc, char **argv);
static void cmd_kill(int argc, char **argv);
static void cmd_set(int argc, char **argv);
static void cmd_tty(int argc, char **argv);
static void cmd_umask(int argc, char **argv);
static void cmd_bell(int argc, char **argv);
static void cmd_dtrap(int argc, char **argv);
static void cmd_systat(int argc, char **argv);
static void cmd_panic(int argc, char **argv);
static void cmd_debug(int argc, char **argv);
static void cmd_ver(int argc, char **argv);
static void cmd_bstat(int argc, char **argv);
// static void cmd_vmmdate(int argc, char **argv);

static int	word_len(char *str);
static char **	split_words(char *str);
static void	free_argv(char **argv);
static int	is_internal(char *cmd);
static void	do_internal(int argc, char **argv);
static void	print_status(char *cmd, int status);
static void	do_external(int argc, char **argv, int exec);
static int	io_redir(int argc, char **argv, int err_fd);
static void	do_cmd(char *cmd);

struct dir_entry
{
	char name[NAME_MAX];
	struct stat st;
	int st_errno;
};

static struct
{
	char *name;
	void (*proc)(int argc, char **argv);
} cmd_tab[] =
{
	{ "path",	cmd_path	},
	{ "addblk",	cmd_addblk	},
	{ "exit",	cmd_exit	},
	{ "cd",		cmd_cd		},
	{ "list",	cmd_list	},
	{ "exec",	cmd_exec	},
	{ "echo",	cmd_echo	},
	{ "sysmesg",	cmd_sysmesg	},
	{ "copy",	cmd_copy	},
	{ "ren",	cmd_ren		},
	{ "del",	cmd_del		},
	{ "dir",	cmd_dir		},
	{ "help",	cmd_help	},
	{ "mkdir",	cmd_mkdir	},
	{ "dmesg",	cmd_dmesg	},
	{ "cat",	cmd_cat		},
	{ "type",	cmd_cat		},
	{ "clr",	cmd_clr		},
	{ "clear",	cmd_clr		},
	{ "kill",	cmd_kill	},
	{ "set",	cmd_set		},
	{ "tty",	cmd_tty		},
	{ "umask",	cmd_umask	},
	{ "bell",	cmd_bell	},
	{ "link",	cmd_link	},
	{ "dtrap",	cmd_dtrap	},
	{ "systat",	cmd_systat	},
	{ "panic",	cmd_panic	},
	{ "debug",	cmd_debug	},
	{ "ver",	cmd_ver		},
	{ "bstat",	cmd_bstat	},
	// { "vmmdate",	cmd_vmmdate	},
};

static char *	command;
static char *	script;
static char *	tty;

static void cmd_path(int argc, char **argv)
{
	char *p;
	
	switch (argc)
	{
	case 1:
		p = getenv("PATH");
		if (p)
			printf("path=\"%s\"\n", getenv("PATH"));
		else
			printf("path is not set\n");
		break;
	case 2:
		if (argv[1][0] == '+')
		{
			p = getenv("PATH");
			if (p)
			{
				char cat[strlen(p) + strlen(argv[1]) + 2];
				
				sprintf(cat, "%s:%s", p, argv[1] + 1);
				setenv("PATH", cat, 1);
			}
			else
				setenv("PATH", argv[1] + 1, 1);
		}
		else
			setenv("PATH", argv[1], 1);
		break;
	default:
		warnx("path: wrong number of arguments");
	}
}

static void cmd_addblk(int argc, char **argv)
{
	if (argc != 2)
	{
		warnx("addblk: wrong number of arguments");
		return;
	}
	if (_blk_add(strtoul(argv[1], NULL, 0)))
		warn("addblk");
}

static void cmd_exit(int argc, char **argv)
{
	if (argc == 1)
		exit(0);
	else if (argc == 2)
		exit(strtoul(argv[1], NULL, 0));
	else
		warnx("exit: wrong number of arguments");
}

static void cmd_cd(int argc, char **argv)
{
	if (argc != 2)
	{
		warnx("cd: wrong number of arguments");
		return;
	}
	if (chdir(argv[1]))
		warn("cd: %s", argv[1]);
	return;
}

static void cmd_list(int argc, char **argv)
{
	struct dirent *de;
	DIR *d;

	if (argc != 1)
	{
		warnx("list: wrong number of arguments");
		return;
	}

	d = opendir(".");
	if (!d)
	{
		perror(".");
		return;
	}
	while ((de = readdir(d)))
		printf("%s\n", de->d_name);
	closedir(d);
}

static void cmd_exec(int argc, char **argv)
{
	do_external(argc - 1, argv + 1, 1);
}

static void cmd_echo(int argc, char **argv)
{
	int i;
	
	if (argc > 1)
	{
		for (i = 1; i < argc - 1; i++)
			printf("%s ", argv[i]);
		printf("%s\n", argv[i]);
	}
}

static void cmd_sysmesg(int argc, char **argv)
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
}

static void copy_file(char *from, char *to)
{
	char buf[4096];
	int s_fd;
	int d_fd;
	int cnt;
	
	s_fd = open(from, O_RDONLY);
	if (s_fd < 0)
	{
		warn("copy: %s", from);
		return;
	}
	
retry:
	d_fd = open(to, O_WRONLY | O_EXCL | O_CREAT, 0666);
	if (d_fd < 0)
	{
		if (errno == EEXIST)
		{
			if (unlink(to))
			{
				warn("copy: %s", to);
				close(s_fd);
				return;
			}
			goto retry;
		}
		warn("copy: %s", to);
		close(s_fd);
		return;
	}
	
	while ((cnt = read(s_fd, buf, sizeof buf)))
	{
		if (cnt < 0)
		{
			warn("copy: %s", from);
			close(d_fd);
			close(s_fd);
			return;
		}
		if (write(d_fd, buf, cnt) != cnt)
		{
			warn("copy: %s", to);
			close(d_fd);
			close(s_fd);
			return;
		}
	}
	
	close(s_fd);
	close(d_fd);
}

static char *basename(char *s)
{
	char *p = strrchr(s, '/');
	
	if (p)
		return p + 1;
	return s;
}

static void cmd_copy(int argc, char **argv)
{
	struct stat st;
	int l;
	int i;
	
	if (argc < 2)
		warnx("copy: wrong number of arguments");
	else if (argc < 3)
		copy_file(argv[1], basename(argv[1]));
	else
	{
		if (argc == 3)
		{
			if (stat(argv[argc - 1], &st) || !S_ISDIR(st.st_mode))
			{
				copy_file(argv[1], argv[2]);
				return;
			}
		}
		
		l = strlen(argv[argc - 1]);
		
		for (i = 1; i < argc - 1; i++)
		{
			char *b = basename(argv[i]);
			char targ[strlen(b) + l + 2];
			
			strcpy(targ, argv[argc - 1]);
			strcat(targ, "/");
			strcat(targ, b);
			
			copy_file(argv[i], targ);
		}
	}
}

static void cmd_ren(int argc, char **argv)
{
	if (argc != 3)
	{
		warnx("ren: wrong number of arguments");
		return;
	}
	
	if (rename(argv[1], argv[2]))
		warn("ren: %s -> %s", argv[1], argv[2]);
}

static void cmd_link(int argc, char **argv)
{
	if (argc != 3)
	{
		warnx("link: wrong number of arguments");
		return;
	}
	
	if (link(argv[1], argv[2]))
		warn("link: %s -> %s", argv[1], argv[2]);
}

static void cmd_del(int argc, char **argv)
{
	int i;
	
	if (argc < 2)
	{
		warnx("del: wrong number of arguments");
		return;
	}
	
	for (i = 1; i < argc; i++)
		if (remove(argv[i]))
			warn("del: %s", argv[i]);
}

static int load_dir(struct dir_entry **buf, const char *path)
{
	static char pathname[PATH_MAX];
	
	struct dir_entry *pde;
	struct dirent *de;
	DIR *dir;
	int cnt;
	int i;
	
	dir = opendir(path);
	if (!dir)
		return -1;
	
	cnt = 0;
	while ((de = readdir(dir)))
		cnt++;
	
	pde = malloc(sizeof *pde * cnt);
	if (!pde)
	{
		closedir(dir);
		return -1;
	}
	
	rewinddir(dir);
	for (i = 0; i < cnt; i++)
	{
		de = readdir(dir);
		if (!de)
		{
			closedir(dir);
			free(pde);
			return -1;
		}
		strcpy(pde[i].name, de->d_name);
		
		if (snprintf(pathname, sizeof pathname, "%s/%s", path, de->d_name) >= sizeof pathname)
		{
			closedir(dir);
			free(pde);
			errno = ENAMETOOLONG;
			return -1;
		}
		
		if (stat(pathname, &pde[i].st))
			pde[i].st_errno = errno;
		else
			pde[i].st_errno = 0;
	}
	*buf = pde;
	closedir(dir);
	return cnt;
}

static int cmp_dir_entry(const void *p1, const void *p2)
{
#define de1	((const struct dir_entry *)p1)
#define de2	((const struct dir_entry *)p2)
	mode_t type1;
	mode_t type2;
	
	if (!de1->st_errno)
		type1 = de1->st.st_mode & S_IFMT;
	else
		type1 = S_IFREG;
	
	if (!de2->st_errno)
		type2 = de2->st.st_mode & S_IFMT;
	else
		type2 = S_IFREG;
	
	if (type1 == S_IFDIR && type2 != S_IFDIR)
		return -1;
	if (type2 == S_IFDIR && type1 != S_IFDIR)
		return 1;
	
	return strcmp(de1->name, de2->name);
#undef de1
#undef de2
}

static void cmd_dir(int argc, char **argv)
{
	struct dir_entry *de;
	char *path = "";
	char *tystr;
	char *tmstr;
	int all = 0;
	int cnt;
	int i;
	
	if (argc < 1 || argc > 2)
	{
		warnx("dir: wrong number of arguments");
		return;
	}
	if (argc == 2)
		path = argv[1];
	
	if (*path == '+')
	{
		path++;
		all = 1;
	}
	
	if (!*path)
		path = ".";
	
	cnt = load_dir(&de, path);
	if (cnt < 0)
	{
		perror(argv[1]);
		return;
	}
	qsort(de, cnt, sizeof *de, cmp_dir_entry);
	
	for (i = 0; i < cnt; i++)
	{
		if (!all && *de[i].name == '.')
			continue;
		
		if (!de[i].st_errno)
		{
			tmstr = ctime(&de[i].st.st_mtime);
			if (!tmstr)
				tmstr = "Invalid time";
			if (S_ISREG(de[i].st.st_mode))
			{
				printf("%-16s  %8ji %s",
					de[i].name,
					(intmax_t)de[i].st.st_size,
					tmstr);
				continue;
			}
			switch (de[i].st.st_mode & S_IFMT)
			{
				case S_IFDIR:  tystr = " <DIR>"; break;
				case S_IFCHR:  tystr = " <CHR>"; break;
				case S_IFBLK:  tystr = " <BLK>"; break;
				case S_IFIFO:  tystr = "<FIFO>"; break;
				case S_IFSOCK: tystr = "<SOCK>"; break;
				default:       tystr = " <\?\?\?>";
			}
			printf("%-16s    %s %s", de[i].name, tystr, tmstr);
		}
		else
			warn("dir: %s", de[i].name);
	}
	free(de);
}

static void cmd_help(int argc, char **argv)
{
	printf("addblk         # increase number of cached disk blocks\n");
	printf("bell           # bell control\n");
	printf("cat, type      # show file contents\n");
	printf("cd             # change into directory\n");
	printf("clr, clear     # clear screen\n");
	printf("copy           # copy files\n");
	printf("del            # remove files and directories\n");
	printf("dir            # list files\n");
	printf("dmesg          # diagnostic messages\n");
	printf("dtrap          # restore default signal handlers\n");
	printf("echo           # print messages\n");
	printf("exec           # replace CLI with another program\n");
	printf("exit           # terminate CLI\n");
	printf("help           # this help page\n");
	printf("kill           # kill processes\n");
	printf("link           # link file\n");
	printf("list           # list files\n");
	printf("mkdir          # create directory\n");
	printf("panic          # emergency shutdown\n");
	printf("path           # set exec search path\n");
	printf("ren            # rename file\n");
	printf("set            # display or set variables\n");
	printf("sysmesg        # print messages on the console\n");
	printf("systat         # system statistics\n");
	printf("tty            # current TTY\n");
	printf("umask          # file permission mask\n");
	printf("ver            # print version information\n");
}

static void cmd_mkdir(int argc, char **argv)
{
	int i;
	
	for (i = 1; i < argc; i++)
		if (mkdir(argv[i], 0777))
			warn("mkdir: %s", argv[i]);
}

static void cmd_dmesg(int argc, char **argv)
{
	char buf[DMESG_BUF];
	int start;
	int end;
	int i;
	
	end = _dmesg(buf, sizeof buf);
	if (end < 0)
	{
		perror("cli: dmesg");
		return;
	}
	
	for (i = end; i < sizeof buf; i++)
		if (buf[i])
			fputc(buf[i], stdout);
	for (i = 0; i < end; i++)
		if (buf[i])
			fputc(buf[i], stdout);
	
	if (argc > 1)
		for (;;)
		{
			sleep(1);
			
			start = end;
			end = _dmesg(buf, sizeof buf);
			if (end < 0)
			{
				perror("cli: dmesg");
				return;
			}
			
			for (i = start; i != end; i++, i %= sizeof buf)
				if (buf[i])
					fputc(buf[i], stdout);
			fflush(stdout);
		}
}

static void cmd_cat(int argc, char **argv)
{
	char buf[8192];
	int bcnt;
	int fd;
	int i;
	
	if (argc < 2)
	{
		warnx("cat: wrong number of arguments");
		return;
	}
	for (i = 1; i < argc; i++)
	{
		fd = open(argv[i], O_RDONLY);
		if (fd < 0)
		{
			warn("%s: %s", argv[0], argv[i]);
			continue;
		}
		while ((bcnt = read(fd, buf, sizeof buf)))
		{
			if (bcnt < 0)
			{
				warn("%s: %s", argv[0], argv[i]);
				close(fd);
				goto next;
			}
			write(1, buf, bcnt);
		}
		close(fd);
next:
		;
	}
}

static void cmd_clr(int argc, char **argv)
{
	printf("\033[H\033[J");
	fflush(stdout);
}

static void cmd_kill(int argc, char **argv)
{
	int sig = SIGTERM;
	long nr;
	int i;
	char *p;
	
	if (argc == 2 && !strcmp(argv[1], "-l"))
	{
		printf(" 1 HUP    2 INT    3 QUIT   4 ILL    5 TRAP   6 ABRT\n");
		printf(" 7 BUS    8 FPE    9 KILL  10 STOP  11 SEGV  12 CONT\n");
		printf("13 PIPE  14 ALRM  15 TERM  16 USR1  17 USR2  18 CHLD\n");
		printf("19 PWR            29 EVT   30 EXEC  31 STK   32 OOM\n");
		return;
	}
	for (i = 1; i < argc; i++)
	{
		nr = strtol(argv[i], &p, 0);
		if (*p || nr != (int)nr)
		{
			warnx("kill: %s: invalid argument", argv[i]);
			return;
		}
		if (i == 1 && nr < 0)
			sig = -nr;
		else
			if (kill(nr, sig))
				warn("kill: %i", (int)nr);
	}
}

static void cmd_set(int argc, char **argv)
{
	char **p;
	char *s;
	
	if (argc > 2)
	{
		warnx("set: wrong number of arguments");
		return;
	}
	if (argc == 1)
	{
		p = environ;
		while (*p)
		{
			printf("%s\n", *p);
			p++;
		}
		return;
	}
	
	s = strdup(argv[1]);
	if (!s)
	{
		warn("set");
		return;
	}
	if (putenv(s))
	{
		warn("set");
		free(s);
		return;
	}
}

static void cmd_tty(int argc, char **argv)
{
	char *tty;
	
	tty = ttyname(0);
	if (!tty)
		printf("Not a TTY.\n");
	else
		printf("%s\n", tty);
}

static void cmd_umask(int argc, char **argv)
{
	mode_t m;
	
	if (argc == 1)
	{
		m = umask(0777);
		printf("0%03lo\n", (long)m);
		umask(m);
	}
	else if (argc == 2)
		umask(strtoul(argv[1], NULL, 8));
	else
		warnx("umask: wrong number of arguments");
}

static void cmd_bell(int argc, char **argv)
{
	int a1 = 400, a2 = 500;
	
	if (argc >= 2)
		a1 = atoi(argv[1]);
	if (argc >= 3)
		a2 = atoi(argv[2]);
	
	if (spk_tone(-1, a1, a2))
		warn("spk_tone");
	spk_close(-1);
}

static void cmd_dtrap(int argc, char **argv)
{
	int i;
	
	for (i = 1; i <= SIGMAX; i++)
		signal(i, SIG_DFL);
}

static void cmd_systat(int argc, char **argv)
{
	struct systat st;
	
	if (_systat(&st))
	{
		perror("_systat");
		return;
	}
	
	printf("task_avail = %i\n",	(int)st.task_avail);
	printf("task_max   = %i\n",	(int)st.task_max);
	printf("core_avail = %lli\n",	(long long)st.core_avail);
	printf("core_max   = %lli\n",	(long long)st.core_max);
	printf("kva_avail  = %lli\n",	(long long)st.kva_avail);
	printf("kva_max    = %lli\n",	(long long)st.kva_max);
	printf("file_avail = %i\n",	(int)st.file_avail);
	printf("file_max   = %i\n",	(int)st.file_max);
	printf("fso_avail  = %i\n",	(int)st.fso_avail);
	printf("fso_max    = %i\n",	(int)st.fso_max);
	printf("blk_dirty  = %i\n",	(int)st.blk_dirty);
	printf("blk_valid  = %i\n",	(int)st.blk_valid);
	printf("blk_avail  = %i\n",	(int)st.blk_avail);
	printf("blk_max    = %i\n",	(int)st.blk_max);
	printf("sw_freq    = %i\n",	(int)st.sw_freq);
	printf("hz         = %i\n",	(int)st.hz);
	printf("uptime     = %i\n",	(int)st.uptime);
	printf("cpu        = %i\n",	(int)st.cpu);
	printf("cpu_max    = %i\n",	(int)st.cpu_max);
}

static void cmd_panic(int argc, char **argv)
{
	if (argc != 2)
	{
		warnx("panic: wrong nr of args");
		return;
	}
	if (_panic(argv[1]))
		warn("panic");
}

static void cmd_debug(int argc, char **argv)
{
	if (argc != 2)
	{
		warnx("debug: wrong nr of args");
		return;
	}
	if (_debug(atoi(argv[1])))
		warn("_debug");
}

static void show_version(void)
{
	struct utsname un;
	
	uname(&un);
	puts(SYS_PRODUCT " Command Line");
	printf("Release %s, %s\n\n", un.release, un.version);
}

static void cmd_ver(int argc, char **argv)
{
	show_version();
}

static void cmd_bstat(int argc, char **argv)
{
	int cnt;
	int i;
	
	cnt = _bdev_max();
	if (cnt < 0)
	{
		warn("bstat");
		return;
	}
	
	struct bdev_stat bs[cnt];
	
	if (_bdev_stat(bs))
		warn("bstat");
	
	puts("NAME             READS    WRITES    ERRORS");
	puts("------------ --------- --------- ---------");
	
	for (i = 0; i < cnt; i++)
		if (*bs[i].name)
		{
			if (!bs[i].read_cnt && !bs[i].write_cnt &&
			    !bs[i].error_cnt)
				continue;
			
			printf("%-12s %9ji %9ji %9ji\n",
				bs[i].name,
				(uintmax_t)bs[i].read_cnt,
				(uintmax_t)bs[i].write_cnt,
				(uintmax_t)bs[i].error_cnt);
		}
}

#if 0
static void cmd_vmmdate(int argc, char **argv)
{
	time_t t;
	
	if (_iopl(3))
	{
		perror("cli: vmmdate: _iopl");
		return;
	}
	t = inl(0xe8);
	fputs(ctime(&t), stdout);
}
#endif

static int word_len(char *str)
{
	char *p = str;
	
	if (*p == '\"')
	{
		p++;
		while (*p && *p != '\"')
			p++;
		if (!*p)
		{
			warnx("unterminated string");
			return -1;
		}
		return p - str + 1;
	}
	
	while (*p && !isspace(*p))
		p++;
	
	return p - str;
}

static char **split_words(char *str)
{
	int cnt = 0;
	int l;
	char *p = str;
	char **words;
	char **wp;
	
	while (*p)
	{
		while (isspace(*p))
			p++;
		if (!*p || *p == '#')
			break;
		
		l = word_len(p);
		if (l < 0)
			return NULL;
		p += l;
		cnt++;
	}
	
	wp = words = calloc(cnt + 1, sizeof *words);
	if (!words)
	{
		perror("cli: calloc");
		return NULL;
	}
	
	p = str;
	while (*p)
	{
		while (isspace(*p))
			p++;
		if (!*p || *p == '#')
			break;
		
		l = word_len(p);
		if (l < 0)
		{
			free_argv(words);
			return NULL;
		}
		*wp = malloc(l + 1);
		if (!*wp)
		{
			perror("cli: malloc");
			free_argv(words);
			return NULL;
		}
		if (*p == '\"')
		{
			memcpy(*wp, p + 1, l - 2);
			(*wp)[l - 2] = 0;
		}
		else
		{
			memcpy(*wp, p, l);
			(*wp)[l] = 0;
		}
		p += l;
		wp++;
	}
	
	return words;
}

static void free_argv(char **argv)
{
	char **p = argv;
	
	while (*p)
		free(*(p++));
	free(argv);
}

static int is_internal(char *cmd)
{
	int i;
	
	for (i = 0; i < sizeof cmd_tab / sizeof *cmd_tab; i++)
		if (!strcmp(cmd_tab[i].name, cmd))
			return 1;
	return 0;
}

static void do_internal(int argc, char **argv)
{
	int i;
	
	for (i = 0; i < sizeof cmd_tab / sizeof *cmd_tab; i++)
		if (!strcmp(cmd_tab[i].name, *argv))
		{
			cmd_tab[i].proc(argc, argv);
			return;
		}
}

static void print_status(char *cmd, int status)
{
	if (!WIFEXITED(status))
	{
		if (WCOREDUMP(status))
			warnx("%s: Signal %i (core dumped)", cmd, WTERMSIG(status));
		else
			warnx("%s: Signal %i", cmd, WTERMSIG(status));
	}
}

static void do_external(int argc, char **argv, int exec)
{
	char *xa[argc + 1];
	int status;
	int i;
	pid_t pid;
	
	for (i = 0; i < argc; i++)
		xa[i] = argv[i];
	xa[i] = NULL;
	
	if (exec)
	{
		execvp(xa[0], xa);
		perror(xa[0]);
	}
	else
	{
		pid = _newtaskvp(xa[0], xa);
		if (pid < 0)
			warn("%s", xa[0]);
		else
		{
			while (wait(&status) != pid);
			print_status(xa[0], status);
		}
	}
}

static int io_redir(int argc, char **argv, int err_fd)
{
	char **p;
	int i;
	
	for (i = 0; i < argc; i++)
	{
		if (!strcmp(argv[i], ">2") && argv[i + 1])
		{
			close(2);
			errno = EBADF;
			if (open(argv[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666) != 2)
			{
				write(err_fd, "cli: ", 5);
				write(err_fd, argv[i + 1], strlen(argv[i + 1]));
				write(err_fd, ": ", 2);
				write(err_fd, strerror(errno),
				      strlen(strerror(errno)));
				write(err_fd, "\n", 1);
				return -1;
			}
			p = &argv[i];
			do
			{
				*p = p[2];
				p++;
			} while(p[-1]);
			argc -= 2;
			i--;
			continue;
		}
		if (!strcmp(argv[i], ">") && argv[i + 1])
		{
			close(1);
			errno = EBADF;
			if (open(argv[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666) != 1)
			{
				write(err_fd, "cli: ", 5);
				write(err_fd, argv[i + 1], strlen(argv[i + 1]));
				write(err_fd, ": ", 2);
				write(err_fd, strerror(errno), strlen(strerror(errno)));
				write(err_fd, "\n", 1);
				return -1;
			}
			p = &argv[i];
			do
			{
				*p = p[2];
				p++;
			} while(p[-1]);
			argc -= 2;
			i--;
			continue;
		}
		if (!strcmp(argv[i], "<") && argv[i + 1])
		{
			close(0);
			errno = EBADF;
			if (open(argv[i + 1], O_RDONLY))
			{
				write(err_fd, "cli: ", 5);
				write(err_fd, argv[i + 1], strlen(argv[i + 1]));
				write(err_fd, ": ", 2);
				write(err_fd, strerror(errno), strlen(strerror(errno)));
				write(err_fd, "\n", 1);
				return -1;
			}
			p = &argv[i];
			do
			{
				*p = p[2];
				p++;
			} while(p[-1]);
			argc -= 2;
			i--;
			continue;
		}
	}
	return argc;
}

static void do_cmd(char *cmd)
{
	char **words;
	char **p0;
	char **p1;
	int l;
	int saved_err;
	int saved_out;
	int saved_in;
	
	p0 = words = split_words(cmd);
	if (!words)
		return;
	while (*p0)
	{
		while (*p0 && !strcmp(*p0, ";"))
			p0++;
		if (!*p0)
			break;
		p1 = p0;
		while (*p1 && strcmp(*p1, ";"))
			p1++;
		
		saved_err = dup(2);
		saved_out = dup(1);
		saved_in  = dup(0);
		fcntl(saved_err, F_SETFD, 1);
		fcntl(saved_out, F_SETFD, 1);
		fcntl(saved_in, F_SETFD, 1);
		l = io_redir(p1 - p0, p0, saved_err);
		if (l <= 0)
		{
			dup2(saved_err, 2);
			dup2(saved_out, 1);
			dup2(saved_in, 0);
			close(saved_err);
			close(saved_out);
			close(saved_in);
			return;
		}
		
		if (is_internal(*p0))
			do_internal(l, p0);
		else
			do_external(l, p0, 0);
		p0 = p1;
		
		dup2(saved_err, 2);
		dup2(saved_out, 1);
		dup2(saved_in, 0);
		close(saved_err);
		close(saved_out);
		close(saved_in);
	}
	free_argv(words);
}

static void sig_int(int nr)
{
	signal(SIGINT, sig_int);
}

static void sig_pipe(int nr)
{
	warnx("received SIGPIPE, exiting");
	
	signal(SIGPIPE, SIG_DFL);
	raise(SIGPIPE);
}

static int run_script(char *pathname)
{
	char cmd[256];
	char *p;
	FILE *f;
	
	f = fopen(pathname, "r");
	if (!f)
	{
		perror(pathname);
		return 1;
	}
	fcntl(fileno(f), F_SETFD, FD_CLOEXEC);
	while (fgets(cmd, sizeof cmd, f))
	{
		p = strchr(cmd, '\n');
		if (p)
			*p = 0;
		do_cmd(cmd);
	}
	fclose(f);
	return 0;
}

static void parse_args(int argc, char **argv)
{
	int c;
	
	while (c = getopt(argc, argv, "C:c:s:t:"), c > 0)
		switch (c)
		{
		case 'C':
			if (chdir(optarg))
				err(errno, "%s", optarg);
			break;
		case 'c':
			command = optarg;
			break;
		case 's':
			script = optarg;
			break;
		case 't':
			tty = optarg;
			break;
		default:
			exit(255);
		}
}

static void switch_tty(const char *pathname)
{
	int ofd, ifd;
	
	ifd = open(pathname, O_RDONLY);
	if (ifd < 0)
	{
		perror(pathname);
		exit(255);
	}
	
	ofd = open(pathname, O_WRONLY);
	if (ifd < 0)
	{
		perror(pathname);
		exit(255);
	}
	
	dup2(ifd, 0);
	dup2(ofd, 1);
	dup2(ofd, 2);
}

int main(int argc, char **argv)
{
	char cwd[PATH_MAX];
	char cmd[256];
	int cnt;
	int i;
	
	if (argc == 2 && !strcmp(argv[1], "--help"))
	{
		fprintf(stderr, "Usage: cli [-t TTY] [-c COMMAND | -s SCRIPT]"
				"\n\n"
				"Command Line Interface.\n\n"
				"    -t TTY      switch to the specified TTY\n"
				"    -c COMMAND  execute shell COMMAND and exit\n"
				"    -C DIR      change into DIR\n"
				"    -s SCRIPT   run script SCRIPT and exit\n\n");
		return 0;
	}
	parse_args(argc, argv);
	
	if (tty)
		switch_tty(tty);
	
	if (command)
	{
		do_cmd(command);
		return 0;
	}
	if (script)
		return run_script(script);
	
	for (i = 1; i <= SIGMAX; i++)
		signal(i, SIG_DFL);
	signal(SIGPIPE, sig_pipe);
	signal(SIGINT, sig_int);
	
	show_version();
	for (;;)
	{
		strcpy(cwd, "???");
		getcwd(cwd, sizeof cwd);
		
		printf("%s> ", cwd);
		fflush(stdout);
		
		signal(SIGCHLD, SIG_IGN);
		cnt = read(0, cmd, sizeof cmd - 1);
		if (cnt < 0)
		{
			perror("cli: stdin");
			if (errno == EINTR)
				continue;
			return errno;
		}
		if (!cnt)
			return 0;
		cmd[cnt] = 0;
		signal(SIGCHLD, SIG_DFL);
		
		do_cmd(cmd);
	}
}
