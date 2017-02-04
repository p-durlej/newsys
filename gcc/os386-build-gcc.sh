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

set -e

. ./const.sh

exec > "$TMPDIR/$ARCH-build-gcc.log" 2>&1

export PREFIX="$INSTDIR"
export PATH="$PATH:$PREFIX/bin"

if [ "$LD_LIBRARY_PATH" ]; then
	export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$INSTDIR/gcc-os386/lib"
else
	export LD_LIBRARY_PATH="$INSTDIR/gcc-os386/lib"
fi

objdir="$TMPDIR/obj-gcc"

rm -rf "$objdir"
mkdir  "$objdir"
cd     "$objdir"

"$TMPDIR/gcc-$GCCVER/configure" --prefix="$PREFIX"	\
	--target=$ARCH-os386-elf			\
	--disable-libada				\
	--disable-libssp				\
	--with-gmp=/usr/local				\
	--with-mpfr=/usr/local

echo "MAKEINFO = :" >> Makefile

"$MAKE" -j $JOBS
"$MAKE" -j $JOBS install
