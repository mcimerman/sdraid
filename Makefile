CFLAGS = -mmcu=atmega328p -Os -Wall -Wextra
CC = avr-gcc
DEV = "$(shell ls /dev/ttyUSB* | sed 1q)"

all:

sdraid: sdraid.o uart.o spi.o sd.o util.o raid0.c raid1.c
	$(CC) $(CFLAGS) -o $@ $^
	avr-objcopy -O ihex $@ $@.hex
	avrdude -c arduino -p m328p -U flash:w:"$@.hex":a -P $(DEV) || rm $@

read: read.o uart.o spi.o sd.o
	$(CC) $(CFLAGS) -o $@ $^
	avr-objcopy -O ihex $@ $@.hex
	avrdude -c arduino -p m328p -U flash:w:"$@.hex":a -P $(DEV) || rm $@

%.o: %.c var.h uart.h spi.h sd.h util.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.hex read sdraid

serial:
	screen $(DEV) 115200 -parenb -cstopb
