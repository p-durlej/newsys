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

#include <wingui_msgbox.h>
#include <wingui_form.h>
#include <wingui_dlg.h>
#include <sys/wait.h>
#include <newtask.h>
#include <devices.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <os386.h>
#include <stdio.h>
#include <mount.h>
#include <err.h>

#define MAIN_FORM	"/lib/forms/devices.frm"
#define DEV_FORM	"/lib/forms/device.frm"
#define DEV_PCI_FORM	"/lib/forms/device_pci.frm"
#define NEWDEVTYPE_FORM	"/lib/forms/newdevtype.frm"
#define NEWDEV_FORM	"/lib/forms/newdev.frm"
#define DEVINIT_FORM	"/lib/forms/devinit.frm"

#define DESKTOPS	"/etc/desktops"
#define DEVICES		"/etc/devices"

static struct gadget *	main_list;
static struct gadget *	b_ok;
static struct gadget *	b_cnc;
static struct gadget *	b_rm;
static struct gadget *	b_add;
static struct form *	main_form;

static int		dev_count;
static struct device *	devs;
static char *		dev_db = DEVICES;

static int		desk_count;
static char **		desks;

static int		iflag;

static void sig_evt(int nr);

static void load_desks(void)
{
	struct dirent *de;
	DIR *d;
	int i;
	
	d = opendir(DESKTOPS);
	if (!d)
		return;
	while ((de = readdir(d)))
	{
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;
		desk_count++;
	}
	desks = malloc(sizeof(char *) * desk_count);
	if (!desks)
	{
		msgbox_perror(main_form, "Devices", "malloc", errno);
		exit(errno);
	}
	rewinddir(d);
	i = 0;
	while ((de = readdir(d)))
	{
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;
		desks[i++] = strdup(de->d_name);
	}
	closedir(d);
}

static int load_devs(const char *pathname)
{
	struct stat st;
	int fd;
	int se;
	
	fd = open(pathname, O_RDONLY);
	if (fd < 0)
		return -1;
	if (fstat(fd, &st))
	{
		se = errno;
		close(fd);
		errno = se;
		return -1;
	}
	
	dev_count = st.st_size / sizeof *devs;
	devs = malloc(st.st_size);
	if (!devs)
	{
		se = errno;
		free(devs);
		close(fd);
		errno = se;
		return -1;
	}
	if (read(fd, devs, st.st_size) < 0)
	{
		se = errno;
		close(fd);
		errno = se;
		return -1;
	}
	close(fd);
	return 0;
}

static int save_devs(const char *pathname)
{
	int cnt;
	int fd;
	int se;
	
	fd = open(pathname, O_WRONLY | O_TRUNC | O_CREAT, 0600);
	if (fd < 0)
	{
		se = errno;
		msgbox_perror(main_form, "Devices", pathname, errno);
		errno = se;
		return -1;
	}
	cnt = sizeof *devs * dev_count;
	if (write(fd, devs, cnt) != cnt)
	{
		se = errno;
		msgbox_perror(main_form, "Devices", pathname, errno);
		errno = se;
		return -1;
	}
	if (close(fd))
	{
		se = errno;
		msgbox_perror(main_form, "Devices", pathname, errno);
		errno = se;
		return -1;
	}
	return 0;
}

static void show_devs(void)
{
	int i;
	
	list_clear(main_list);
	for (i = 0; i < dev_count; i++)
		list_newitem(main_list, devs[i].desc);
}

#define MEM	0
#define IO	1
#define DMA	2
#define IRQ	3

static struct gadget *rsrc_list;
static struct form *dev_form;

static struct device *curr_dev;

static struct rsrc
{
	char desc[64];
	int index;
	int type;
} rsrc[40];

static int rsrc_cnt = 0;

