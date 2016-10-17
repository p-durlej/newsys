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

#ifndef _EVENT_H
#define _EVENT_H

#define EVT_MAX		256

#define E_WINGUI	1
#define E_SIGNAL	2
#define E_FNOTIF	3

struct event;

typedef void event_cb(struct event *e);

struct event
{
	int type;
	
	event_cb *proc;
	void *data;
	
	union
	{
		int sig;
		
		struct
		{
			int type;
			int more;
			int wd;
			
			union
			{
				struct
				{
					int abs_x, abs_y;
					int ptr_x, ptr_y;
					int ptr_button;
					int dd_serial;
				};
				
				struct
				{
					unsigned shift;
					unsigned ch;
				};
				
				struct
				{
					int redraw_x;
					int redraw_y;
					int redraw_w;
					int redraw_h;
				};
			};
		} win;
	};
};

#ifndef _KERN_
void evt_on_fnotif(event_cb *proc);
int  evt_signal(int nr, void *proc);
int  evt_count(void);
void evt_wait(void);
void evt_idle(void);
#endif

#endif
