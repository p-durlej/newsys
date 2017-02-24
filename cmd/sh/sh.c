/* Copyright (c) 2017, Piotr Durlej
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

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <findexec.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <err.h>

#define _PATH_B_GLOB	"/bin/glob"
#define _PATH_B_LOGIN	"/bin/login"
#define _PATH_B_NEWGRP	"/bin/newgrp"
#define _PATH_E_PROFILE	"/etc/profile"
#define UPROFILE	".profile"

#define GLOB_CHRS	"*?["

#define ERR_SYNTAX	127
#define ERR_EXEC	127
#define ERR_FORK	127
#define ERR_MEM		127
#define ERR_GETCWD	127
#define ERR_CHDIR	127
#define ERR_ARGC	127
#define ERR_ARGS	127
#define ERR_OPEN	127
#define ERR_STAT	127
#define ERR_SEEK	127
#define ERR_NOLABEL	1
#define ERR_UNK		127

#define TK_STRING	1
#define TK_SEMICOLON	2
#define TK_PIPE		3
#define TK_OREDIR	4
#define TK_IREDIR	5
#define TK_BACKGROUND	6
#define TK_LPAREN	7
#define TK_RPAREN	8
#define TK_LBRACE	9
#define TK_RBRACE	10

extern char **environ;

typedef int cmd_proc(int argc, char **argv);

struct token
{
	int	type;
	int	par;
	char *	str;
	int	vas;
};

static int  sstk_push(void);
static void sstk_pop(void);

static int cmd_noop(int argc, char **argv);
static int cmd_exit(int argc, char **argv);
static int cmd_chdir(int argc, char **argv);
static int cmd_pwd(int argc, char **argv);
static int cmd_goto(int argc, char **argv);
static int cmd_umask(int argc, char **argv);
static int cmd_wait(int argc, char **argv);
static int cmd_shift(int argc, char **argv);
static int cmd_login(int argc, char **argv);
static int cmd_newgrp(int argc, char **argv);
static int cmd_source(int argc, char **argv);
static int cmd_export(int argc, char **argv);
static int cmd_unset(int argc, char **argv);
static int cmd_exec(int argc, char **argv);
static int cmd_rtrap(int argc, char **argv);

static const struct spec_char *spec_char(char c);
static void		tk_dump(struct token *tk);
static struct token *	tokenize(char *cmd);

static void		prompt(const char *s);
static int		waitexit(int st);
static char *		readln(void);

static void		pstatus(const char *prog, int st);
static struct token *	skip_over(const struct token *tk, const struct token *e);
static struct token *	skip_redir(const struct token *tk, const struct token *e);
static void		do_exec(const char *name, char *const argv[], char *const envp[]);
static void		do_comm1(const struct token *s, const struct token *e, int no_wait, const char *iredir, const char *oredir);
static void		do_comm(const struct token *tk, const struct token *e);

static const char *	do_subst_1(char *s, size_t *rlen);
static char *		do_subst(char *src);

static int		do_line(char *ln);
static void		loop(void);

static void		try_profile(const char *pathname, int mand);
static void		profile(void);

static const char *	var_get(const char *name);
static int		var_set(const char *name, const char *value, int exp);
static int		var_unset(const char *name);
static int		var_assign(const char *nameval, int exp);
static void		var_fetch(void);

static const struct spec_char
{
	int	token;
	int	parity;
	char *	ch;
} spec_chars[] =
{
	{ TK_SEMICOLON,		0,		";" },
	{ TK_PIPE,		0,		"|" },
	{ TK_PIPE,		0,		"^" },
	{ TK_OREDIR,		0,		">" },
	{ TK_IREDIR,		0,		"<" },
	{ TK_BACKGROUND,	0,		"&" },
	{ TK_LPAREN,		TK_RPAREN,	"(" },
	{ TK_RPAREN,		0,		")" },
	{ TK_LBRACE,		TK_RBRACE,	"{" },
	{ TK_RBRACE,		0,		"}" },
};

static const struct comm
{
	char *		name;
	cmd_proc *	proc;
	int		v6;
} comms[] =
{
	{ ":",		cmd_noop,	1 },
	{ "exit",	cmd_exit,	1 },
	{ "chdir",	cmd_chdir,	1 },
	{ "cd",		cmd_chdir,	1 },
	{ "pwd",	cmd_pwd	,	0 },
	{ "goto",	cmd_goto,	1 },
	{ "umask",	cmd_umask,	0 },
	{ "wait",	cmd_wait,	1 },
	{ "shift",	cmd_shift,	1 },
	{ "login",	cmd_login,	1 },
	{ "newgrp",	cmd_newgrp,	1 },
	{ ".",		cmd_source,	0 },
	{ "export",	cmd_export,	0 },
	{ "unset",	cmd_unset,	0 },
	{ "exec",	cmd_exec,	0 },
	{ "rtrap",	cmd_rtrap,	0 },
};

static struct var
{
	char *	name;
	char *	value;
	int	exp;
} *vars;

static struct sstkel
{
	struct sstkel *	next;
	FILE *		file;
	int		nmlc;
	char *		name;
} *source_stk;

extern char *__progname;

static char	lastprompt[256];
static char *	promptp;
static int	v6_flag;
static int	l_flag;
static int	u_flag = 1;
static void *	sigquit;
static void *	sigterm;
static void *	sigint;
static uid_t	uid;
static int	lastst;
static int	stop;
static int	source_nmlc;
static char *	source_name;
static FILE *	source;
static char **	sargv;
static int	sargc;
static int	varc;
static char *	arg0;

static int sstk_push(void)
{
	struct sstkel *e;
	
	e = malloc(sizeof *e);
	if (e == NULL)
		return -1;
	
	e->nmlc = source_nmlc;
	e->name = source_name;
	e->file = source;
	e->next = source_stk;
	source_stk = e;
	
	source	    = NULL;
	source_name = NULL;
	return 0;
}

static void sstk_pop(void)
{
	struct sstkel *e;
	
	e = source_stk;
	source_stk  = e->next;
	source_name = e->name;
	source_nmlc = e->nmlc;
	source	    = e->file;
	free(e);
}

static int cmd_noop(int argc, char **argv)
{
	return 0;
}

static int cmd_exit(int argc, char **argv)
{
	if (v6_flag && source == stdin)
		return waitexit(lastst);
	
	if (argc > 2)
	{
		warnx("%s: wrong nr of args", argv[0]);
		return ERR_ARGC;
	}
	if (argc > 1)
		lastst = atoi(argv[1]);
	exit(lastst);
}

static int cmd_chdir(int argc, char **argv)
{
	const char *d;
	
	if (argc < 2)
	{
		d = var_get("HOME");
		if (d == NULL)
			d = "/";
	}
	else if (argc == 2)
		d = argv[1];
	else
	{
		warnx("%s: wrong nr of args", argv[0]);
		return ERR_ARGC;
	}
	
	if (chdir(d))
	{
		warn("%s: %s", argv[0], d);
		return ERR_CHDIR;
	}
	return 0;
}

static int cmd_pwd(int argc, char **argv)
{
	static char buf[PATH_MAX];
	
	if (argc > 1)
	{
		warnx("%s: wrong nr of args", argv[0]);
		return ERR_ARGC;
	}
	if (getcwd(buf, sizeof buf) == NULL)
	{
		warn("pwd: getcwd");
		return ERR_GETCWD;
	}
	puts(buf);
	return 0;
}

static int cmd_goto(int argc, char **argv)
{
	struct stat st;
	char *p;
	
	if (argc != 2)
	{
		warnx("%s: wrong nr of args", argv[0]);
		return ERR_ARGC;
	}
	if (fstat(fileno(source), &st))
	{
		warn("%s: fstat", source_name);
		return ERR_STAT;
	}
	if (!S_ISREG(st.st_mode))
	{
		warnx("%s: Not a regular file", source_name);
		return ERR_SEEK;
	}
	
	rewind(source);
	while (p = readln(), p) /* XXX */
	{
		while (isspace(*p))
			p++;
		
		if (*p != ':')
			continue;
		p++;
		
		while (isspace(*p))
			p++;
		
		if (!strcmp(p, argv[1]))
			return 0;
	}
	
	warnx("goto: %s: Not found", argv[1]);
	return ERR_NOLABEL;
}

