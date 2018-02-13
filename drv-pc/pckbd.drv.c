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

#include <sysload/kparam.h>
#include <sysload/flags.h>
#include <kern/printk.h>
#include <kern/wingui.h>
#include <kern/config.h>
#include <kern/start.h>
#include <kern/clock.h>
#include <kern/intr.h>
#include <kern/lib.h>

#include <devices.h>
#include <errno.h>

#define SCAN_CTRL		0x1d
#define SCAN_LSHIFT		0x2a
#define SCAN_RSHIFT		0x36
#define SCAN_ALT		0x38
#define SCAN_CLOCK		0x3a
#define SCAN_NLOCK		0x45
#define SCAN_SLOCK		0x46

static unsigned io_base;
static unsigned aux_irq_nr;
static unsigned irq_nr;

static struct win_desktop *desktop = NULL;

static unsigned unshifted_map[128] =
{
	0, '\e', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
	
	'\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
	
	WIN_KEY_CTRL, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
	
	WIN_KEY_SHIFT, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', WIN_KEY_SHIFT,
	
	'*', WIN_KEY_ALT, ' ', WIN_KEY_CAPS, WIN_KEY_F1, WIN_KEY_F2, WIN_KEY_F3, WIN_KEY_F4,
	WIN_KEY_F5, WIN_KEY_F6, WIN_KEY_F7, WIN_KEY_F8, WIN_KEY_F9, WIN_KEY_F10,
	
	WIN_KEY_NLOCK, WIN_KEY_SLOCK,
	
	WIN_KEY_HOME, WIN_KEY_UP, WIN_KEY_PGUP, '-',
	WIN_KEY_LEFT, '5', WIN_KEY_RIGHT, '+',
	WIN_KEY_END, WIN_KEY_DOWN, WIN_KEY_PGDN,
	WIN_KEY_INS, WIN_KEY_DEL, WIN_KEY_SYSRQ,
	
	0, 0,
	
	WIN_KEY_F11, WIN_KEY_F12, 0, 0, WIN_KEY_F13, WIN_KEY_F14, WIN_KEY_F15
};

static unsigned shifted_map[128] =
{
	0, '\e', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
	
	'\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
	
	WIN_KEY_CTRL, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
	
	WIN_KEY_SHIFT, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', WIN_KEY_SHIFT,
	
	'*', WIN_KEY_ALT, ' ', WIN_KEY_CAPS, WIN_KEY_F1, WIN_KEY_F2, WIN_KEY_F3, WIN_KEY_F4,
	WIN_KEY_F5, WIN_KEY_F6, WIN_KEY_F7, WIN_KEY_F8, WIN_KEY_F9, WIN_KEY_F10,
	
	WIN_KEY_NLOCK, WIN_KEY_SLOCK,
	
	WIN_KEY_HOME, WIN_KEY_UP, WIN_KEY_PGUP, '-',
	WIN_KEY_LEFT, '5', WIN_KEY_RIGHT, '+',
	WIN_KEY_END, WIN_KEY_DOWN, WIN_KEY_PGDN,
	WIN_KEY_INS, WIN_KEY_DEL, WIN_KEY_SYSRQ,
	
	0, 0,
	
	WIN_KEY_F11, WIN_KEY_F12, 0, 0, WIN_KEY_F13, WIN_KEY_F14, WIN_KEY_F15
};

static struct
{
	unsigned prefix;
	unsigned kbd_code;
	unsigned nl_code;
	unsigned code;
} numpad_map[] =
{
	{ 0xe0, 0x35, '/',	'/'		},
	{ 0x00, 0x37, '*',	'*'		},
	{ 0xe0, 0x1c, '\n',	'\n'		},
	{ 0x00, 0x47, '7',	WIN_KEY_HOME	},
	{ 0x00, 0x48, '8',	WIN_KEY_UP	},
	{ 0x00, 0x49, '9',	WIN_KEY_PGUP	},
	{ 0x00, 0x4a, '-',	'-'		},
	{ 0x00, 0x4b, '4',	WIN_KEY_LEFT	},
	{ 0x00, 0x4c, '5',	'5'		},
	{ 0x00, 0x4d, '6',	WIN_KEY_RIGHT	},
	{ 0x00, 0x4e, '+',	'+'		},
	{ 0x00, 0x4f, '1',	WIN_KEY_END	},
	{ 0x00, 0x50, '2',	WIN_KEY_DOWN	},
	{ 0x00, 0x51, '3',	WIN_KEY_PGDN	},
	{ 0x00, 0x52, '0',	WIN_KEY_INS	},
	{ 0x00, 0x53, '.',	WIN_KEY_DEL	},
};

