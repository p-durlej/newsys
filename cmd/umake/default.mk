CPPFLAGS=
LDFLAGS=
CFLAGS=
LD=ld
AS=as
CC=cc
AR=ar

.c.o:
	$(CC) $(CPPFLAGS) -c -o $@ $<

.c:
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $<
