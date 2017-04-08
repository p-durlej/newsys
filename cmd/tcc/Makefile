
OBJ_I386 = $(ARCH)-asm.o $(ARCH)-gen.o $(ARCH)-link.o
OBJ_SUP	 = slib/strtold.o
OBJ	 = libtcc.o tccasm.o tcc.o tccelf.o tccgen.o tccpp.o
OBJ	+= $(OBJ_I386)
OBJ	+= $(OBJ_SUP)
BIN	 = tcc

CC	 = $(ARCH)-os386-elf-gcc -Iinclude -Isinclude

all: $(BIN)

$(BIN): $(OBJ)

clean:
	-rm -f $(OBJ) $(BIN)
