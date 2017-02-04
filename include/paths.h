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

/* standard paths */

#define _PATH_DEFPATH		"/bin:/usr/bin"
#define _PATH_STDPATH		"/bin:/usr/bin"

#define _PATH_BSHELL		"/bin/sh"
#define _PATH_TTY		"/dev/tty"
#define _PATH_UNIX		_PATH_L_KERN
#define _PATH_DEV		"/dev"
#define _PATH_MEM		"/dev/mem"
#define _PATH_CONSOLE		"/dev/console"
#define _PATH_DEVNULL		"/dev/null"
#define _PATH_SHELLS		_PATH_E_SHELLS

/* extensions */

#define _PATH_B_DEFSHELL	_PATH_BSHELL

#define _PATH_E_BOOT_CONF	"/etc/boot.conf"
#define _PATH_E_SS_COPY		"/etc/sysinstall.copy"
#define _PATH_E_SS_MKDIR	"/etc/sysinstall.mkdir"
#define _PATH_E_SS_LINK		"/etc/sysinstall.link"
#define _PATH_E_PASSWD		"/etc/passwd"
#define _PATH_E_PASSWD_S	"/etc/passwd-secret"
#define _PATH_E_GROUP		"/etc/group"
#define _PATH_E_DESKTOPS	"/etc/desktops"
#define _PATH_E_FSTAB		"/etc/fstab"
#define _PATH_E_MODULES		"/etc/modules"
#define _PATH_E_BSCRIPT		"/etc/boot"
#define _PATH_E_SYSFONTS	"/etc/sysfonts"
#define _PATH_E_DEVICES		"/etc/devices"
#define _PATH_E_SS		"/etc/sysinstall"
#define _PATH_E_SHELLS		"/etc/shells"
#define _PATH_E_ISSUE		"/etc/issue"
#define _PATH_E_MOTD		"/etc/motd"

#define _PATH_B_VTTY		"/bin/vtty"
#define _PATH_B_CLI		"/bin/cli"
#define _PATH_B_SH		"/bin/sh"
#define _PATH_B_CPWD		"/bin/passwd"
#define _PATH_B_WAVAIL		"/bin/avail"
#define _PATH_B_OSK		"/bin/osk"
#define _PATH_B_EDIT		"/bin/edit"
#define _PATH_B_FILEMGR		"/bin/filemgr"
#define _PATH_B_FILEMGR_SLAVE	"/bin/filemgr.slave"
#define _PATH_B_CP		"/bin/cp"
#define _PATH_B_MV		"/bin/mv"
#define _PATH_B_RM		"/bin/rm"
#define _PATH_B_PREF_COLOR	"/bin/pref.color"
#define _PATH_B_TASKBAR		"/bin/taskbar"

#define _PATH_B_FIND_DEVS	"/sbin/find_devs"
#define _PATH_B_VTTY_CON	"/sbin/vtty-con"
#define _PATH_B_MKFS		"/sbin/mkfs"
#define _PATH_B_SS		"/sbin/sysinstall"
#define _PATH_B_SHUTDOWN	"/sbin/shutdown"
#define _PATH_B_LOGIN		"/sbin/login"
#define _PATH_B_WLOGIN		"/sbin/wlogin"
#define _PATH_B_IDEVS		"/sbin/init.devs"
#define _PATH_B_IDESKTOP	"/sbin/init.desktop"
#define _PATH_B_IPROFILE	"/sbin/init.profile"
#define _PATH_B_HWCLOCK		"/sbin/hwclock"
#define _PATH_B_TASKMAN		"/sbin/taskman"
#define _PATH_B_SHUTDOWN	"/sbin/shutdown"
#define _PATH_B_FSCK		"/sbin/fsck"
#define _PATH_B_FDISK		"/sbin/fdisk"
#define _PATH_B_EDIT_DEVS	"/sbin/edit_devs"
#define _PATH_B_WDMESG		"/sbin/wdmesg"
#define _PATH_B_MKFSI		"/sbin/filemgr.mkfsi"
#define _PATH_B_USERMGR		"/sbin/usermgr"

#define _PATH_L_KERN		"/lib/sys/kern.bin"
#define _PATH_L_BB		"/lib/sys/hdboot.bin"
#define _PATH_L_SYSLOAD		"/lib/sys/sysload"
#define _PATH_L_LIBX		"/lib/sys/libx.bin"
#define _PATH_L_THEMES		"/lib/w_themes"

#define _PATH_H_DEFAULT		"/etc/syshome"
#define _PATH_H_ROOT		"/root"

#define _PATH_V_DEVICES		"/var/run/devices"

#define _PATH_FONT_MONO		"/lib/fonts/mono"
#define _PATH_FONT_SYSTEM	"/lib/fonts/system"
#define _PATH_FONT_MONO_L	"/lib/fonts/mono-large"
#define _PATH_FONT_SYSTEM_L	"/lib/fonts/system-large"
#define _PATH_FONT_MONO_N	"/lib/fonts/mono-narrow"
#define _PATH_FONT_MONO_LN	"/lib/fonts/mono-large-narrow"

#define _PATH_I386_IPL		"/lib/sys/hdipl.bin"
#define _PATH_I386_BB		"/lib/sys/hdboot.bin"

#define _PATH_I386_SERIAL_DRV	"/lib/serial.drv"
#define _PATH_I386_PCKBD_DRV	"/lib/pckbd.drv"
#define _PATH_I386_PCSPK_DRV	"/lib/pcspk.drv"
#define _PATH_I386_VBE_DRV	"/lib/vbe.drv"

#define _PATH_M_PTY		"/dev/pty"
#define _PATH_M_DEV		"/dev"

#define _PATH_D_PTMX		"/dev/pty/ptmx"
#define _PATH_D_CONSOLE		"/dev/console"

#define _PATH_P_MICONS		"/media/icons"
#define _PATH_P_MEDIA		"/media"
