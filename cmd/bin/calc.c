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
#include <wingui_form.h>
#include <wingui_menu.h>
#include <wingui_dlg.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>

#define MAIN_FORM	"/lib/forms/calc.frm"

#define F_DIGITS	8
#define I_DIGITS	8
#define DIGITS		((F_DIGITS + I_DIGITS) * 2)

static struct gadget *disp_label;
static struct form *main_form;

struct fxpn
{
	int8_t	digit[DIGITS];
	int	negative;
};

static int clear_on_input = 0;
static int pos = F_DIGITS;
static char op = 0;
static int error;

static struct fxpn arg0;
static struct fxpn arg1;
static struct fxpn mem;

static void fxp_zero(struct fxpn *n);
static void fxp_shr(struct fxpn *n, int count);
static void fxp_shl(struct fxpn *n, int count);
static void fxp_add(struct fxpn *acc, struct fxpn *arg);
static void fxp_sub(struct fxpn *acc, struct fxpn *arg);
static void fxp_mul(struct fxpn *acc, struct fxpn *arg);
static void fxp_div(struct fxpn *acc, struct fxpn *arg);

static int fxp_nzero(struct fxpn *n)
{
	int i;
	
	for (i = 0; i < DIGITS; i++)
		if (n->digit[i])
			return 1;
	
	return 0;
}

static void fxp_zero(struct fxpn *n)
{
	memset(n, 0, sizeof *n);
}

static void fxp_shr(struct fxpn *n, int count)
{
	if (count < 0)
		fxp_shl(n, -count);
	else
	{
		memmove(n->digit, n->digit + count, DIGITS - count);
		memset(n->digit + DIGITS - count, 0, count);
	}
}

static void fxp_shl(struct fxpn *n, int count)
{
	if (count < 0)
		fxp_shr(n, -count);
	else
	{
		memmove(n->digit + count, n->digit, DIGITS - count);
		memset(n->digit, 0, count);
	}
}

static int fxp_ucmp(struct fxpn *arg1, struct fxpn *arg2)
{
	int i;
	
	for (i = DIGITS - 1; i >= 0; i--)
	{
		if (arg1->digit[i] < arg2->digit[i])
			return -1;
		if (arg1->digit[i] > arg2->digit[i])
			return 1;
	}
	
	return 0;
}

static void fxp_uadd(struct fxpn *acc, struct fxpn *arg)
{
	int cy = 0;
	int i;
	
	for (i = 0; i < DIGITS; i++)
	{
		acc->digit[i] += arg->digit[i] + cy;
		if (acc->digit[i] > 9)
		{
			acc->digit[i] -= 10;
			cy = 1;
		}
		else
			cy = 0;
	}
}

static void fxp_usub(struct fxpn *acc, struct fxpn *arg)
{
	int cy = 0;
	int i;
	
	for (i = 0; i < DIGITS; i++)
	{
		acc->digit[i] -= arg->digit[i] + cy;
		if (acc->digit[i] < 0)
		{
			acc->digit[i] += 10;
			cy = 1;
		}
		else
			cy = 0;
	}
}

static void fxp_add(struct fxpn *acc, struct fxpn *arg)
{
	struct fxpn a1;
	struct fxpn a2;
	int n;
	
	if (acc->negative == arg->negative)
		fxp_uadd(acc, arg);
	else
	{
		if (fxp_ucmp(acc, arg) >= 0)
		{
			n = acc->negative;
			a1 = *acc;
			a2 = *arg;
		}
		else
		{
			n = !acc->negative;
			a1 = *arg;
			a2 = *acc;
		}
		
		fxp_usub(&a1, &a2);
		*acc = a1;
		acc->negative = n;
	}
}

static void fxp_sub(struct fxpn *acc, struct fxpn *arg)
{
	struct fxpn a1;
	struct fxpn a2;
	int n;
	
	if (acc->negative == arg->negative)
	{
		if (fxp_ucmp(acc, arg) >= 0)
		{
			n = acc->negative;
			a1 = *acc;
			a2 = *arg;
		}
		else
		{
			n = !acc->negative;
			a1 = *arg;
			a2 = *acc;
		}
		
		fxp_usub(&a1, &a2);
		*acc = a1;
		acc->negative = n;
	}
	else
		fxp_uadd(acc, arg);
}