static void show_rsrc(void)
{
	int i;
	
	list_clear(rsrc_list);
	rsrc_cnt = 0;
	for (i = 0; i < DEV_MEM_COUNT; i++)
		if (curr_dev->mem_size[i])
		{
			sprintf(rsrc[rsrc_cnt].desc, "MEM: 0x%08x - 0x%08x",
				curr_dev->mem_base[i],
				curr_dev->mem_base[i] + curr_dev->mem_size[i] - 1);
			rsrc[rsrc_cnt].index = i;
			rsrc[rsrc_cnt].type = MEM;
			rsrc_cnt++;
		}
	for (i = 0; i < DEV_IO_COUNT; i++)
		if (curr_dev->io_size[i])
		{
			sprintf(rsrc[rsrc_cnt].desc, "IO: 0x%04x - 0x%04x",
				curr_dev->io_base[i],
				curr_dev->io_base[i] + curr_dev->io_size[i] - 1);
			rsrc[rsrc_cnt].index = i;
			rsrc[rsrc_cnt].type = IO;
			rsrc_cnt++;
		}
	for (i = 0; i < DEV_DMA_COUNT; i++)
		if (curr_dev->dma_nr[i] != (unsigned)-1)
		{
			sprintf(rsrc[rsrc_cnt].desc, "DMA: %i", curr_dev->dma_nr[i]);
			rsrc[rsrc_cnt].index = i;
			rsrc[rsrc_cnt].type = DMA;
			rsrc_cnt++;
		}
	for (i = 0; i < DEV_IRQ_COUNT; i++)
		if (curr_dev->irq_nr[i] != (unsigned)-1)
		{
			sprintf(rsrc[rsrc_cnt].desc, "IRQ: %i", curr_dev->irq_nr[i]);
			rsrc[rsrc_cnt].index = i;
			rsrc[rsrc_cnt].type = IRQ;
			rsrc_cnt++;
		}
	
	for (i = 0; i < rsrc_cnt; i++)
		list_newitem(rsrc_list, rsrc[i].desc);
}

static void rsrc_request(struct gadget *lg, int i, int button)
{
	struct rsrc *r = &rsrc[i];
	const char *title = "";
	char buf[64];
	unsigned v;
	
	switch (r->type)
	{
	case MEM:
		sprintf(buf, "0x%08x", curr_dev->mem_base[r->index]);
		title = "Memory Base:";
		break;
	case IO:
		sprintf(buf, "0x%04x", curr_dev->io_base[r->index]);
		title = "I/O Base:";
		break;
	case DMA:
		sprintf(buf, "%i", curr_dev->dma_nr[r->index]);
		title = "DMA Channel:";
		break;
	case IRQ:
		sprintf(buf, "%i", curr_dev->irq_nr[r->index]);
		title = "IRQ Line:";
		break;
	}
	
retry:
	dlg_input(dev_form, title, buf, sizeof buf);
	
	switch (r->type)
	{
	case MEM:
		curr_dev->mem_base[r->index] = strtoul(buf, NULL, 0);
		break;
	case IO:
		curr_dev->io_base[r->index] = strtoul(buf, NULL, 0);
		break;
	case DMA:
		v = strtoul(buf, NULL, 0);
		if (v == (unsigned)-1)
		{
			msgbox(dev_form, "Devices", "Invalid value.");
			goto retry;
		}
		curr_dev->dma_nr[r->index] = v;
		break;
	case IRQ:
		v = strtoul(buf, NULL, 0);
		if (v == (unsigned)-1)
		{
			msgbox(dev_form, "Devices", "Invalid value.");
			goto retry;
		}
		curr_dev->irq_nr[r->index] = v;
		break;
	}
	
	show_rsrc();
}

static void drvr_chg_click(void)
{
	dlg_input(dev_form, "Driver File:", curr_dev->driver, sizeof curr_dev->driver);
}