static void kbd_out(unsigned char byte)
{
	while (inb(io_base + 4) & 2);
	outb(io_base, byte);
}

static void kbc_out(unsigned char byte)
{
	while (inb(io_base + 4) & 2);
	outb(io_base + 4, byte);
}

static void aux_out(unsigned char byte)
{
	kbc_out(0xd4);
	kbd_out(byte);
}

static void set_leds(unsigned shift)
{
	unsigned char lbits = 0;

	if (shift & WIN_SHIFT_CLOCK)
		lbits  = 4;
	if (shift & WIN_SHIFT_NLOCK)
		lbits |= 2;
	if (shift & WIN_SHIFT_SLOCK)
		lbits |= 1;

	kbd_out(0xed);
	kbd_out(lbits);
}

static void kbd_irq()
{
	static unsigned prefix   = 0;
	static unsigned shift    = 0;
	static unsigned notoggle = 0;
	
	unsigned char scan;
	unsigned toggle = 0;
	unsigned ch;
	int i;
	
	scan = inb(io_base);
	
	if (scan == 0xe0)
	{
		prefix = 0xe0;
		return;
	}
	
	for (i = 0; i < sizeof numpad_map / sizeof *numpad_map; i++)
		if (numpad_map[i].prefix == prefix && numpad_map[i].kbd_code == (scan & 0x7f))
		{
			if (shift & WIN_SHIFT_NLOCK)
				ch = numpad_map[i].nl_code;
			else
				ch = numpad_map[i].code;
			break;
		}
	if (i == sizeof numpad_map / sizeof *numpad_map)
	{
		if (shift & WIN_SHIFT_SHIFT)
			ch = shifted_map[scan & 0x7f];
		else
			ch = unshifted_map[scan & 0x7f];
	}
	
	if (shift & WIN_SHIFT_CTRL && ch >= 'a' && ch <= 'z')
		ch -= 0x60;
	
	if (shift & WIN_SHIFT_CLOCK && ch >= 'a' && ch <= 'z')
		ch -= 0x20;
	
#if DEBUG
	if ((shift & WIN_SHIFT_ALT) && (shift & WIN_SHIFT_CTRL) && ch == WIN_KEY_END)
		panic("halted by console operator");
#endif
	
	if (scan & 0x80)
	{
		switch (scan & 0x7f)
		{
		case SCAN_CLOCK:
			notoggle &= ~WIN_SHIFT_CLOCK;
			break;
		case SCAN_NLOCK:
			notoggle &= ~WIN_SHIFT_NLOCK;
			break;
		case SCAN_SLOCK:
			notoggle &= ~WIN_SHIFT_SLOCK;
			break;
		case SCAN_LSHIFT:
		case SCAN_RSHIFT:
			if (prefix == 0xe0)
				goto fini;
			win_keyup(desktop, WIN_KEY_SHIFT);
			shift &= ~WIN_SHIFT_SHIFT;
			break;
		case SCAN_CTRL:
			win_keyup(desktop, WIN_KEY_CTRL);
			shift &= ~WIN_SHIFT_CTRL;
			break;
		case SCAN_ALT:
			win_keyup(desktop, WIN_KEY_ALT);
			shift &= ~WIN_SHIFT_ALT;
			break;
		default:
			if (ch)
				win_keyup(desktop, ch);
		}
	}
	else
	{
		switch (scan)
		{
		case SCAN_CLOCK:
			toggle = WIN_SHIFT_CLOCK;
			break;
		case SCAN_NLOCK:
			toggle = WIN_SHIFT_NLOCK;
			break;
		case SCAN_SLOCK:
			toggle = WIN_SHIFT_SLOCK;
			break;
		case SCAN_LSHIFT:
		case SCAN_RSHIFT:
			if (prefix == 0xe0)
				goto fini;
			win_keydown(desktop, WIN_KEY_SHIFT);
			shift |= WIN_SHIFT_SHIFT;
			break;
		case SCAN_CTRL:
			win_keydown(desktop, WIN_KEY_CTRL);
			shift |= WIN_SHIFT_CTRL;
			break;
		case SCAN_ALT:
			win_keydown(desktop, WIN_KEY_ALT);
			shift |= WIN_SHIFT_ALT;
			break;
		default:
			if (ch)
				win_keydown(desktop, ch);
		}
		
		if (toggle)
		{
			toggle   &= ~notoggle;
			shift    ^= toggle;
			notoggle |= toggle;
			set_leds(shift);
		}
	}
	
fini:
	prefix = 0;
}

