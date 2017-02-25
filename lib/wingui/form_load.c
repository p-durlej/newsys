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

#include <wingui_form.h>
#include <wingui_dlg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <os386.h>

#define DEBUG		0

#define TK_NULL		1
#define TK_STRING	2
#define TK_UNQUOTED	3
#define TK_NUMBER	4
#define TK_EOF		5

#define	stderr		(_get_stderr())

struct string
{
	int buf_len;
	int str_len;
	char *buf;
};

struct scale
{
	int x_num, x_den;
	int y_num, y_den;
};

#define RESCALE_X(x, sc)	{ (x) *= (sc)->x_num; (x) /= (sc)->x_den; }
#define RESCALE_Y(y, sc)	{ (y) *= (sc)->y_num; (y) /= (sc)->y_den; }

struct token
{
	int	type;
	
	union
	{
		char *	string;
		int	number;
	} v;
};

static void init_str(struct string *str)
{
	str->buf_len = 0;
	str->str_len = 0;
	str->buf     = NULL;
}

static void free_str(struct string *str)
{
	free(str->buf);
}

static int grow_str(struct string *str, char ch)
{
	int new_len;
	char *n;
	
	new_len  = str->str_len + 17;
	new_len &= ~15;
	
	if (new_len != str->buf_len)
	{
		n = realloc(str->buf, new_len);
		if (!n)
			return -1;
		
		str->buf_len = new_len;
		str->buf     = n;
	}
	
	str->buf[str->str_len++] = ch;
	str->buf[str->str_len]   = 0;
	return 0;
}

static int get_token(FILE *f, struct token *tk)
{
	struct string str;
	char ch;
	
	init_str(&str);
	
restart:
	while (ch = fgetc(f), isspace(ch));
	if (ferror(f))
		return -1;
	if (feof(f))
	{
		tk->type = TK_EOF;
		return 0;
	}
	
	if (ch == '#')
	{
		while (ch = fgetc(f), ch != EOF && ch != '\n');
		if (ferror(f))
			return -1;
		if (feof(f))
		{
			tk->type = TK_EOF;
			return 0;
		}
		goto restart;
	}
	
	if (isdigit(ch) || ch == '-')
	{
		int neg = 0;
		int nr  = 0;
		
		if (ch == '-')
			neg = 1;
		else
			nr = ch - '0';
		
		while (ch = fgetc(f), isdigit(ch))
		{
			nr *= 10;
			nr += ch - '0';
		}
		if (ferror(f))
			return -1;
		if (feof(f))
		{
			_set_errno(EINVAL);
			return -1;
		}
		ungetc(ch, f);
		
		tk->type     = TK_NUMBER;
		tk->v.number = neg ? - nr : nr;
		return 0;
	}
	
	if (ch == '\"')
	{
		while (ch = fgetc(f), ch != '\"' && ch != EOF)
		{
			if (ch == '\\')
			{
				ch = fgetc(f);
				if (ch == EOF)
					break;
				
				switch (ch)
				{
				case 'n':
					ch = '\n';
					break;
				case '\\':
				default:
					;
				}
			}
			if (grow_str(&str, ch))
			{
				free_str(&str);
				return -1;
			}
		}
		if (ferror(f))
		{
			free_str(&str);
			return -1;
		}
		if (feof(f))
		{
			free_str(&str);
			_set_errno(EINVAL);
			return -1;
		}
		
		tk->type     = TK_STRING;
		tk->v.string = str.buf ? str.buf : strdup("");
		if (!tk->v.string)
			return -1;
		return 0;
	}
	
	if (grow_str(&str, ch))
	{
		free_str(&str);
		return -1;
	}
	
	if (ch == '(' || ch == ')' || ch == ',' || ch == ';')
	{
		tk->type     = TK_UNQUOTED;
		tk->v.string = str.buf ? str.buf : strdup("");
		if (!tk->v.string)
			return -1;
		return 0;
	}
	
	while (ch = fgetc(f), !isspace(ch) &&
		ch != '(' && ch != ')' &&
		ch != ',' && ch != ';' &&
		ch != '-' && ch != EOF)
	{
		if (grow_str(&str, ch))
		{
			free_str(&str);
			return -1;
		}
	}
	if (ferror(f))
	{
		free_str(&str);
		return -1;
	}
	if (feof(f))
	{
		free_str(&str);
		_set_errno(EINVAL);
		return -1;
	}
	ungetc(ch, f);
	
	if (str.buf && !strcmp(str.buf, "NULL"))
	{
		tk->type = TK_NULL;
		free_str(&str);
		return 0;
	}
	
	tk->type     = TK_UNQUOTED;
	tk->v.string = str.buf ? str.buf : strdup("");
	if (!tk->v.string)
		return -1;
	return 0;
}