static int cmd_umask(int argc, char **argv)
{
	mode_t m;
	
	if (argc < 2)
	{
		m = umask(0777);
		umask(m);
		printf("0%03o\n", (unsigned)m);
		return 0;
	}
	if (argc > 2)
	{
		warnx("umask: wrong nr of args");
		return ERR_ARGC;
	}
	m = strtoul(argv[1], NULL, 8);
	umask(m);
	return 0;
}

static int cmd_wait(int argc, char **argv)
{
	return ERR_SYNTAX;
}

static int cmd_shift(int argc, char **argv)
{
	if (!sargc)
		return ERR_ARGC;
	sargc--;
	sargv++;
	return 0;
}

static int cmd_login(int argc, char **argv)
{
	if (!l_flag)
	{
		warnx("Not a login shell");
		return ERR_UNK;
	}
	execl(_PATH_B_LOGIN, _PATH_B_LOGIN, NULL);
	warn("login: execl: %s", _PATH_B_LOGIN);
	return ERR_EXEC;
}

static int cmd_newgrp(int argc, char **argv)
{
	if (!l_flag)
	{
		warnx("Not a login shell");
		return ERR_UNK;
	}
	execl(_PATH_B_NEWGRP, _PATH_B_NEWGRP, NULL);
	warn("newgrp: execl: %s", _PATH_B_NEWGRP);
	return ERR_EXEC;
}

