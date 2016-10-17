#
# Copyright (c) 2016, Piotr Durlej
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

form(-1,-1,238,170,"Configure Comm Port");
flag(FORM_TITLE);
flag(FORM_FRAME);

frame(NULL,76,0,90,82,			"Word Length");

chkbox("w5",88,12,			"5 bits");
chkbox("w6",88,28,			"6 bits");
chkbox("w7",88,44,			"7 bits");
chkbox("w8",88,60,			"8 bits");

frame(NULL,0,0,76,98,			"Parity");

chkbox("pn",12,12,			"None");
chkbox("pe",12,28,			"Even");
chkbox("po",12,44,			"Odd");
chkbox("ph",12,60,			"High");
chkbox("pl",12,76,			"Low");

frame(NULL,0,98,166,56,			"Stop Bits");

chkbox("s1",12,112,			"1 bit");
chkbox("s2",12,128,			"2 bits (or 1.5)");

label(NULL,4,156,			"Speed:");

popup("sp",60,154);

button(NULL,170,8,60,20,		"OK",1);
button(NULL,170,32,60,20,		"Cancel",2);
