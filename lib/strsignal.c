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

#include <sys/signal.h>
#include <string.h>

const char *const sys_siglist[] = 
{
	[SIGHUP]	= "Hang up",
	[SIGINT]	= "Interrupt",
	[SIGQUIT]	= "Quit",
	[SIGILL]	= "Illegal instruction",
	[SIGTRAP]	= "Machine code trap",
	[SIGABRT]	= "Aborted",
	[SIGBUS]	= "Bus error",
	[SIGFPE]	= "FPU error",
	[SIGKILL]	= "Killed",
	[SIGSTOP]	= "Stopped",
	[SIGSEGV]	= "Segmentation fault",
	[SIGCONT]	= "Continue",
	[SIGPIPE]	= "Broken pipe",
	[SIGALRM]	= "Alarm",
	[SIGTERM]	= "Terminated",
	[SIGUSR1]	= "User signal 1",
	[SIGUSR2]	= "User signal 2",
	[SIGCHLD]	= "Child death",
	[SIGPWR]	= "Power failure",
	[SIGINFO]	= "Status request",
	
	[SIGEVT]	= "Event received",
	[SIGEXEC]	= "Load error",
	[SIGSTK]	= "Stack fault",
	[SIGOOM]	= "Out of memory"
};

const char *strsignal(int sig)
{
	static char buf[64];
	
	char *p;
	
	if (sig < 1 || sig > sizeof sys_siglist / sizeof *sys_siglist)
		goto bad_sig;
	if (sys_siglist[sig] == NULL)
		goto bad_sig;
	return (char *)sys_siglist[sig];
	
bad_sig:
	sprintf(buf, "Unknown signal %i", sig);
	return buf;
}
