
DRV := bootfb.drv amd64-pc.drv

AMD64_PC_O := 

CLEAN := $(AMD64_PC_O)

AMD64_PC_O :=	amd64-pc/main.o		\
		amd64-pc/bios.o		\
		amd64-pc/asmintr.o	\
		amd64-pc/intr.o		\
		amd64-pc/clock.o	\
		amd64-pc/dma.o

amd64-pc.drv.o: $(AMD64_PC_O)
	$(LD) -r -o $@ $(AMD64_PC_O)

amd64-pc.drv: amd64-pc.drv.o amd64-pc-syms
	$(MODGEN) $@ $< amd64-pc-syms

include drv.mk
