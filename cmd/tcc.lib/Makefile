
OBJ	= libtcc1.o
CC	= $(TOPDIR)/cross/tcc/ctcc -I$(TOPDIR)/include

all: libtcc1.a

libtcc1.a: $(OBJ)
	$(AR) cr libtcc1.a $(OBJ)

clean:
	-rm -f libtcc1.a $(OBJ)