static void free_token(struct token *tk)
{
	if (tk->type == TK_UNQUOTED || tk->type == TK_STRING)
		free(tk->v.string);
}

static int skip_comma(FILE *f)
{
	struct token tk;
	
	if (get_token(f, &tk))
		return -1;
	if (tk.type != TK_UNQUOTED || strcmp(tk.v.string, ","))
	{
		_set_errno(EINVAL);
		free_token(&tk);
		return -1;
	}
	free_token(&tk);
	return 0;
}

static int get_number(FILE *f, int skcom, int *buf)
{
	struct token tk;
	
	if (skcom && skip_comma(f))
		return -1;
	
	if (get_token(f, &tk))
		return -1;
	if (tk.type != TK_NUMBER)
	{
		_set_errno(EINVAL);
		return -1;
	}
	*buf = tk.v.number;
	free_token(&tk);
	return 0;
}

static int get_nstring(FILE *f, int skcom, char **buf)
{
	struct token tk;
	
	if (skcom && skip_comma(f))
		return -1;
	
	if (get_token(f, &tk))
		return -1;
	if (tk.type == TK_NULL)
	{
		*buf = NULL;
		return 0;
	}
	if (tk.type != TK_STRING)
	{
		_set_errno(EINVAL);
		return -1;
	}
	*buf = tk.v.string;
	return 0;
}

static int get_string(FILE *f, int skcom, char **buf)
{
	char *s;
	
	if (get_nstring(f, skcom, &s))
		return -1;
	if (!s)
	{
		_set_errno(EINVAL);
		return -1;
	}
	*buf = s;
	return 0;
}

static void adjust_scale(struct scale *sc, int w, int h)
{
	int fw, fh;
	
	win_text_size(WIN_FONT_DEFAULT, &fw, &fh, "M");
	sc->x_num = fw;
	sc->y_num = fh;
	sc->x_den = w;
	sc->y_den = h;
}

static int do_font(FILE *f, struct form *frm, struct scale *sc)
{
	int w, h;
	
	if (get_number(f, 0, &w))		return -1;
	if (get_number(f, 1, &h))		return -1;
	adjust_scale(sc, w, h);
	return 0;
}

static int do_label(FILE *f, struct form *frm, struct scale *sc)
{
	struct gadget *g;
	char *label = NULL;
	char *name;
	int x, y;
	
	if (get_nstring(f, 0, &name))	return -1;
	if (get_number(f, 1, &x))	goto fail;
	if (get_number(f, 1, &y))	goto fail;
	if (get_string(f, 1, &label))	goto fail;
	
#if DEBUG
	_cprintf("label(\"%s\", %i, %i, \"%s\");\n", name, x, y, label);
#endif
	
	RESCALE_X(x, sc);
	RESCALE_Y(y, sc);
	
	g = label_creat(frm, x, y, label);
	if (!g)
		goto fail;
	g->name = name;
	
	free(label);
	return 0;
fail:
	free(label);
	free(name);
	return -1;
}

static int do_button(FILE *f, struct form *frm, struct scale *sc)
{
	struct gadget *g;
	char *label = NULL;
	char *name;
	int x, y, w, h;
	int res;
	
	if (get_nstring(f, 0, &name))	return -1;
	if (get_number(f, 1, &x))	goto fail;
	if (get_number(f, 1, &y))	goto fail;
	if (get_number(f, 1, &w))	goto fail;
	if (get_number(f, 1, &h))	goto fail;
	if (get_string(f, 1, &label))	goto fail;
	if (get_number(f, 1, &res))	goto fail;
	
#if DEBUG
	_cprintf("button(\"%s\", %i, %i, %i, %i, \"%s\", %i);\n", name, x, y, w, h, label, res);
#endif
	
	RESCALE_X(x, sc);
	RESCALE_Y(y, sc);
	RESCALE_X(w, sc);
	RESCALE_Y(h, sc);
	
	g = button_creat(frm, x, y, w, h, label, NULL);
	if (!g)
		goto fail;
	button_set_result(g, res);
	g->name = name;
	
	free(label);
	return 0;
fail:
	free(label);
	free(name);
	return -1;
}