static void edit_dev(struct device *dev)
{
	struct gadget *pcil = NULL;
	struct gadget *dpo;
	struct gadget *ppo;
	int is_pci;
	int cnt;
	int i;
	
	curr_dev = dev;
	
	is_pci = dev->pci_bus >= 0;
	
	if (is_pci)
	{
		dev_form  = form_load(DEV_PCI_FORM);
		pcil	  = gadget_find(dev_form, "pci");
		rsrc_list = NULL;
	}
	else
	{
		dev_form  = form_load(DEV_FORM);
		rsrc_list = gadget_find(dev_form, "rsrc");
	}
	
	dpo	  = gadget_find(dev_form, "d_popup");
	ppo	  = gadget_find(dev_form, "p_popup");
	
	label_set_text(gadget_find(dev_form, "desc"), dev->desc);
	label_set_text(gadget_find(dev_form, "name"), dev->name);
	label_set_text(gadget_find(dev_form, "type"), dev->type);
	label_set_text(gadget_find(dev_form, "driver"), dev->driver);
	
	if (*dev->desktop_name)
		for (i = 0; i < desk_count; i++)
		{
			popup_newitem(dpo, desks[i]);
			if (!strcmp(desks[i], dev->desktop_name))
				popup_set_index(dpo, i);
		}
	else
	{
		gadget_remove(gadget_find(dev_form, "d_frame"));
		gadget_remove(dpo);
	}
	
	if (*dev->parent_type)
		for (cnt = 0, i = 0; i < dev_count; i++)
		{
			if (strcmp(dev->parent_type, devs[i].type))
				continue;
			popup_newitem(ppo, devs[i].name);
			if (!strcmp(devs[i].name, dev->parent))
				popup_set_index(ppo, cnt);
			cnt++;
		}
	else
	{
		gadget_remove(gadget_find(dev_form, "p_frame"));
		gadget_remove(ppo);
	}
	
	if (!iflag && rsrc_list)
		list_on_request(rsrc_list, rsrc_request);
	
	if (is_pci)
	{
		char buf[80];
		
		sprintf(buf, "PCI: bus %i, device %i, function %i", dev->pci_bus, dev->pci_dev, dev->pci_func);
		label_set_text(pcil, buf);
	}
	else
		show_rsrc();
	
	form_set_dialog(main_form, dev_form);
	while (form_wait(dev_form) != 1)
		if (iflag)
			msgbox(dev_form, "Devices",
				"Device drivers cannot be changed while\n"
				"the system installer is running.");
		else
			drvr_chg_click();
	
	if (*dev->desktop_name)
	{
		i = popup_get_index(dpo);
		if (i >= 0 && i < desk_count)
			strcpy(dev->desktop_name, popup_get_text(dpo, i));
	}
	if (*dev->parent_type)
	{
		i = popup_get_index(ppo);
		if (i >= 0)
			strcpy(dev->parent, popup_get_text(ppo, i));
	}
	
	form_close(dev_form);
}

static void on_request(struct gadget *g, int i, int button)
{
	form_lock(main_form);
	edit_dev(&devs[i]);
	form_unlock(main_form);
}

static int on_close(struct form *f)
{
	exit(0);
}

static void ok_click(void)
{
	if (save_devs(dev_db))
	{
		msgbox_perror(main_form, "Devices", dev_db, errno);
		return;
	}
	if (!iflag)
		dlg_reboot(main_form, "Devices");
	exit(0);
}

static void rm_click(void)
{
	int i;
	
	if (iflag)
	{
		msgbox(main_form, "Devices", "Devices cannot be removed while "
					     "the system installer is running.");
		return;
	}
	
	i = list_get_index(main_list);
	
	if (i < 0 || i >= dev_count)
	{
		msgbox(main_form, "Devices",
				  "You must have a device selected to perform "
				  "this command.");
		return;
	}
	
	if (msgbox_ask(main_form, "Remove Device",
				  "Are you sure you want to remove this "
				  "device?") != MSGBOX_YES)
		return;
	
	memmove(&devs[i], &devs[i + 1], (--dev_count - i) * sizeof *devs);
	show_devs();
}

static char devi_path[PATH_MAX];

static int get_dev_type(void)
{
	struct dirent *de;
	struct gadget *li;
	struct form *f;
	const char *p;
	DIR *d;
	int i;
	
	strcpy(devi_path, "/lib/drv_list/");
	
	f  = form_load(NEWDEVTYPE_FORM);
	li = gadget_find(f, "list");
	
	d = opendir(devi_path);
	if (!d)
	{
		char msg[256];
		
		sprintf(msg, "Cannot open %s:\n\n%m", devi_path);
		msgbox(f, "Devices", msg);
		return -1;
	}
	while ((de = readdir(d)))
	{
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;
		list_newitem(li, strdup(de->d_name));
	}
	closedir(d);
	
	if (form_wait(f) != 1)
	{
		form_close(f);
		return -1;
	}
	
	i = list_get_index(li);
	if (i < 0)
	{
		form_close(f);
		return -1;
	}
	p = list_get_text(li, i);
	strcat(devi_path, p);
	strcat(devi_path, "/");
	form_close(f);
	return 0;
}