static int cmd_source(int argc, char **argv)
{
	char *n;
	FILE *f;
	
	if (argc != 2)
	{
		warnx("%s: wrong nr of args", argv[0]);
		return ERR_ARGC;
	}
	
	f = fopen(argv[1], "r");
	if (f == NULL)
	{
		warn("%s: %s", argv[0], argv[1]);
		return ERR_OPEN;
	}
	fcntl(fileno(f), F_SETFD, FD_CLOEXEC);
	
	n = strdup(argv[1]);
	if (n == NULL)
	{
		warn("%s", argv[0]);
		fclose(f);
		return ERR_MEM;
	}
	
	if (sstk_push())
	{
		warn("%s", argv[0]);
		return ERR_MEM;
	}
	source_nmlc = 1;
	source_name = n;
	source = f;
	return 0;
}

static int cmd_export(int argc, char **argv)
{
	int i, n;
	
	if (argc <= 1)
	{
		for (i = 0; i < varc; i++)
			if (vars[i].exp)
				puts(vars[i].name);
		return 0;
	}
	
	lastst = 0;
	for (i = 1; i < argc; i++)
	{
		for (n = 0; n < varc; n++)
			if (!strcmp(argv[i], vars[n].name))
			{
				vars[n].exp = 1;
				break;
			}
		if (n >= varc && var_set(argv[i], "", 1))
		{
			warn("%s", argv[i]);
			lastst = ERR_MEM;
		}
	}
	return lastst;
}

static int cmd_unset(int argc, char **argv)
{
	int i;
	
	if (argc < 2)
	{
		warnx("%s: wrong nr of args\n", argv[0]);
		return ERR_ARGC;
	}
	for (i = 1; i < argc; i++)
		var_unset(argv[i]);
	return 0;
}

static int cmd_exec(int argc, char **argv)
{
	do_exec(argv[1], argv + 1, NULL);
	return ERR_EXEC;
}

static int cmd_rtrap(int argc, char **argv)
{
	int i;
	
	for (i = 1; i <= NSIG; i++)
		signal(i, SIG_DFL);
	return 0;
}

static const struct spec_char *spec_char(char c)
{
	int i;
	
	for (i = 0; i < sizeof spec_chars / sizeof *spec_chars; i++)
		if (*spec_chars[i].ch == c)
			return &spec_chars[i];
	return NULL;
}

static struct token *add_tk(struct token *tkt, int cnt, int type, int par, char *str, int vas)
{
	tkt = realloc(tkt, (cnt + 2) * sizeof *tkt);
	if (!tkt)
		return NULL;
	tkt[cnt].type = type;
	tkt[cnt].par  = par;
	tkt[cnt].str  = str;
	tkt[cnt].vas  = vas;
	return tkt;
}

static void tk_dump(struct token *tk)
{
}

static struct token *tokenize(char *cmd)
{
	const struct spec_char *sc;
	struct token *tk = NULL;
	int cnt = 0;
	int pquot;
	int quot;
	int vas;
	char *d;
	char *e;
	char *p;
	
	for (p = cmd; *p; p++)
	{
		while (isspace(*p))
			p++;
		if (!*p)
			break;
		
		sc = spec_char(*p);
		if (sc != NULL)
		{
			tk = add_tk(tk, cnt++, sc->token, sc->parity, sc->ch, 0);
			if (!tk)
				return NULL;
			continue;
		}
		
		pquot = vas = 0;
		for (d = e = p; *e && !isspace(*e) && !(sc = spec_char(*e)); e++)
			switch (*e)
			{
			case '\\':
				if (!*++e)
					goto syn_err;
				*d++ = *e;
				continue;
			case '\'':
			case '\"':
				pquot = 1;
				quot = *e;
				e++;
				while (*e && *e != quot)
					*d++ = *e++;
				if (!*e)
					goto syn_err;
				break;
			case '=':
				if (!pquot)
					vas = 1;
			default:
				*d++ = *e;
			}
		tk = add_tk(tk, cnt++, TK_STRING, 0, p, vas);
		
		if (sc)
		{
			tk = add_tk(tk, cnt++, sc->token, sc->parity, sc->ch, 0);
			if (!tk)
				return NULL;
		}
		if (!*e)
		{
			*d = 0;
			break;
		}
		p  = e;
		*d = 0;
	}
	if (!tk)
	{
		tk = malloc(sizeof *tk);
		if (tk == NULL)
			return NULL;
	}
	tk[cnt].type = 0;
	tk_dump(tk);
	return tk;
syn_err:
	warnx("Syntax error");
	errno = 0;
	return NULL;
}

