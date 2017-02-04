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

#ifndef _UNISTD_H
#define _UNISTD_H

#include <seekdef.h>
#include <libdef.h>

#include <sys/types.h>

int	ioctl(int fd, int cmd, void *buf);
ssize_t	read(int fd, void *buf, size_t count);
ssize_t	write(int fd, const void *buf, size_t count);
int	close(int fd);
int	unlink(const char *pathname);
int	link(const char *name1, const char *name2);
off_t	lseek(int fd, off_t off, int whence);
int	access(const char *pathname, int mode);
int	chown(const char *path, uid_t uid, gid_t gid);
int	fchown(int fd, uid_t uid, gid_t gid);
int	pipe(int fd[2]);
int	dup(int fd);
int	dup2(int fd1, int fd2);
int	sync(void);

int	revoke(const char *pathname);

char *	getcwd(char *path, int len);
int	chdir(const char *path);
int	chroot(const char *path);
int	rmdir(const char *path);

useconds_t ualarm(useconds_t u_delay, useconds_t repeat);
int	usleep2(useconds_t usec, int intr);
int	usleep(useconds_t usec);
int	alarm(int sec);
int	sleep(int sec);
int	pause(void);

pid_t	fork(void);
int	_exit(int status) __attribute__((noreturn));

int	nice(int inc);
pid_t	getpid(void);
pid_t	getppid(void);
pid_t	getpgrp(void);
pid_t	getpgid(pid_t pid);
int	setpgid(pid_t pid, pid_t pgid);
int	setpgrp(void);
pid_t	setsid(void);

uid_t	getuid(void);
uid_t	geteuid(void);
gid_t	getgid(void);
gid_t	getegid(void);
int	getgroups(int size, gid_t *list);
int	setgroups(size_t size, const gid_t *list);
int	setuid(uid_t uid);
int	setgid(gid_t gid);

int	execve(const char *path, char *const argv[], char *const envp[]);
int	execv(const char *path, char *const argv[]);
int	execl(const char *path, ...);
int	execle(const char *path, ...);
int	execvp(const char *path, char *const argv[]);
int	execlp(const char *path, ...);

char *	ttyname(int fd);
int	ttyname_r(int fd, const char *buf, size_t len);
int	isatty(int fd);

extern char **environ;

#define R_OK		4
#define W_OK		2
#define X_OK		1
#define F_OK		0

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

extern char *optarg;
extern int   optind;
extern int   optopt;
extern int   opterr;
extern int   optreset;

int	getopt(int argc, char *const argv[], const char *opts);
int	_getopt(int argc, char *const argv[], const char *opts,
		char **opta, int *opti, int *opto, int *opte, int *optr);

#endif
