#
# Copyright (c) 2017, Piotr Durlej
# All rights reserved.
#

include config.mk

ZLIB_O = adler32.o	\
	 compress.o	\
	 crc32.o	\
	 deflate.o	\
	 gzclose.o	\
	 gzlib.o	\
	 gzread.o	\
	 gzwrite.o	\
	 infback.o	\
	 inffast.o	\
	 inflate.o	\
	 inftrees.o	\
	 trees.o	\
	 uncompr.o	\
	 zutil.o

all: zlib.a

zlib.a: $(ZLIB_O)
	-rm -f zlib.a
	$(AR) -q zlib.a $(ZLIB_O)

clean:
	-rm -f zlib.a $(ZLIB_O)
