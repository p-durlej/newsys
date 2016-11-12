
DRV := vga.drv vga256.drv vbe.drv i386-pc.drv biosdisk.drv

I386_PC_O := i386-pc/main.o	\
	     i386-pc/bios.o	\
	     i386-pc/console.o	\
	     i386-pc/asmintr.o	\
	     i386-pc/intr.o	\
	     i386-pc/clock.o	\
	     i386-pc/dma.o	\
	     i386-pc/vesacon.o	\
	     i386-pc/monofnt.o

CLEAN := $(I386_PC_O)

i386-pc.drv.o: $(I386_PC_O)
	$(LD) -r $(CFLAGS) -o $@ $(I386_PC_O)

i386-pc.drv: i386-pc.drv.o i386-pc-syms
	$(MODGEN) $@ $< i386-pc-syms

include drv.mk