static int do_chkbox(FILE *f, struct form *frm, struct scale *sc)
{
	struct gadget *g;
	char *label = NULL;
	char *name;
	int x, y;
	
	if (get_nstring(f, 0, &name))	return -1;
	if (get_number(f, 1, &x))	goto fail;
	if (get_number(f, 1, &y))	goto fail;
	if (get_string(f, 1, &label))	goto fail;
	
#if DEBUG
	_cprintf("chkbox(\"%s\", %i, %i, \"%s\");\n", name, x, y, label);
#endif
	
	RESCALE_X(x, sc);
	RESCALE_Y(y, sc);
	
	g = chkbox_creat(frm, x, y, label);
	if (!g)
		goto fail;
	g->name = name;
	
	free(label);
	return 0;
fail:
	free(label);
	free(name);
	return -1;
}

static int do_input(FILE *f, struct form *frm, struct scale *sc)
{
	struct gadget *g;
	char *name;
	int x, y, w, h;
	int flags;
	
	if (get_nstring(f, 0, &name))	return -1;
	if (get_number(f, 1, &x))	goto fail;
	if (get_number(f, 1, &y))	goto fail;
	if (get_number(f, 1, &w))	goto fail;
	if (get_number(f, 1, &h))	goto fail;
	if (get_number(f, 1, &flags))	goto fail;
	
#if DEBUG
	_cprintf("input(\"%s\", %i, %i, %i, %i, %i);\n", name, x, y, w, h, flags);
#endif
	
	RESCALE_X(x, sc);
	RESCALE_Y(y, sc);
	RESCALE_X(w, sc);
	RESCALE_Y(h, sc);
	
	g = input_creat(frm, x, y, w, h);
	if (!g)
		goto fail;
	input_set_flags(g, flags);
	g->name = name;
	
	return 0;
fail:
	free(name);
	return -1;
}

static int do_list(FILE *f, struct form *frm, struct scale *sc)
{
	struct gadget *g;
	char *name;
	int x, y, w, h;
	int flags;
	
	if (get_nstring(f, 0, &name))	return -1;
	if (get_number(f, 1, &x))	goto fail;
	if (get_number(f, 1, &y))	goto fail;
	if (get_number(f, 1, &w))	goto fail;
	if (get_number(f, 1, &h))	goto fail;
	if (get_number(f, 1, &flags))	goto fail;
	
#if DEBUG
	_cprintf("input(\"%s\", %i, %i, %i, %i, %i);\n", name, x, y, w, h, flags);
#endif
	
	RESCALE_X(x, sc);
	RESCALE_Y(y, sc);
	RESCALE_X(w, sc);
	RESCALE_Y(h, sc);
	
	g = list_creat(frm, x, y, w, h);
	if (!g)
		goto fail;
	list_set_flags(g, flags);
	g->name = name;
	
	return 0;
fail:
	free(name);
	return -1;
}

static int do_picture(FILE *f, struct form *frm, struct scale *sc)
{
	struct gadget *g;
	char *file = NULL;
	char *name = NULL;
	int x, y, w, h;
	
	if (get_nstring(f, 0, &name))	return -1;
	if (get_number(f, 1, &x))	goto fail;
	if (get_number(f, 1, &y))	goto fail;
	if (get_nstring(f, 1, &file))	goto fail;
	
#if DEBUG
	_cprintf("picture(\"%s\", %i, %i, \"%s\");\n", name, x, y, file);
#endif
	
	RESCALE_X(x, sc);
	RESCALE_Y(y, sc);
	
	g = pict_creat(frm, x, y, file);
	if (!g)
		goto fail;
	g->name = name;
	
	w = g->rect.w;
	h = g->rect.h;
	
	RESCALE_X(w, sc);
	RESCALE_Y(h, sc);
	
	pict_scale(g, w, h);
	
	free(file);
	return 0;
fail:
	free(file);
	free(name);
	return -1;
}

