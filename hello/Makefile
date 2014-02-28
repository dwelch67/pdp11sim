

all : start.hex

clean :
	rm -f *.o
	rm -f *.elf
	rm -f *.bin
	rm -f *.hex
	rm -f *.list

start.hex : start.s notmain.c dsim.ld
	pdp11-bsd2.11-as start.s -o start.o
	pdp11-bsd2.11-gcc -O2 -c notmain.c -o notmain.o
	pdp11-bsd2.11-ld -T dsim.ld start.o notmain.o -o start.elf
	pdp11-bsd2.11-objdump -D start.elf > start.list
	pdp11-bsd2.11-objcopy start.elf -O ihex start.hex

