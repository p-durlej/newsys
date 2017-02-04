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

form(-1, -1, 240, 142, "Warning Beep");
flag(FORM_ALLOW_CLOSE);
flag(FORM_CENTER);
flag(FORM_TITLE);
flag(FORM_FRAME);

frame(NULL, 0, 0, 240, 50, "Pitch");
hsbar("frequency", 10, 15, 220, 12);
label("l_frequency", 10, 30, "XXX Hz");

frame(NULL, 0, 50, 240, 50, "Duration");
hsbar("duration", 10, 65, 220, 12);
label("l_duration", 10, 80, "XXX ms");

chkbox("enable", 5, 100, "Enable warning beep");

frame(NULL, 5, 115, 230, 2, NULL);

button(NULL,   5, 122, 60, 16, "OK",     1);
button(NULL,  67, 122, 60, 16, "Cancel", 2);
button(NULL, 175, 122, 60, 16, "Test",   3);
