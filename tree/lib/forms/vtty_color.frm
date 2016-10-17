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

form(-1,-1,202,132,		"VTTY Colors");
flag(FORM_FRAME);
flag(FORM_TITLE);
flag(FORM_ALLOW_CLOSE);

frame(NULL,0,0,202,68,		"Palette");
colorsel("c0",14,14,20,20);
colorsel("c1",36,14,20,20);
colorsel("c2",58,14,20,20);
colorsel("c3",80,14,20,20);
colorsel("c4",102,14,20,20);
colorsel("c5",124,14,20,20);
colorsel("c6",146,14,20,20);
colorsel("c7",168,14,20,20);
colorsel("c8",14,36,20,20);
colorsel("c9",36,36,20,20);
colorsel("c10",58,36,20,20);
colorsel("c11",80,36,20,20);
colorsel("c12",102,36,20,20);
colorsel("c13",124,36,20,20);
colorsel("c14",146,36,20,20);
colorsel("c15",168,36,20,20);

label(NULL,4,70,		"Foreground:");
label(NULL,4,86,		"Background:");
label(NULL,100,70,		"Cursor:");

colorsel("fgcs",70,68,16,16);
colorsel("bgcs",70,84,16,16);
colorsel("cscs",140,68,16,16);

frame(NULL,4,102,194,2,		NULL);

button(NULL,4,108,60,20,	"OK",3);
button(NULL,68,108,60,20,	"Cancel",4);