static int get_dev(void)
{
	struct dirent *de;
	struct gadget *li;
	struct form *f;
	const char *p;
	DIR *d;
	int i;
	
	f  = form_load(NEWDEV_FORM);
	li = gadget_find(f, "list");
	
	d = opendir(devi_path);
	if (!d)
	{
		char msg[256];
		
		sprintf(msg, "Cannot open %s:\n\n%m", devi_path);
		msgbox(f, "Devices", msg);
		return -1;
	}
	while ((de = readdir(d)))
	{
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;
		list_newitem(li, strdup(de->d_name));
	}
	closedir(d);
	
	if (form_wait(f) != 1)
	{
		form_close(f);
		return -1;
	}
	
	i = list_get_index(li);
	if (i < 0)
	{
		form_close(f);
		return -1;
	}
	p = list_get_text(li, i);
	strcat(devi_path, p);
	form_close(f);
	return 0;
}

static char *get_desktop(void)
{
	static char buf[16];
	
	char *p;
	
	if (*buf)
		return buf;
	
	p = getenv("DESKTOP_DIR");
	if (!p)
		return NULL;
	if (strncmp(p, DESKTOPS "/", sizeof DESKTOPS))
		return NULL;
	if (strlen(p + sizeof DESKTOPS) >= sizeof buf)
		return NULL;
	strcpy(buf, p + sizeof DESKTOPS);
	return buf;
}

static void add_click(void)
{
	struct device dev;
	char buf[256];
	char *p;
	FILE *f;
	int md;
	int i;
	
	if (get_dev_type())
		return;
	if (get_dev())
		return;
	
	memset(&dev, 0, sizeof dev);
	
	dev.pci_bus = dev.pci_dev = dev.pci_func = -1;
	
	for (i = 0; i < DEV_IRQ_COUNT; i++)
		dev.irq_nr[i] = -1;
	for (i = 0; i < DEV_DMA_COUNT; i++)
		dev.dma_nr[i] = -1;
	
	f = fopen(devi_path, "r");
	if (!f)
	{
		sprintf(buf, "Cannot open %s:\n\n%m", devi_path);
		msgbox(main_form, "Devices", buf);
		return;
	}
	while (fgets(buf, sizeof buf, f))
	{
		char fmt[32];
		
		p = strchr(buf, '\n');
		if (p)
			*p = 0;
		if (!*buf)
			continue;
		
		if (!strncmp(buf, "desc=", 5))
		{
			strcpy(dev.desc, buf + 5);
			continue;
		}
		if (!strncmp(buf, "type=", 5))
		{
			strcpy(dev.type, buf + 5);
			continue;
		}
		if (!strncmp(buf, "name=", 5))
		{
			p = strchr(buf + 5, '%');
			if (p)
			{
				int n = 0;
				
				if (p[1] != 'i')
				{
					msgbox(main_form, "Devices", "Invalid device information file.");
					return;
				}
				
next:
				sprintf(fmt, buf + 5, n);
				for (i = 0; i < dev_count; i++)
					if (!strcmp(fmt, devs[i].name))
					{
						n++;
						goto next;
					}
				strcpy(dev.name, fmt);
			}
			else
				strcpy(dev.name, buf + 5);
			continue;
		}
		if (!strcmp(buf, "desktop"))
		{
			p = get_desktop();
			
			if (p && strlen(p) < sizeof dev.desktop_name)
				strcpy(dev.desktop_name, p);
			else
			{
				msgbox(main_form, "Devices",
				       "The current desktop name could not be determined.\n"
				       "The device may not be usable until you set the desktop manually.");
				if (desk_count)
					strcpy(dev.desktop_name, *desks);
				else
					strcpy(dev.desktop_name, "desktop0");
			}
			continue;
		}
		if (!strncmp(buf, "driver=", 7))
		{
			strcpy(dev.driver, buf + 7);
			continue;
		}
		if (!strncmp(buf, "parent_type=", 12))
		{
			strcpy(dev.parent_type, buf + 12);
			continue;
		}
		if (!strncmp(buf, "parent=", 7))
		{
			strcpy(dev.parent, buf + 7);
			continue;
		}
		
		for (i = 0; i < DEV_MEM_COUNT; i++)
		{
			sprintf(fmt, "mem%i_base=", i);
			if (!strncmp(buf, fmt, strlen(fmt)))
			{
				dev.mem_base[i] = strtoul(buf + strlen(fmt), NULL, 0);
				goto cont;
			}
			sprintf(fmt, "mem%i_size=", i);
			if (!strncmp(buf, fmt, strlen(fmt)))
			{
				dev.mem_size[i] = strtoul(buf + strlen(fmt), NULL, 0);
				goto cont;
			}
		}
		for (i = 0; i < DEV_IO_COUNT; i++)
		{
			sprintf(fmt, "io%i_base=", i);
			if (!strncmp(buf, fmt, strlen(fmt)))
			{
				dev.io_base[i] = strtoul(buf + strlen(fmt), NULL, 0);
				goto cont;
			}
			sprintf(fmt, "io%i_size=", i);
			if (!strncmp(buf, fmt, strlen(fmt)))
			{
				dev.io_size[i] = strtoul(buf + strlen(fmt), NULL, 0);
				goto cont;
			}
		}
		for (i = 0; i < DEV_DMA_COUNT; i++)
		{
			sprintf(fmt, "dma%i=", i);
			if (!strncmp(buf, fmt, strlen(fmt)))
			{
				dev.dma_nr[i] = strtoul(buf + strlen(fmt), NULL, 0);
				goto cont;
			}
		}
		for (i = 0; i < DEV_IRQ_COUNT; i++)
		{
			sprintf(fmt, "irq%i=", i);
			if (!strncmp(buf, fmt, strlen(fmt)))
			{
				dev.irq_nr[i] = strtoul(buf + strlen(fmt), NULL, 0);
				goto cont;
			}
		}
		goto bad_conf;
cont:
		;
	}
	fclose(f);
	if (*dev.parent_type && !*dev.parent)
		goto bad_conf;
	
	edit_dev(&dev);
	
	if (iflag)
	{
		struct form *form;
		
		form = form_load(DEVINIT_FORM);
		if (form != NULL)
			form_show(form);
		win_idle();
		sleep(1);
		win_idle();
		
		sig_evt(-1);
		md = _mod_load(dev.driver, &dev, sizeof dev);
		signal(SIGEVT, SIG_DFL);
		if (form != NULL)
			form_close(form);
		if (md < 0)
		{
			sprintf(buf, "Cannot load \"%s\":\n\n%m", dev.driver);
			msgbox(main_form, "Devices", buf);
			return;
		}
	}
	
	devs = realloc(devs, sizeof *devs * ++dev_count); /* XXX */
	devs[dev_count - 1] = dev;
	show_devs();
	return;
bad_conf:
	msgbox(main_form, "Devices", "Invalid device information file.");
}