static int do_frame(FILE *f, struct form *frm, struct scale *sc)
{
	struct gadget *g;
	char *label = NULL;
	char *name  = NULL;
	int x, y, w, h;
	
	if (get_nstring(f, 0, &name))	return -1;
	if (get_number(f, 1, &x))	goto fail;
	if (get_number(f, 1, &y))	goto fail;
	if (get_number(f, 1, &w))	goto fail;
	if (get_number(f, 1, &h))	goto fail;
	if (get_nstring(f, 1, &label))	goto fail;
	
#if DEBUG
	_cprintf("frame(\"%s\", %i, %i, %i, %i, \"%s\");\n", name, x, y, w, h, label);
#endif
	
	RESCALE_X(x, sc);
	RESCALE_Y(y, sc);
	RESCALE_X(w, sc);
	RESCALE_Y(h, sc);
	
	g = frame_creat(frm, x, y, w, h, label);
	if (!g)
		goto fail;
	g->name = name;
	
	free(label);
	return 0;
fail:
	free(label);
	free(name);
	return -1;
}

static int do_popup(FILE *f, struct form *frm, struct scale *sc)
{
	struct gadget *g;
	char *name = NULL;
	int x, y;
	
	if (get_nstring(f, 0, &name))	return -1;
	if (get_number(f, 1, &x))	goto fail;
	if (get_number(f, 1, &y))	goto fail;
	
#if DEBUG
	_cprintf("popup(\"%s\", %i, %i);\n", name, x, y);
#endif
	
	RESCALE_X(x, sc);
	RESCALE_Y(y, sc);
	
	g = popup_creat(frm, x, y);
	if (!g)
		goto fail;
	g->name = name;
	
	return 0;
fail:
	free(name);
	return -1;
}

static int do_hsbar(FILE *f, struct form *frm, struct scale *sc)
{
	struct gadget *g;
	char *name = NULL;
	int x, y, w, h;
	
	if (get_nstring(f, 0, &name))	return -1;
	if (get_number(f, 1, &x))	goto fail;
	if (get_number(f, 1, &y))	goto fail;
	if (get_number(f, 1, &w))	goto fail;
	if (get_number(f, 1, &h))	goto fail;
	
#if DEBUG
	_cprintf("hsbar(\"%s\", %i, %i, %i, %i);\n", name, x, y, w, h);
#endif
	
	RESCALE_X(x, sc);
	RESCALE_Y(y, sc);
	RESCALE_X(w, sc);
	RESCALE_Y(h, sc);
	
	g = hsbar_creat(frm, x, y, w, h);
	if (!g)
		goto fail;
	g->name = name;
	
	return 0;
fail:
	free(name);
	return -1;
}

static int do_vsbar(FILE *f, struct form *frm, struct scale *sc)
{
	struct gadget *g;
	char *name = NULL;
	int x, y, w, h;
	
	if (get_nstring(f, 0, &name))	return -1;
	if (get_number(f, 1, &x))	goto fail;
	if (get_number(f, 1, &y))	goto fail;
	if (get_number(f, 1, &w))	goto fail;
	if (get_number(f, 1, &h))	goto fail;
	
#if DEBUG
	_cprintf("vsbar(\"%s\", %i, %i, %i, %i);\n", name, x, y, w, h);
#endif
	
	RESCALE_X(x, sc);
	RESCALE_Y(y, sc);
	RESCALE_X(w, sc);
	RESCALE_Y(h, sc);
	
	g = vsbar_creat(frm, x, y, w, h);
	if (!g)
		goto fail;
	g->name = name;
	
	return 0;
fail:
	free(name);
	return -1;
}

static int do_colorsel(FILE *f, struct form *frm, struct scale *sc)
{
	struct gadget *g;
	char *name = NULL;
	int x, y, w, h;
	
	if (get_nstring(f, 0, &name))	return -1;
	if (get_number(f, 1, &x))	goto fail;
	if (get_number(f, 1, &y))	goto fail;
	if (get_number(f, 1, &w))	goto fail;
	if (get_number(f, 1, &h))	goto fail;
	
#if DEBUG
	_cprintf("colorsel(\"%s\", %i, %i, %i, %i);\n", name, x, y, w, h);
#endif
	
	RESCALE_X(x, sc);
	RESCALE_Y(y, sc);
	RESCALE_X(w, sc);
	RESCALE_Y(h, sc);
	
	g = colorsel_creat(frm, x, y, w, h);
	if (!g)
		goto fail;
	g->name = name;
	
	return 0;
fail:
	free(name);
	return -1;
}

