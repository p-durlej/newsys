
BIN  = elotest
OBJ  = elotest.o elo.o elo_i386.o elo_usys.o
HEAD = elo.h

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) -static -o $(BIN) $(OBJ)

clean:
	rm -f $(BIN) $(OBJ)
