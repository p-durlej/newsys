
all: fdboot.bin hdboot.bin hdflat.bin hdipl.bin cdboot.bin

fdboot.bin: fdboot.S boot.h
	$(CC) -c fdboot.S
	$(LD) fdboot.o -o fdboot.bin --oformat binary -Ttext 0

hdboot.bin: hdboot.S boot.h
	$(CC) -DHDFLAT=0 -c hdboot.S
	$(LD) hdboot.o -o hdboot.bin --oformat binary -Ttext 0

hdflat.bin: hdboot.S boot.h
	$(CC) -DHDFLAT=1 -c hdboot.S -o hdflat.o
	$(LD) hdflat.o -o hdflat.bin --oformat binary -Ttext 0

hdboot.elf: hdboot.S boot.h
	$(CC) -c hdboot.S
	$(LD) hdboot.o -o hdboot.elf -Ttext 0

hdipl.bin: hdipl.S
	$(CC) -c hdipl.S
	$(LD) hdipl.o -o hdipl.bin --oformat binary -Ttext 0

cdboot.bin: cdboot.S
	$(CC) -c cdboot.S
	$(LD) cdboot.o -o cdboot.bin --oformat binary -Ttext 0
	../cross/cdcks cdboot.bin

clean:
	rm -f fdboot.bin hdboot.bin hdflat.bin hdipl.bin hdboot.elf cdboot.bin *.o