static void aux_irq()
{
	static unsigned char packet[3];
	static unsigned btn_state = 0;
	static time_t recv_time = 0;
	static int i = 0;
	
	unsigned char data;
	
	data = inb(io_base);
	
	if (recv_time + 2 < clock_time())
		i = 0;
	recv_time = clock_time();
	
	packet[i++] = data;
	if (i < 3)
		return;
	
	if (packet[1] || packet[2] || (packet[0] & 48))
	{
		int x = packet[1];
		int y = packet[2];
		
		if (packet[0] & 16)
			x -= 256;
		if (packet[0] & 32)
			y -= 256;
		
		win_ptrmove_rel(desktop, x, -y);
	}
	
	if ((btn_state ^ packet[0]) & 1)
	{
		if (packet[0] & 1)
			win_ptrdown(desktop, 0);
		else
			win_ptrup(desktop, 0);
	}
	if ((btn_state ^ packet[0]) & 2)
	{
		if (packet[0] & 2)
			win_ptrdown(desktop, 1);
		else
			win_ptrup(desktop, 1);
	}
	
	btn_state = packet[0];
	i = 0;
}

static int wait_in(unsigned char byte, unsigned max_delay)
{
	time_t tmo_time = clock_time() + max_delay;
	
	while (tmo_time >= clock_time())
		while (inb(io_base + 4) & 1)
			if (inb(io_base) == byte)
				return 1;
	return 0;
}

static void reset(void)
{
	kbc_out(0x60);
	kbd_out(0x47);
	
	aux_out(0xff);
	if (wait_in(0xaa, 2))
	{
		aux_out(0xf4);
		wait_in(0xaa, 2);
	}
	else
	{
		if (kparam.boot_flags & BOOT_VERBOSE)
			printk("pckbd.drv: auxiliary unit reset failed\n");
		clock_delay(clock_hz() * 2);
	}
	
	kbd_out(0xff);
	wait_in(0xaa, 2);
	
	kbd_out(0xf3);
	kbd_out(0x00);
}

int mod_onload(unsigned md, char *pathname, struct device *dev, unsigned dev_size)
{
	io_base    = 0x60;
	irq_nr	   = 1;
	aux_irq_nr = 12;

	if (dev_size >= sizeof *dev)
	{
		io_base    = dev->io_base[0];
		irq_nr	   = dev->irq_nr[0];
		aux_irq_nr = dev->irq_nr[1];
	}

	desktop = win_getdesktop();
	if (!desktop)
		return EINVAL;

	reset();
	irq_set(irq_nr, kbd_irq);
	irq_ena(irq_nr);
	irq_set(aux_irq_nr, aux_irq);
	irq_ena(aux_irq_nr);
	set_leds(0);

	win_input(md);
	return 0;
}

int mod_onunload(unsigned md)
{
	if (desktop)
		win_uninstall_input(desktop, md);
	irq_dis(aux_irq_nr);
	irq_dis(irq_nr);
	return 0;
}
