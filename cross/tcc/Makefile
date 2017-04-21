
OBJ_I386 = $(ARCH)-asm.o $(ARCH)-gen.o $(ARCH)-link.o
OBJ	 = libtcc.o tccasm.o tcc.o tccelf.o tccgen.o tccpp.o
OBJ	+= $(OBJ_I386)
BIN	 = ctcc

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) -o $(BIN) $(OBJ)

clean:
	-rm -f $(OBJ) $(BIN)
