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

all: zlib.o

zlib.o: $(ZLIB_O)
	$(LD) -r -o zlib.o $(ZLIB_O)

clean:
	-rm -f zlib.o $(ZLIB_O)
