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

form(-1, -1, 240, 168,	"Device");
flag(FORM_TITLE);
flag(FORM_FRAME);

label("desc", 10, 10,		"");
label(NULL, 10, 30,		"Name:");
label(NULL, 10, 50,		"Type:");
label(NULL, 10, 70,		"Driver:");
label("name", 60, 30,		"");
label("type", 60, 50,		"");
label("driver", 20, 90,		"");

frame("d_frame", 130, 10, 100, 37, " Desktop ");
popup("d_popup", 141, 21);

frame("p_frame", 130, 47, 100, 37, " Bus ");
popup("p_popup", 141, 58);

frame(NULL, 4, 114, 232, 2, NULL);
frame(NULL, 4, 135, 232, 2, NULL);

label("pci", 6, 120,		"PCI: bus 0, device 0, function 0");

button(NULL, 170, 86, 56, 20,	"Change",2);
button(NULL, 4, 142, 60, 20,	"OK",1);