static int do_bargraph(FILE *f, struct form *frm, struct scale *sc)
{
	struct gadget *g;
	char *name = NULL;
	int x, y, w, h;
	
	if (get_nstring(f, 0, &name))	return -1;
	if (get_number(f, 1, &x))	goto fail;
	if (get_number(f, 1, &y))	goto fail;
	if (get_number(f, 1, &w))	goto fail;
	if (get_number(f, 1, &h))	goto fail;
	
#if DEBUG
	_cprintf("bargraph(\"%s\", %i, %i, %i, %i, %i, %i);\n", name, x, y, w, h, v, l);
#endif
	
	RESCALE_X(x, sc);
	RESCALE_Y(y, sc);
	RESCALE_X(w, sc);
	RESCALE_Y(h, sc);
	
	g = bargraph_creat(frm, x, y, w, h);
	if (!g)
		goto fail;
	g->name = name;
	
	return 0;
fail:
	free(name);
	return -1;
}

static int do_sizebox(FILE *f, struct form *frm, struct scale *sc)
{
	struct gadget *g;
	char *name = NULL;
	int x, y, w, h;
	
	if (get_nstring(f, 0, &name))	return -1;
	if (get_number(f, 1, &w))	goto fail;
	if (get_number(f, 1, &h))	goto fail;
	
#if DEBUG
	_cprintf("sizebox(\"%s\", %i, %i);\n", name, w, h);
#endif
	
	RESCALE_X(w, sc);
	RESCALE_Y(h, sc);
	
	g = sizebox_creat(frm, w, h);
	if (!g)
		goto fail;
	g->name = name;
	
	return 0;
fail:
	free(name);
	return -1;
}

static const struct dispatch
{
	int (*func)(FILE *f, struct form *frm, struct scale *sc);
	char *name;
} dispatch[] =
{
	{ do_font,	"font"		},
	{ do_label,	"label"		},
	{ do_button,	"button"	},
	{ do_chkbox,	"chkbox"	},
	{ do_input,	"input"		},
	{ do_picture,	"picture"	},
	{ do_list,	"list"		},
	{ do_frame,	"frame"		},
	{ do_popup,	"popup"		},
	{ do_hsbar,	"hsbar"		},
	{ do_vsbar,	"vsbar"		},
	{ do_colorsel,	"colorsel"	},
	{ do_bargraph,	"bargraph"	},
	{ do_sizebox,	"sizebox"	},
};

static const struct
{
	char *	name;
	int	value;
} flagtab[] =
{
	{ "FORM_TITLE",			FORM_TITLE		},
	{ "FORM_FRAME",			FORM_FRAME		},
	{ "FORM_ALLOW_CLOSE",		FORM_ALLOW_CLOSE	},
	{ "FORM_ALLOW_RESIZE",		FORM_ALLOW_RESIZE	},
	{ "FORM_ALLOW_MINIMIZE",	FORM_ALLOW_MINIMIZE	},
	{ "FORM_BACKDROP",		FORM_BACKDROP		},
	{ "FORM_NO_BACKGROUND",		FORM_NO_BACKGROUND	},
	{ "FORM_EXCLUDE_FROM_LIST",	FORM_EXCLUDE_FROM_LIST	},
	{ "FORM_APPFLAGS",		FORM_APPFLAGS		},
	{ "FORM_FXS_APPFLAGS",		FORM_FXS_APPFLAGS	},
	{ "FORM_CENTER",		FORM_CENTER		}
};

static int expect_unq(FILE *f, char *s)
{
	struct token tk;
	
	if (get_token(f, &tk))
	{
		perror("expect_unq: get_token");
		return -1;
	}
	if (tk.type != TK_UNQUOTED || strcmp(tk.v.string, s))
	{
#if DEBUG
		if (tk.type == TK_UNQUOTED)
			_cprintf("expect_unq: %s expected, got %s\n", s, tk.v.string);
		else
			_cprintf("expect_unq: %s expected\n", s);
#endif
		_set_errno(EINVAL);
		return -1;
	}
	free_token(&tk);
	return 0;
}

