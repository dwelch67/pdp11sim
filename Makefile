

all : pdp11sim start.hex

clean :
	rm -f *.o
	rm -f *.elf
	rm -f *.bin
	rm -f *.hex
	rm -f *.list
	rm -f pdp11sim

pdp11sim : pdp11sim.c
	gcc -O2 pdp11sim.c -o pdp11sim

start.hex : start.s dsim.ld
	pdp11-bsd2.11-as start.s -o start.o
	pdp11-bsd2.11-ld -T dsim.ld start.o -o start.elf
	pdp11-bsd2.11-objdump -D start.elf > start.list
	pdp11-bsd2.11-objcopy start.elf -O ihex start.hex

