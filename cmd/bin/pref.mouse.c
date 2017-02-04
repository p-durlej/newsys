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

#include <priv/pointer.h>
#include <wingui_form.h>
#include <wingui.h>
#include <confdb.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#define MAIN_FORM	"/lib/forms/mouse.frm"

static struct ptr_conf pconf;

static struct gadget *n_speed_hsbar;
static struct gadget *a_speed_hsbar;
static struct gadget *a_thrshld_hsbar;
static struct gadget *swap_chk;
static struct gadget *larg_chk;
static struct form *main_form;

static void save_settings(void)
{
	int i;
	
	for (i = 2; i < sizeof pconf.map / sizeof *pconf.map; i++)
		pconf.map[i] = i;
	if (chkbox_get_state(swap_chk))
	{
		pconf.map[0] = 1;
		pconf.map[1] = 0;
	}
	else
	{
		pconf.map[0] = 0;
		pconf.map[1] = 1;
	}
	
	if (chkbox_get_state(larg_chk))
		strcpy(pconf.ptr_path, "/lib/pointers/large");
	else
		strcpy(pconf.ptr_path, "/lib/pointers");
	
	pconf.speed.acc_threshold = hsbar_get_pos(a_thrshld_hsbar) + 1;
	pconf.speed.nor_mul = hsbar_get_pos(n_speed_hsbar) + 25;
	pconf.speed.acc_mul = hsbar_get_pos(a_speed_hsbar) + 25;
	pconf.speed.nor_div = 100;
	pconf.speed.acc_div = 100;
	win_map_buttons(pconf.map, sizeof pconf.map / sizeof *pconf.map);
	win_set_ptr_speed(&pconf.speed);
	c_save("ptr_conf", &pconf, sizeof pconf);
	win_update();
}

static void reset_click(void)
{
	hsbar_set_pos(n_speed_hsbar, 75);
	hsbar_set_pos(a_speed_hsbar, 75);
	hsbar_set_pos(a_thrshld_hsbar, 0);
	
	save_settings();
}

static void ok_click(void)
{
	save_settings();
	exit(0);
}

int main()
{
	int i;
	int r;
	
	if (win_attach())
		err(255, NULL);
	
	for (i = 0; i < sizeof pconf.map / sizeof *pconf.map; i++)
		pconf.map[i] = i;
	if (c_load("ptr_conf", &pconf, sizeof pconf))
	{
		if (win_get_dpi_class() > 0)
			strcpy(pconf.ptr_path, "/lib/pointers/large");
		else
			strcpy(pconf.ptr_path, "/lib/pointers");
	}
	
	win_get_ptr_speed(&pconf.speed);
	
	main_form	= form_load(MAIN_FORM);
	n_speed_hsbar	= gadget_find(main_form, "speed");
	a_speed_hsbar	= gadget_find(main_form, "a_speed");
	a_thrshld_hsbar	= gadget_find(main_form, "a_thr");
	swap_chk	= gadget_find(main_form, "swap");
	larg_chk	= gadget_find(main_form, "large");
	
	hsbar_set_limit(n_speed_hsbar, 975);
	hsbar_set_step(n_speed_hsbar, 20);
	hsbar_set_pos(n_speed_hsbar, 100 * pconf.speed.nor_mul / pconf.speed.nor_div - 25);
	
	hsbar_set_limit(a_speed_hsbar, 975);
	hsbar_set_step(a_speed_hsbar, 20);
	hsbar_set_pos(a_speed_hsbar, 100 * pconf.speed.acc_mul / pconf.speed.acc_div - 25);
	
	hsbar_set_limit(a_thrshld_hsbar, 10);
	hsbar_set_pos(a_thrshld_hsbar, pconf.speed.acc_threshold - 1);
	
	if (!strcmp(pconf.ptr_path, "/lib/pointers/large"))
		chkbox_set_state(larg_chk, 1);
	
	if (pconf.map[0])
		chkbox_set_state(swap_chk, 1);
	
	
	while (r = form_wait(main_form), r != 2)
		switch (r)
		{
		case 1:
			ok_click();
			break;
		case 2:
			break;
		case 3:
			reset_click();
			break;
		}
	return 0;
}
