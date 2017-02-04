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

#include <priv/wingui_wbeep.h>
#include <priv/wingui_form.h>
#include <wingui_color.h>
#include <wingui.h>
#include <event.h>
#include <errno.h>

win_setmode_cb *on_setmode;
win_update_cb *	on_update;

win_resize_cb *	on_resize;
win_unlock_cb *	on_unlock;
win_lock_cb *	on_lock;

static event_cb *on_fnotif;

extern int form_cfg_loaded;
extern int form_th_loaded;
extern int wc_loaded;

void evt_on_fnotif(event_cb *proc)
{
	on_fnotif = proc;
}

void win_on_setmode(win_setmode_cb *proc)
{
	on_setmode = proc;
}

void win_on_update(win_update_cb *proc)
{
	on_update = proc;
}

void win_on_resize(win_resize_cb *proc)
{
	on_resize = proc;
}

void win_on_unlock(win_unlock_cb *proc)
{
	on_unlock = proc;
}

void win_on_lock(win_lock_cb *proc)
{
	on_lock = proc;
}

void evt_wait(void)
{
	void form_reload_config(void);
	
	int saved_errno;
	struct event e;
	
	saved_errno = _get_errno();
	if (!evt_count())
		form_do_deferred();
	
	while (_evt_wait(&e));
	
	if (e.type == E_WINGUI)
		switch (e.win.type)
		{
		case WIN_E_SETMODE:
			if (on_setmode)
				on_setmode();
			wc_loaded = 0;
			break;
		case WIN_E_UPDATE:
			if (pref_wbeep_loaded)
				pref_wbeep_reload();
			if (form_cfg_loaded)
				form_reload_config();
			if (on_update)
				on_update();
			form_th_loaded = 0;
			wc_loaded = 0;
			break;
		case WIN_E_RESIZE:
			form_ws_resized();
			if (on_resize)
				on_resize();
			break;
		case WIN_E_SEC_UNLOCK:
			if (on_unlock)
				on_unlock();
			break;
		case WIN_E_SEC_LOCK:
			if (on_lock)
				on_lock();
			break;
		default:
			;
		}
	
	if (e.type == E_FNOTIF && on_fnotif)
		on_fnotif(&e);
	
	if (e.type && e.proc)
		e.proc(&e);
	
	if (!evt_count())
		form_do_deferred();
	_set_errno(saved_errno);
}

void evt_idle(void)
{
	form_do_deferred(); /* XXX */
	while (evt_count())
		evt_wait();
	form_do_deferred(); /* XXX */
}