static void fxp_mul(struct fxpn *acc, struct fxpn *arg)
{
	struct fxpn a;
	int i, n;
	
	a = *acc;
	fxp_zero(acc);
	acc->negative = a.negative ^ arg->negative;
	
	for (i = 0; i < DIGITS; i++)
	{
		n = arg->digit[i];
		while (n--)
			fxp_uadd(acc, &a);
		
		fxp_shl(&a, 1);
	}
}

static void fxp_div(struct fxpn *acc, struct fxpn *arg)
{
	struct fxpn r;
	struct fxpn a;
	int i;
	
	fxp_zero(&a);
	fxp_zero(&r);
	r.negative = acc->negative ^ arg->negative;
	
	for (i = 0; i < DIGITS; i++)
		if (arg->digit[i])
			break;
	
	if (i == DIGITS)
		return;
	
	for (i = DIGITS - 1; i >= 0; i--)
	{
		fxp_shl(&a, 1);
		a.digit[0] = acc->digit[i];
		
		while (fxp_ucmp(&a, arg) >= 0)
		{
			fxp_usub(&a, arg);
			r.digit[i]++;
		}
	}
	
	*acc = r;
}

static int main_form_close(struct form *f)
{
	exit(0);
}

static void update(void)
{
	char buf[DIGITS + 7];
	char *p = buf;
	int last;
	int i;
	
	if (error)
	{
		label_set_text(disp_label, " - error -");
		return;
	}
	
	if (fxp_nzero(&mem))
	{
		*(p++) = 'M';
		*(p++) = ' ';
	}
	else
	{
		*(p++) = ' ';
		*(p++) = ' ';
	}
	
	if (arg0.negative)
		*(p++) = '-';
	else
		*(p++) = ' ';
	
	for (i = DIGITS - 1; i > F_DIGITS; i--)
		if (arg0.digit[i])
			break;
	
	for (; i >= F_DIGITS; i--)
		*(p++) = arg0.digit[i] + '0';
	
	*(p++) = '.';
	
	for (last = 0; last < F_DIGITS; last++)
		if (arg0.digit[last])
			break;
	
	if (last > pos + 1)
		last = pos + 1;
	
	for (i = F_DIGITS - 1; i >= last; i--)
		*(p++) = arg0.digit[i] + '0';
	
	*p = 0;
	label_set_text(disp_label, buf);
}

static void input(unsigned digit)
{
	if (error)
		return;
	
	if (pos < 0)
		return;
	
	if (clear_on_input)
	{
		clear_on_input = 0;
		fxp_zero(&arg0);
		pos = F_DIGITS;
	}
	
	if (pos == F_DIGITS)
	{
		if (arg0.digit[F_DIGITS + I_DIGITS - 1])
			return;
		
		fxp_shl(&arg0, 1);
		arg0.digit[pos] = digit;
	}
	else
		arg0.digit[pos--] = digit;
	
	update();
}

static void input_digit(char ch)
{
	input(ch - '0');
}

static void do_op()
{
	int i;
	
	if (error)
		return;
	
	switch (op)
	{
	case '+':
		fxp_add(&arg1, &arg0);
		arg0 = arg1;
		break;
	
	case '-':
		fxp_sub(&arg1, &arg0);
		arg0 = arg1;
		break;
	
	case '*':
		fxp_mul(&arg1, &arg0);
		fxp_shr(&arg1, F_DIGITS);
		arg0 = arg1;
		break;
	
	case '/':
		if (fxp_nzero(&arg0))
		{
			fxp_shl(&arg1, F_DIGITS);
			fxp_div(&arg1, &arg0);
			arg0 = arg1;
		}
		else
			error = 1;
		break;
	
	default:
		clear_on_input = 1;
		arg1 = arg0;
	}
	
	for (i = F_DIGITS + I_DIGITS; i < DIGITS; i++)
		if (arg0.digit[i])
			error = 1;
	
	clear_on_input = 1;
	pos = F_DIGITS;
	update();
}

static void ce_click()
{
	fxp_zero(&arg0);
	pos = F_DIGITS;
	
	update();
}

static void c_click()
{
	fxp_zero(&arg0);
	fxp_zero(&arg1);
	pos = F_DIGITS;
	error = 0;
	op = 0;
	
	update();
}

static void equ_click()
{
	do_op();
	op = 0;
}

