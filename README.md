SD RAID: a hardware RAID on SD cards for AVR
============================================

**It is assumed that the cards are SDHC.**

### Notes

SD code ([sd.c](./sd.c)) is partly taken from [here](https://github.com/i350/ATMEGA328P-SD-Card-FAT32-SPI-ATMEL-Studio/tree/master).

It is cleaned up and styled a ~~bit~~ lot.

Other notes [here](./NOTES.md).

## Idea

### Project output

Have a working memory abstraction allowing redundancy using classic
RAID techniques like mirroring and possibly different parity
layouts like RAID5.

## "Documentation"

### Building

To build the RAID run `make`, to upload it as well, run `make upload`.

After it has been uploaded, run `make serial` to see the serial I/O with
GNU screen.

Requirements:

- `avr-gcc`
- `avr-objcopy`
- `avrdude`
- GNU or BSD `make`
- `screen` for serial communication

The `Makefile` assumes the following:

- your Atmega328P(B)'s serial port is `/dev/ttyUSB[0-9]*`

### Terminology

I call:
- the RAID array a **volume**,
- the underlying SD cards **extents**,
- the superblock **metatada**

### Interface

> Still an open question, behave just like another SPI slave,
or allow for more high level API throgh UART.

For now see `blkdev_ops_t` in [<var.h>](./var.h).

### File tree

```
.
├── Makefile
├── NOTES.md
├── README.md
├── photos
│   ├── sdraid1.jpg
│   ├── sdraid2.jpg
│   └── sdraid3.jpg
├── raid0.c          <-- striping functions
├── raid1.c          <-- mirroring functions
├── sd.c             <-- SD card commands
├── sd.h
├── sdraid.c
├── spi.c            <-- SPI control
├── spi.h
├── uart.c           <-- serial communication
├── uart.h
├── util.c           <-- helper functions
├── util.h
└── var.h            <-- main structures and constants
```

### Metadata

```
+--------------+ <- LBA 3
|   SDRAID     |
|  Superblock  |
+--------------+ <- LBA 4
|              |
|              |
|    Data      |
|              |
|              |
|              |
|              |
+--------------+
```

During assembly there is a sdraid superblock written to all
extents holding the volume configuration:

```c
typedef struct metadata {
	char magic[4];
	uint8_t version;
	uint8_t devno;
	uint8_t level;
	uint8_t strip_size_bits;
	uint32_t blkno;
	uint32_t data_blkno;
	uint8_t data_offset;
	uint8_t index;
} __attribute__((packed)) metadata_t;
```


# Setup

### Pins

SPI is connected to ICSP on ARDUINO UNO clone
[scheme 1](https://jgaurorawiki.com/_media/a5/arduino-icsp.jpg)
or [scheme 2](https://www.olimex.com/Products/AVR/Programmers/AVR-ICSP/resources/AVR-ICSP.gif)
.

Slave selects are connected on `PB2`, `PB1` and `PB0` (Arduino pins 10, 9 and 8)
(see [<spi.h>](./spi.h)).


### Cable colors

```
GREEN: SS
YELLOW: SCK
GREY: MOSI
PURPLE: MISO
RED: VCC
BROWN: GND
```

## Photos

![sdrai photo3](photos/sdraid3.jpg)
![sdrai photo1](photos/sdraid1.jpg)
![sdrai photo2](photos/sdraid2.jpg)
