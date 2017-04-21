
OBJ_I386 = $(ARCH)-asm.o $(ARCH)-gen.o $(ARCH)-link.o
OBJ_SUP	 = slib/strtold.o
OBJ	 = libtcc.o tccasm.o tcc.o tccelf.o tccgen.o tccpp.o
OBJ	+= $(OBJ_I386)
OBJ	+= $(OBJ_SUP)
BIN	 = tcc

include config.mk

CPPFLAGS	+= -Isinclude

all: $(BIN)

$(BIN): $(OBJ)

clean:
	-rm -f $(OBJ) $(BIN)
