.c.o:
	$(CC) $(CPPFLAGS) -c -o $@ $<

.c:
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $<
