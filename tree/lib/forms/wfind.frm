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

form(-1, -1, 320, 158,				"Find files");
flag(FORM_FXS_APPFLAGS);

frame(NULL,		0, 0, 320, 45,		"");
label(NULL,		12, 16,			"File name:");
input("name_input",	64, 12, 150, 20, 1);
chkbox("name_sp_chk",	220, 16,		"Shell pattern");

frame(NULL,		0, 45, 320, 45,		"");
label(NULL,		12, 61,			"Search in:");
input("root_input",	64, 57, 180, 20, 1);
button("root_btn",	244, 57, 60, 20,	"Browse", 0);

frame(NULL,		0, 90, 320, 45,		"");
label(NULL,		12, 106,		"Contents:");
input("content_input",	64, 102, 120, 20, 1);
chkbox("content_rx_chk",190, 106,		"Regular expression");

button("find_btn",	4, 134, 60, 20,		"Find", 0);
button("cancel_btn",	68, 134, 60, 20,	"Cancel", 0);