static void promptchar(char c)
{
	if (promptp < lastprompt + sizeof lastprompt - 1)
		*promptp++ = c;
	putchar(c);
}

static void prompt(const char *s)
{
	promptp = lastprompt;
	
	while (*s)
	{
		switch (*s)
		{
		case '\\':
			switch (*++s)
			{
			case '$':
				if (uid)
				{
					if (v6_flag)
						promptchar('%');
					else
						promptchar('%');
				}
				else
					promptchar('#');
				break;
			case 0:
				promptchar('\\');
				break;
			default:
				promptchar('\\');
				promptchar(*s);
				break;
			}
			break;
		default:
			promptchar(*s);
		}
		s++;
	}
	
	*promptp = 0;
}

static int waitexit(int st)
{
	if (WTERMSIG(st))
		return 128 + WTERMSIG(st);
	return WEXITSTATUS(st);
}

static char *readln(void)
{
	static char buf[4096];
	
	const char *ps1;
	const char *ps2;
	char *p;
	int c;
	
	ps1 = var_get("PS1");
	ps2 = var_get("PS2");
	if (ps1 == NULL)
		ps1 = "\\$ ";
	if (ps2 == NULL)
		ps2 = "> ";
	if (v6_flag)
		ps2 = "";
	
	if (source == stdin)
		prompt(ps1);
	p = buf;
	while (c = fgetc(source), c != EOF && c != '\n')
	{
		if (c == '#' && !v6_flag)
		{
			while (c = fgetc(source), c != EOF && c != '\n');
			if (c == EOF)
				break;
			ungetc(c, source);
			continue;
		}
		if (c == '\\')
		{
			c = fgetc(source);
			if (c == EOF)
				break;
			if (c == '\n')
			{
				if (v6_flag)
				{
					if (p >= buf + sizeof buf - 1)
						goto too_long;
					*p++ = ' ';
				}
				if (source == stdin)
					prompt(ps2);
				continue;
			}
			ungetc(c, source);
			c = '\\';
		}
		if (p >= buf + sizeof buf - 1)
			goto too_long;
		*p++ = c;
	}
	
	*lastprompt = 0;
	
	if (c == EOF)
	{
		if (ferror(source))
			warn("%s", source_name);
		return NULL;
	}
	*p = 0;
	return buf;
too_long:
	*lastprompt = 0;
	errno = ENOMEM;
	return NULL;
}

static void pstatus(const char *prog, int st)
{
	int sig = WTERMSIG(st);
	
	if (sig && sig != SIGINT)
		warnx("%s: %s%s", prog, strsignal(sig),
			WCOREDUMP(st) ? " (core dumped)" : "");
}

static struct token *skip_over(const struct token *tk, const struct token *e)
{
	int nest = 1;
	int l, r;
	
	r = tk->type;
	l = tk->par;
	if (!l)
		abort();
	
	for (tk++; nest; tk++)
	{
		if (tk >= e)
			return NULL;
		
		if (tk->type == l)
		{
			nest--;
			continue;
		}
		if (tk->type == r)
		{
			nest++;
			continue;
		}
		if (tk->par)
		{
			tk = skip_over(tk, e);
			if (tk == NULL)
				return NULL;
		}
	}
	return (struct token *)tk - 1;
}

static struct token *skip_redir(const struct token *tk, const struct token *e)
{
	while (tk < e)
		switch (tk++->type)
		{
		case TK_OREDIR:
		case TK_IREDIR:
			tk++;
			break;
		default:
			return NULL;
		}
	return (struct token *)tk;
}

static void do_exec(const char *name, char *const argv[], char *const tpev[])
{
	const char *pathname;
	char **envp;
	size_t sz;
	int cnt;
	int i;
	
	if (tpev == NULL)
		tpev = (char *const []){ NULL };
	
	pathname = _findexec(name, var_get("PATH"));
	if (pathname == NULL)
	{
		if (errno == ENOENT)
			warnx("%s: Not found", name);
		else
			warn("%s", name);
		return;
	}
	
	for (cnt = 0, i = 0; i < varc; i++)
		if (vars[i].exp)
			cnt++;
	for (i = 0; tpev[i] != NULL; i++)
		cnt++;
	
	envp = calloc(sizeof *envp, cnt + 1);
	if (envp == NULL)
	{
		warn(NULL);
		return;
	}
	
	for (cnt = 0, i = 0; i < varc; i++)
		if (vars[i].exp)
		{
			sz  = strlen(vars[i].name);
			sz += strlen(vars[i].value);
			sz += 2;
			
			envp[cnt] = malloc(sz);
			if (envp[cnt] == NULL)
				goto clean;
			
			strcpy(envp[cnt], vars[i].name);
			strcat(envp[cnt], "=");
			strcat(envp[cnt], vars[i].value);
			cnt++;
		}
	for (i = 0; tpev[i] != NULL; i++)
		envp[cnt++] = tpev[i];
	
	execve(pathname, argv, envp);
clean:
	warn(NULL);
	
	for (i = 0; i < cnt; i++)
		free(envp[i]);
	free(envp);
}