struct form *form_load(const char *pathname)
{
	struct form *frm = NULL;
	struct scale sc;
	struct token tk;
	char *title;
	int x, y, w, h;
	int flags = 0;
	FILE *f = NULL;
	int i;
	
#if DEBUG	
	_cprintf("form_load:\n");
#endif
	
	sc.x_num = 1;
	sc.x_den = 1;
	sc.y_num = 1;
	sc.y_den = 1;
	adjust_scale(&sc, 6, 11);
	
	f = fopen(pathname, "r");
	if (!f)
		return NULL;
	
	if (expect_unq(f, "form"))	goto fail;
	if (expect_unq(f, "("))		goto fail;
	
	if (get_number(f, 0, &x))	goto fail;
	if (get_number(f, 1, &y))	goto fail;
	if (get_number(f, 1, &w))	goto fail;
	if (get_number(f, 1, &h))	goto fail;
	
	if (get_string(f, 1, &title))	goto fail;
	
	if (expect_unq(f, ")"))		goto fail;
	if (expect_unq(f, ";"))		goto fail;
	
#if DEBUG	
	_cprintf("form(%i, %i, %i, %i, \"%s\");\n", x, y, w, h, title);
#endif
	
	RESCALE_X(w, &sc);
	RESCALE_Y(h, &sc);
	
	for (;;)
	{
		if (get_token(f, &tk))
		{
			_set_errno(EINVAL);
			goto fail;
		}
		if (tk.type == TK_EOF)
			break;
		if (tk.type != TK_UNQUOTED)
		{
			_set_errno(EINVAL);
			goto fail;
		}
		if (strcmp(tk.v.string, "flag"))
			break;
		free_token(&tk);
		
		if (expect_unq(f, "("))	goto fail;
		
		if (get_token(f, &tk))
		{
			_set_errno(EINVAL);
			goto fail;
		}
		if (tk.type != TK_UNQUOTED)
		{
			_set_errno(EINVAL);
			goto fail;
		}
		for (i = 0; i < sizeof flagtab / sizeof *flagtab; i++)
			if (!strcmp(flagtab[i].name, tk.v.string))
			{
				flags |= flagtab[i].value;
				break;
			}
		if (i >= sizeof flagtab / sizeof *flagtab)
		{
			_set_errno(EINVAL);
			goto fail;
		}
		free_token(&tk);
		
		if (expect_unq(f, ")"))	goto fail;
		if (expect_unq(f, ";"))	goto fail;
	}
	
	frm = form_creat(flags, 0, x, y, w, h, title);
	if (!frm)
	{
#if DEBUG
		perror("form_load: form_creat");
#endif
		goto fail;
	}
	
#if DEBUG
	_cprintf("form_load: form created\n");
#endif
	
	while (tk.type != TK_EOF)
	{
		if (tk.type != TK_UNQUOTED)
		{
			_set_errno(EINVAL);
			goto fail;
		}
		
		for (i = 0; i < sizeof dispatch / sizeof *dispatch; i++)
			if (!strcmp(dispatch[i].name, tk.v.string))
			{
				if (expect_unq(f, "("))
					goto fail;
				if (dispatch[i].func(f, frm, &sc))
					goto fail;
				break;
			}
		if (i >= sizeof dispatch / sizeof *dispatch)
		{
			_set_errno(EINVAL);
			goto fail;
		}
		
		free_token(&tk);
		
		if (expect_unq(f, ")"))	goto fail;
		if (expect_unq(f, ";"))	goto fail;
		
		if (get_token(f, &tk))
		{
			fclose(f);
			return NULL;
		}
	}	
	free_token(&tk);
	
#if DEBUG
	_cprintf("form_load: fini\n");
#endif
	fclose(f);
	return frm;
fail:
#if DEBUG
	_cprintf("form_load: failed\n");
#endif
	if (f)
		fclose(f);
	if (frm)
		form_close(frm);
	return NULL;
}

struct gadget *gadget_find(struct form *f, const char *name)
{
	struct gadget *g = f->gadget;
	
	while (g)
	{
		if (g->name && !strcmp(g->name, name))
			return g;
		g = g->next;
	}
	_set_errno(ENOENT);
	return NULL;
}
