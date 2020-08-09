all : program.hex burn program.lst
CC = avr-gcc

SRCS = main.c ePaperColor.c fullsd.c basicfat.c avr_print.c

PROCESSOR=atmega168pb
PROGRAMCODE=m168pb
CFLAGS = -O1 -mmcu=$(PROCESSOR) -DF_CPU=8000000UL -I.
ASFLAGS = $(CFLAGS) -x assembler-with-cpp

CFLAGS:=$(CFLAGS) -flto -fwhole-program  #NOTE: For figuring out more details from binaries, ditch this line.
LDFLAGS:=-Wl,--relax -mcall-prologues  -Wl,-Map,program.map

#-Wa,-a,-ad

program.elf : $(SRCS)
	avr-gcc $(CFLAGS) $(LDFLAGS) -o $@ $^ -L /usr/lib64/binutils/avr/2.19.1

program.hex : program.elf
	avr-objcopy -j .text -j .data -O ihex program.elf program.hex 

program.lst : $(SRCS)
	avr-gcc -c -g -Wa,-a,-ad $(CFLAGS) $(LDFLAGS) $^ > $@

burn : program.hex
	sudo avrdude -c usbtiny -p $(PROGRAMCODE) -U flash:w:program.hex


readfuses :
	sudo avrdude -c usbtiny -p $(PROGRAMCODE) -U hfuse:r:high.txt:b -U lfuse:r:low.txt:b

#careful here...
#burnfuses :
#	sudo avrdude -c usbtiny -p $(PROGRAMCODE) -U lfuse:w:0xE6:m -U hfuse:w:0xDF:m

clean : 
	rm -f *~ high.txt low.txt program.hex program.map program.elf $(OBJS) *.o usbdrv/*.o usbdrv/*~ program.lst

serial :
	stty -F /dev/ttyUSB0 4800 -parenb -echo -cstopb

