
include config.mk
include	mk/$(MACH).mk

# LOAD_O	= $(ARCH_O) load.o exports.o elo/elo.o elo/elosys.o ../libc.a
LOAD_O	= $(ARCH_O) load.o exports.o elo/elo.o elo/elosys.o
CRTB_O	= ../arch/crt0/crtbegin.o ../libc.o
CRTE_O	= ../arch/crt0/crtend.o
OBJS	= $(HEAD_O) $(CRTB_O) $(LOAD_O) $(CRTE_O)

all: load load.elf

exports.c: ../libc_syms mkexports
	./mkexports

load: head.o $(OBJS) $(LIBGCC)
	$(LD) -T arch/load.lds --oformat binary head.o $(OBJS) $(LIBGCC) -o load
	../../cross/ckexe -w load

load.elf: head.o $(OBJS)
	$(LD) -T arch/load.lds head.o $(OBJS) $(LIBGCC) -o load.elf

clean:
	rm -f load load.elf exports.c head.o
	rm -f $(LOAD_O)
