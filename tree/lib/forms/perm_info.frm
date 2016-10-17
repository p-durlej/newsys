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

form(-1,-1,238,106,"Permissions");
flag(FORM_TITLE);
flag(FORM_FRAME);
font(6,11);

label(NULL,60,8,	"Read");
label(NULL,100,8,	"Write");
label(NULL,140,8,	"Exec");
label(NULL,180,8,	"SUID/SGID");

label(NULL,8,27,	"Owner");
label(NULL,8,46,	"Group");
label(NULL,8,65,	"Others");

button(NULL,6,84,60,16,	"OK",1);
button(NULL,68,84,60,16,"Cancel",2);

chkbox("irusr",60,27,	"Allow");
chkbox("iwusr",100,27,	"Allow");
chkbox("ixusr",140,27,	"Allow");

chkbox("irgrp",60,46,	"Allow");
chkbox("iwgrp",100,46,	"Allow");
chkbox("ixgrp",140,46,	"Allow");

chkbox("iroth",60,65,	"Allow");
chkbox("iwoth",100,65,	"Allow");
chkbox("ixoth",140,65,	"Allow");

chkbox("isuid",180,27,	"Set");
chkbox("isgid",180,46,	"Set");
