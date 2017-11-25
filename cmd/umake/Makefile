# Copyright (C) Piotr Durlej

include config.mk

OBJ = umake.o load.o
BIN = umake

all: $(BIN)

clean:
	-rm -f $(BIN) $(OBJ)

$(BIN): $(OBJ)
	$(CC) -o $(BIN) $(OBJ) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS)
