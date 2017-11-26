CPPFLAGS=
LDFLAGS=
CFLAGS=
LD=ld
AS=as
CC=cc

.c.o:
	$(CC) $(CPPFLAGS) -c -o $@ $<

.c:
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $<