static void sig_evt(int nr)
{
	win_idle();
	signal(SIGEVT, sig_evt);
}

int main(int argc, char **argv)
{
	int demo = 0;
	int c;
	int r;
	
	if (getenv("OSDEMO"))
	{
		dev_db = "/tmp/devices";
		iflag  = 1;
		demo   = 1;
	}
	
	while (c = getopt(argc, argv, "i"), c > 0)
		switch (c)
		{
		case 'i':
			iflag = 1;
			break;
		default:
			return 1;
		}
	
	if (argc > optind)
		dev_db = argv[optind];
	
	signal(SIGEVT, SIG_DFL);
	
	if (win_attach())
		err(255, NULL);
	
	if (getuid())
	{
		msgbox(NULL, "Devices", "You are not root.");
		return 0;
	}
	
	main_form = form_load(MAIN_FORM);
	main_list = gadget_find(main_form, "list");
	b_ok	  = gadget_find(main_form, "b_ok");
	b_cnc	  = gadget_find(main_form, "b_cnc");
	b_rm	  = gadget_find(main_form, "b_rm");
	b_add	  = gadget_find(main_form, "b_add");
	
	if (iflag)
	{
		gadget_focus(b_ok);
		gadget_hide(b_cnc);
		gadget_hide(b_rm);
		gadget_move(b_add, b_cnc->rect.x - main_form->workspace_rect.x,
				   b_cnc->rect.y - main_form->workspace_rect.y);
	}
	
	form_on_close(main_form, on_close);
	list_on_request(main_list, on_request);
	
	if (load_devs(dev_db))
		msgbox_perror(main_form, "Devices", dev_db, errno);
	load_desks();
	show_devs();
	
	if (demo)
	{
		form_show(main_form);
		msgbox(main_form, "Devices",
			"Existing device entries cannot be edited or removed in the OS live demo mode.\n\n"
			"Install the operating system to manage devices.");
	}
	
	while (r = form_wait(main_form), r != 2)
		switch (r)
		{
		case 1:
			ok_click();
			break;
		case 3:
			rm_click();
			break;
		case 4:
			add_click();
			break;
		}
	return 0;
}