static void add_click()
{
	do_op();
	op = '+';
}

static void sub_click()
{
	do_op();
	op = '-';
}

static void mul_click()
{
	do_op();
	op = '*';
}

static void div_click()
{
	do_op();
	op = '/';
}

static void dot_click()
{
	if (clear_on_input)
	{
		clear_on_input = 0;
		fxp_zero(&arg0);
		pos = F_DIGITS;
		update();
	}
	
	if (pos == F_DIGITS)
		pos--;
}

static void digit_click(struct gadget *g, int x, int y)
{
	input(g->l_data);
}

static void mr_click()
{
	clear_on_input = 1;
	pos = F_DIGITS;
	arg0 = mem;
	
	update();
}

static void mc_click()
{
	fxp_zero(&mem);
	
	update();
}

static void mp_click()
{
	do_op();
	fxp_add(&mem, &arg0);
	
	update();
}

static void mm_click()
{
	do_op();
	fxp_sub(&mem, &arg0);
	
	update();
}

static void about_click()
{
	dlg_about7(main_form, NULL, "Calculator v1.1", SYS_PRODUCT, SYS_AUTHOR, SYS_CONTACT, "calc.pnm");
}

static struct kdispatch
{
	char ch;
	void *proc;
} kdispatch[] =
{
	{ '\b',		ce_click	},
	{ '\033',	c_click		},
	{ '\n',		equ_click	},
	{ '=',		equ_click	},
	{ '+',		add_click	},
	{ '-',		sub_click	},
	{ '*',		mul_click	},
	{ '/',		div_click	},
	{ '.',		dot_click	},
	{ ',',		dot_click	},
	{ '0',		input_digit	},
	{ '1',		input_digit	},
	{ '2',		input_digit	},
	{ '3',		input_digit	},
	{ '4',		input_digit	},
	{ '5',		input_digit	},
	{ '6',		input_digit	},
	{ '7',		input_digit	},
	{ '8',		input_digit	},
	{ '9',		input_digit	},
};

static int main_form_key_down(struct form *f, unsigned ch, unsigned shift)
{
	static const struct kdispatch *e = kdispatch + sizeof kdispatch / sizeof *kdispatch;
	
	struct kdispatch *p;
	
	for (p = kdispatch; p < e; p++)
		if (p->ch == ch)
		{
			void (*proc)(char ch) = p->proc;
			
			proc(ch);
			return 0;
		}
	return 1;
}

int main(int argc, char **argv)
{
	struct menu *om;
	struct menu *m;
	int i;
	
	if (win_attach())
		err(255, NULL);
	
	om = menu_creat();
	menu_newitem(om, "About ...", about_click);
	
	m = menu_creat();
	menu_submenu(m, "Options", om);
	
	main_form = form_load(MAIN_FORM);
	if (!main_form)
	{
		perror(MAIN_FORM);
		return 1;
	}
	form_on_key_down(main_form, main_form_key_down);
	form_on_close(main_form, main_form_close);
	form_set_menu(main_form, m);
	
	for (i = 0; i < 10; i++)
	{
		struct gadget *g;
		char is[2];
		
		sprintf(is, "%i", i);
		
		g = gadget_find(main_form, is);
		g->l_data = i;
		
		button_on_click(g, digit_click);
	}
	
	button_on_click(gadget_find(main_form, "mul"), mul_click);
	button_on_click(gadget_find(main_form, "mr"), mr_click);
	button_on_click(gadget_find(main_form, "c"), c_click);
	
	button_on_click(gadget_find(main_form, "div"), div_click);
	button_on_click(gadget_find(main_form, "mc"), mc_click);
	button_on_click(gadget_find(main_form, "ce"), ce_click);
	
	button_on_click(gadget_find(main_form, "sub"), sub_click);
	button_on_click(gadget_find(main_form, "mp"), mp_click);
	
	button_on_click(gadget_find(main_form, "point"), dot_click);
	button_on_click(gadget_find(main_form, "exe"), equ_click);
	button_on_click(gadget_find(main_form, "add"), add_click);
	button_on_click(gadget_find(main_form, "mm"), mm_click);
	
	disp_label = gadget_find(main_form, "display");
	
	fxp_zero(&arg0);
	fxp_zero(&arg1);
	fxp_zero(&mem);
	update();
	form_show(main_form);
	
	for (;;)
		win_wait();
}
