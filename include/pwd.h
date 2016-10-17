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

#ifndef __PWD_H
#define __PWD_H

#include <sys/types.h>

struct passwd
{
	uid_t pw_uid;
	gid_t pw_gid;
	char *pw_name;
	char *pw_gecos;
	char *pw_dir;
	char *pw_shell;
};

struct passwd *getpwent(void);
void setpwent(void);
void endpwent(void);

struct passwd *getpwnam(const char *name);
struct passwd *getpwuid(uid_t uid);

int _newuser(const char *name, uid_t uid, gid_t gid);
int _deluser(const char *name);
int _setgroup(const char *name, gid_t gid);
int _setshell(const char *name, const char *shell);
int _sethome(const char *name, const char *home);
int _setpass(const char *name, const char *pass);
int _chkpass(const char *name, const char *pass);

#define PASSWD_MAX	31
#define LOGNAME_MAX	31

#endif