static void do_comm1(const struct token *s, const struct token *e, int no_wait, const char *iredir, const char *oredir)
{
	const struct token *tk, *rp;
	int do_glob = 0;
	char **argv = NULL;
	char **tpev = NULL;
	int ifd = 0, ofd = 1;
	int oi = -1, oo = -1;
	pid_t pid;
	int argc;
	char *p;
	int coms = 0;
	int vas;
	int i;
	
	if (!s->type)
		return;
	
	tk = s;
	if (tk->type == TK_LBRACE || tk->type == TK_LPAREN)
	{
		tk = skip_over(tk, e);
		if (tk == NULL)
			goto syn_err;
		tk++;
	}
	
	vas = 1;
	for (argc = 0; tk < e; tk++)
		switch (tk->type)
		{
		case TK_STRING:
			if (!vas || !tk->vas)
			{
				p = tk->str + strcspn(tk->str, GLOB_CHRS);
				if (*p)
					do_glob = 1;
				coms = 1;
				vas = 0;
			}
			argc++;
			break;
		case TK_IREDIR:
		case TK_OREDIR:
			tk++;
			break;
		default:
			argc++;
		}
	if (iredir != NULL)
	{
		ifd = open(iredir, O_RDONLY);
		if (ifd < 0)
		{
			warn("%s: open", iredir);
			goto clean;
		}
		fcntl(ifd, F_SETFD, 1);
	}
	if (oredir != NULL)
	{
		ofd = open(oredir, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (ofd < 0)
		{
			warn("%s: open", oredir);
			goto clean;
		}
		fcntl(ofd, F_SETFD, 1);
	}
	
	if (s->type == TK_LBRACE)
	{
		rp = skip_over(s, e);
		if (rp == NULL)
			goto syn_err;
		if (skip_redir(rp + 1, e) != e)
			goto syn_err;
		do_comm(s + 1, rp);
		goto clean;
	}
	if (s->type == TK_LPAREN)
	{
		rp = skip_over(s, e);
		if (rp == NULL)
			goto syn_err;
		if (skip_redir(rp + 1, e) != e)
			goto syn_err;
		pid = fork();
		if (pid < 0)
		{
			warn("fork");
			goto clean;
		}
		if (!pid)
		{
			if (ifd != 0)
				dup2(ifd, 0);
			if (ofd != 1)
				dup2(ofd, 1);
			do_comm(s + 1, rp);
			_exit(lastst);
		}
		goto do_wait;
	}
	
	if (!argc)
		goto clean;
	
	if (!v6_flag && !coms)
	{
		for (tk = s; tk < e; tk++)
			if (tk->type == TK_STRING)
				if (var_assign(tk->str, 0))
				{
					warn("var_assign");
					lastst = ERR_MEM;
				}
		goto clean;
	}
	
	argv = calloc(sizeof *argv, argc + 3);
	tpev = argv + argc + 2;
	if (argv == NULL)
	{
		lastst = ERR_MEM;
		warn("calloc");
		goto clean;
	}
	
	i = 0;
	if (do_glob)
		argv[i++] = _PATH_B_GLOB;
	
	vas = 1;
	for (tk = s; tk < e; tk++)
		switch (tk->type)
		{
		case TK_STRING:
			if (!vas || !tk->vas)
				vas = 0;
			else
			{
				if (tpev <= argv + i + 1)
					abort();
				*--tpev = tk->str;
				break;
			}
			/* if (i >= argc + 1)
				abort(); */
			if (argv + i + 1 >= tpev)
				abort();
			argv[i++] = tk->str;
			break;
		case TK_IREDIR:
		case TK_OREDIR:
			tk++;
			break;
		default:
			if (argv + i + 1 >= tpev)
				abort();
			argv[i++] = tk->str;
			break;
		}
	argv[i] = NULL;
	
	for (i = 0; i < sizeof comms / sizeof *comms; i++)
	{
		if (v6_flag && !comms[i].v6)
			continue;
		if (!strcmp(comms[i].name, argv[0]))
		{
			if (ifd != 0)
			{
				if (oi = dup(0), oi < 0)
					goto dup_err;
				fcntl(oi, F_SETFD, 1);
				dup2(ifd, 0);
			}
			if (ofd != 1)
			{
				if (oo = dup(1), oo < 0)
					goto dup_err;
				fcntl(oo, F_SETFD, 1);
				dup2(ofd, 1);
			}
			lastst = comms[i].proc(argc, argv);
			goto clean;
		}
	}
	
	pid = fork();
	if (pid < 0)
	{
		warn("fork");
		lastst = ERR_FORK;
		return;
	}
	if (!pid)
	{
		if (ifd != 0)
			dup2(ifd, 0);
		if (ofd != 1)
			dup2(ofd, 1);
		signal(SIGQUIT, sigquit);
		signal(SIGTERM, sigterm);
		signal(SIGINT, sigint);
		do_exec(argv[0], argv, tpev);
		_exit(ERR_EXEC);
	}
do_wait:
	if (!no_wait)
	{
		while (wait(&lastst) != pid);
		if (argc)
			pstatus(argv[0], lastst);
		else
			pstatus("subshell", lastst);
		lastst = waitexit(lastst);
	}
	goto clean;
syn_err:
	warnx("Syntax error.");
	lastst = ERR_SYNTAX;
	stop = 1;
	goto clean;
dup_err:
	warnx("dup");
	goto clean;
clean:
	if (oi >= 0)
	{
		dup2(oi, 0);
		close(oi);
	}
	if (oo >= 1)
	{
		dup2(oo, 1);
		close(oo);
	}
	if (ifd != 0)
		close(ifd);
	if (ofd != 1)
		close(ofd);
	if (argv != NULL)
		free(argv);
}

static void do_comm(const struct token *tk, const struct token *e)
{
	const char *iredir = NULL;
	const char *oredir = NULL;
	const struct token *com;
	const struct token *tkp;
	int argc = 0;
	int oi = -1, oo = -1;
	int pp[2];
	
	if (e == NULL)
		for (e = tk; e->type; e++)
			;
	
	stop = 0;
	for (com = tkp = tk; tkp < e && !stop; tkp++)
	{
		switch (tkp->type)
		{
		case TK_STRING:
			break;
		case TK_PIPE:
			if (oi < 0)
			{
				oi = dup(0);
				if (oi < 0)
					goto dup_err;
				fcntl(oi, F_SETFD, 1);
			}
			if (oo < 0)
			{
				oo = dup(1);
				if (oo < 0)
					goto dup_err;
				fcntl(oo, F_SETFD, 1);
			}
			
			if (pipe(pp))
			{
				warn("pipe");
				goto clean;
			}
			fcntl(pp[0], F_SETFD, 1);
			fcntl(pp[1], F_SETFD, 1);
			
			dup2(pp[1], 1);
			close(pp[1]);
			
			do_comm1(com, tkp, 1, NULL, NULL);
			
			dup2(pp[0], 0);
			close(pp[0]);
			dup2(oo, 1);
			
			iredir = oredir = NULL;
			com    = tkp + 1;
			argc   = 0;
			break;
		case TK_BACKGROUND:
		case TK_SEMICOLON:
			if (oi >= 0)
			{
				dup2(oi, 0);
				close(oi);
				oi = -1;
			}
			if (oo >= 0)
			{
				dup2(oo, 1);
				close(oo);
				oo = -1;
			}
			
			do_comm1(com, tkp, tkp->type == TK_BACKGROUND, iredir, oredir);
			iredir = oredir = NULL;
			com    = tkp + 1;
			argc   = 0;
			break;
		case TK_IREDIR:
			if ((++tkp)->type != TK_STRING)
				goto syn_err;
			iredir = tkp->str;
			break;
		case TK_OREDIR:
			if ((++tkp)->type != TK_STRING)
				goto syn_err;
			oredir = tkp->str;
			break;
		case TK_LPAREN:
		case TK_LBRACE:
			if (u_flag && com != tkp)
				break;
			tkp = skip_over(tkp, e);
			if (tkp == NULL)
				goto syn_err;
			break;
		case TK_RPAREN:
		case TK_RBRACE:
			if (u_flag)
				break;
			goto syn_err;
		default:
			abort();
		}
	}
	do_comm1(com, tkp, 0, iredir, oredir);
	goto clean;
dup_err:
	warn("dup");
	goto clean;
syn_err:
	warnx("Syntax error.");
	lastst = ERR_SYNTAX;
	stop = 1;
clean:
	if (oi >= 0)
	{
		dup2(oi, 0);
		close(oi);
	}
	if (oo >= 0)
	{
		dup2(oo, 1);
		close(oo);
	}
}

static int vname_char(char ch)
{
	if (isalnum(ch))
		return 1;
	if (ch == '_')
		return 1;
	return 0;
}

static const char *do_subst_1(char *s, size_t *rlen)
{
	static char buf[64];
	
	const char *v = "";
	char *e;
	int sc;
	int c;
	int i;
	
	c = *++s;
	if (isdigit(c))
	{
		*rlen = 2;
		i = c - '0';
		if (!i)
		{
			if (source != stdin)
				return source_name;
			return arg0;
		}
		if (i >= sargc)
			return "";
		return sargv[i];
	}
	else if (c == '$')
	{
		if (v6_flag)
			sprintf(buf, "%05u", (unsigned)getpid());
		else
			sprintf(buf, "%u", (unsigned)getpid());
		*rlen = 2;
		return buf;
	}
	else if (v6_flag)
	{
		*rlen = 1;
		return "$";
	}
	else if (c == '?')
	{
		sprintf(buf, "%i", lastst);
		*rlen = 2;
		return buf;
	}
	else if (c == '{')
	{
		for (e = s + 1; *e && *e != '}'; e++);
		sc = *e;
		*e = 0;
		v  = var_get(s + 1);
		*e = sc;
		if (v == NULL)
			v = "";
		*rlen = e - s + 2;
		return v;
	}
	for (e = s; *e && vname_char(*e); e++);
	sc = *e;
	*e = 0;
	v  = var_get(s);
	*e = sc;
	if (v == NULL)
		v = "";
	*rlen = e - s + 1;
	return v;
}

static char *do_subst(char *src)
{
#define GROW_BUF(ncap)					\
	do						\
	{						\
		size_t nc = ncap;			\
		char *n;				\
							\
		if (nc >= cap)				\
		{					\
			nc += 16;			\
			nc &= ~(size_t)15;		\
			n = realloc(buf, nc);		\
			if (n == NULL)			\
			{				\
				se = errno;		\
				free(buf);		\
				errno = se;		\
				return NULL;		\
			}				\
			cap = nc;			\
			buf = n;			\
		}					\
	} while (0)
	
	const char *s;
	char *buf;
	char *sp;
	int quot = 0;
	size_t cap;
	size_t ssl;
	size_t svl;
	size_t i;
	int se;
	
	if (!*src)
		return strdup("");
	
	buf = NULL;
	cap = 0;
	i   = 0;
	sp  = src;
	while (*sp)
	{
		if (!quot && (*sp == '\'' || (*sp == '\"' && v6_flag)))
			quot = *sp;
		else if (*sp == quot)
			quot = 0;
		
		if (sp[0] == '\\' && sp[1])
		{
			GROW_BUF(i + 2);
			buf[i++] = sp[0];
			buf[i++] = sp[1];
			sp += 2;
			continue;
		}
		
		if (!quot && *sp == '$')
		{
			s   = do_subst_1(sp, &ssl);
			svl = strlen(s);
			GROW_BUF(i + svl);
			memcpy(buf + i, s, svl);
			sp += ssl;
			i  += svl;
			continue;
		}
		GROW_BUF(i + 1);
		buf[i++] = *sp++;
	}
	buf[i] = 0;
	return buf;
#undef GROW_BUF
}

static int do_line(char *ln)
{
	struct token *tk;
	char *p;
	int se;
	
	p = do_subst(ln);
	if (p == NULL)
	{
		warn("cannot substitute");
		return -1;
	}
	
	errno = 0;
	tk = tokenize(p);
	if (tk == NULL)
	{
		if (errno)
		{
			se = errno;
			warn("cannot parse"); /* XXX error */
			free(p);
			errno = se;
			return -1;
		}
		free(p);
		lastst = ERR_SYNTAX;
		return 0;
	}
	do_comm(tk, 0);
	free(tk);
	free(p);
	return 0;
}

static void loop(void)
{
	char *p;
	
	do
		while (p = readln(), p)
			if (do_line(p))
				return;
	while (source_stk && (sstk_pop(), 1));
}

static void try_profile(const char *pathname, int mand)
{
	FILE *f;
	
	f = fopen(pathname, "r");
	if (f == NULL)
	{
		if (errno == ENOENT)
			return;
		if (mand)
			err(ERR_OPEN, "%s", pathname);
		warn("%s", pathname);
		return;
	}
	if (sstk_push())
		err(1, "sstk_push");
	source_name = _PATH_E_PROFILE;
	source	    = f;
}

static void profile(void)
{
	char *p;
	
	p = getenv("HOME");
	if (p != NULL)
	{
		if (asprintf(&p, "%s/" UPROFILE, p) < 0)
			err(ERR_MEM, "asprintf");
		try_profile(p, 0);
		free(p);
	}
	try_profile(_PATH_E_PROFILE, 1);
}

static const char *var_get(const char *name)
{
	int i;
	
	if (v6_flag)
		return getenv(name);
	
	if (strchr(name, '=') != NULL)
		return NULL;
	
	for (i = 0; i < varc; i++)
		if (!strcmp(name, vars[i].name))
			return vars[i].value;
	return NULL;
}

static int var_set(const char *name, const char *value, int exp)
{
	struct var *nv;
	char *n, *v;
	int i;
	
	if (v6_flag)
		return setenv(name, value, 1);
	
	if (strchr(name, '=') != NULL)
	{
		errno = EINVAL;
		return -1;
	}
	
	v = strdup(value);
	if (v == NULL)
		return -1;
	
	for (i = 0; i < varc; i++)
		if (!strcmp(name, vars[i].name))
		{
			free(vars[i].value);
			vars[i].value = v;
			return 0;
		}
	
	n = strdup(name);
	if (n == NULL)
	{
		free(v);
		return -1;
	}
	
	nv = realloc(vars, sizeof *vars * (varc + 1));
	if (nv == NULL)
	{
		free(n);
		free(v);
		return -1;
	}
	vars = nv;
	
	vars[varc].name  = n;
	vars[varc].value = v;
	vars[varc].exp   = exp;
	varc++;
	return 0;
}

static int var_unset(const char *name)
{
	int i;
	
	if (strchr(name, '=') != NULL)
	{
		errno = EINVAL;
		return -1;
	}
	
	for (i = 0; i < varc; i++)
		if (!strcmp(name, vars[i].name))
		{
			for (; i < varc - 1; i++)
				vars[i] = vars[i + 1];
			varc--;
			return 0;
		}
	errno = EINVAL;
	return -1;
}

static int var_assign(const char *nameval, int exp)
{
	struct var *nv;
	char *n = NULL;
	char *v = NULL;
	char *q;
	int se;
	int i;
	
	if (v6_flag)
	{
		n = strdup(nameval);
		if (n == NULL)
			return -1;
		if (putenv(n))
			goto clean;
		return 0;
	}
	
	q = strchr(nameval, '=');
	if (q == NULL)
	{
		errno = EINVAL;
		return -1;
	}
	
	v = strdup(q + 1);
	if (v == NULL)
		return -1;
	
	for (i = 0; i < varc; i++)
		if (!strncmp(nameval, vars[i].name, q - nameval) && vars[i].name[q - nameval] == 0)
		{
			free(vars[i].value);
			vars[i].value = v;
			return 0;
		}
	
	n = malloc(q - nameval + 1);
	if (n == NULL)
		goto clean;
	memcpy(n, nameval, q - nameval);
	n[q - nameval] = 0;
	
	nv = realloc(vars, sizeof *vars * (varc + 1));
	if (nv == NULL)
		goto clean;
	vars = nv;
	
	vars[varc].name  = n;
	vars[varc].value = v;
	vars[varc].exp   = exp;
	varc++;
	return 0;
clean:
	se = errno;
	free(n);
	free(v);
	errno = se;
	return -1;
}

static void var_fetch(void)
{
	char **p;
	
	if (v6_flag)
		return;
	
	for (p = environ; *p; p++)
		if (var_assign(*p, 1))
			err(ERR_MEM, "var_assign");
}

static void siginth(int nr)
{
	write(1, lastprompt, strlen(lastprompt));
	
	signal(nr, siginth);
}

int main(int argc, char **argv)
{
	int c;
	
	source_name = "stdin";
	source	    = stdin;
	
	arg0 = argv[0];
	uid  = getuid();
	
	var_fetch();
	
	while (c = getopt(argc, argv, "6c:tl"), c > 0)
		switch (c)
		{
		case '6':
			v6_flag = 1;
			break;
		case 'c':
			if (do_line(optarg))
				return ERR_UNK;
			if (source_stk)
			{
				sstk_pop();
				loop();
			}
			return waitexit(lastst);
		case 'l':
			l_flag = 1;
			break;
		default:
			return ERR_ARGS;
		}
	
	if (optind < argc)
	{
		source_name = argv[optind];
		source = fopen(source_name, "r");
		if (source == NULL)
			err(1, "%s: fopen", source_name);
		fcntl(fileno(source), F_SETFD, FD_CLOEXEC);
		sargv = argv + optind;
		sargc = argc - optind;
	}
	
	if (l_flag)
		profile();
	
	sigquit = signal(SIGQUIT, SIG_IGN);
	sigterm = signal(SIGTERM, SIG_IGN);
	sigint  = signal(SIGINT,  siginth);
	
	loop();
	return waitexit(lastst);
}
