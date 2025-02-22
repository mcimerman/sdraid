# m328p
CC = avr-gcc
CFLAGS = -mmcu=atmega328p -Os -Wall -Wextra -std=c99 -pedantic

# client
CC2 = gcc
CFLAGS2 = -O2 -Wall -Wextra -std=c99 -pedantic

BAUDRATE = 9600

DEV = "$(shell ls /dev/ttyUSB* | sed 1q)"

.PHONY: all clean upload ctl cyclic-writer

all: main

upload: main
	avr-objcopy -O ihex $< $<.hex
	avrdude -c arduino -p m328p -U flash:w:"$<.hex":a -P $(DEV) || rm $<

main: main.o sdraid.o uart.o spi.o sd.o util.o raid0.o raid1.o raid5.o
	$(CC) $(CFLAGS) -o $@ $^

ctl: common.h
	$(CC2) $(CFLAGS2) -o $@ ctl.c
	./ctl $(DEV)

cyclic-writer:
	$(CC2) $(CFLAGS2) -o $@ cyclic-writer.c

%.o: %.c var.h uart.h spi.h sd.h util.h common.h sdraid.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.hex main ctl cyclic-writer

serial:
	screen $(DEV) $(BAUDRATE) -parenb -cstopb
