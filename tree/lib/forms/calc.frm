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

form(-1,-1,152,95,"Calculator");
flag(FORM_FXS_APPFLAGS);

label("display",2,2,		"");

button("7",2,15,23,18,		"7",0);
button("8",27,15,23,18,		"8",0);
button("9",52,15,23,18,		"9",0);
button("mul",77,15,23,18,	"*",0);
button("mr",102,15,23,18,	"MR",0);
button("c",127,15,23,18,	"C",0);

button("4",2,35,23,18,		"4",0);
button("5",27,35,23,18,		"5",0);
button("6",52,35,23,18,		"6",0);
button("div",77,35,23,18,	"/",0);
button("mc",102,35,23,18,	"MC",0);
button("ce",127,35,23,18,	"CE",0);

button("1",2,55,23,18,		"1",0);
button("2",27,55,23,18,		"2",0);
button("3",52,55,23,18,		"3",0);
button("sub",77,55,23,18,	"-",0);
button("mp",102,55,23,18,	"M+",0);

button("0",2,75,23,18,		"0",0);
button("point",27,75,23,18,	".",0);
button("exe",52,75,23,18,	"=",0);
button("add",77,75,23,18,	"+",0);
button("mm",102,75,23,18,	"M-",0);
