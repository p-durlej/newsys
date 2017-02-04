#
# Copyright (c) 2017, Piotr Durlej
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

form(-1,-1,200,172,"File Info");
flag(FORM_TITLE);
flag(FORM_FRAME);
font(6,11);

picture("icon",2,2,		NULL);

label("name_label",36,13,	"filename");

input("name_input",36,9,162,18,1);

label("size",2,36,		"size");
label("mode",2,48,		"mode");
label("ino",2,61,		"ino");
label("nlink",2,74,		"nlink");

label(NULL,2,87,		"Owner:");
label(NULL,2,100,		"Group:");

label("uid_label",62,87,	"uname");
label("gid_label",62,100,	"gname");

button("uid_btn",148,87,50,13,	"Change",4);
button("gid_btn",148,100,50,13,	"Change",5);

label(NULL,2,113,		"Accessed:");
label(NULL,2,126,		"i changed:");
label(NULL,2,140,		"Modified:");

label("atime",62,113,		"atime");
label("ctime",62,126,		"ctime");
label("mtime",62,140,		"mtime");

button(NULL,2,153,60,16,	"OK",1);
button(NULL,64,153,60,16,	"Cancel",2);
button(NULL,126,153,60,16,	"Mode",3);
