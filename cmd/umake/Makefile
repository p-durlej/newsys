# Copyright (C) Piotr Durlej

CFLAGS = -g

OBJ = umake.o load.o
BIN = umake

all: $(BIN)

clean:
	-rm -f $(BIN) $(OBJ)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJ)

include cmd.mk
