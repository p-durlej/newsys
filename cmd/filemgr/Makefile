
include config.mk

FILEMGR_O := main.o get_info.o file_type.o
LIB_DEP	  := $(CRT0) $(CRTBEG) $(CRTEND) $(LIBC)

all: filemgr filemgr.slave filemgr.mkfsi pref.filemgr

filemgr: $(FILEMGR_O) $(LIB_DEP)
	$(CC) $(FILEMGR_O) -o filemgr $(CPPFLAGS) $(CFLAGS) $(LDFLAGS)

pref.filemgr: pref.filemgr.c $(LIB_DEP)
	$(CC) pref.filemgr.c -o pref.filemgr $(CPPFLAGS) $(CFLAGS) $(LDFLAGS)

filemgr.slave: filemgr.slave.c $(LIB_DEP)
	$(CC) filemgr.slave.c -o filemgr.slave $(CPPFLAGS) $(CFLAGS) $(LDFLAGS)

filemgr.mkfsi: filemgr.mkfsi.c $(LIB_DEP)
	$(CC) filemgr.mkfsi.c -o filemgr.mkfsi $(CPPFLAGS) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(FILEMGR_O)
	rm -f filemgr filemgr.elf pref.filemgr filemgr.slave filemgr.mkfsi
