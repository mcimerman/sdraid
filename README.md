SD RAID: a hardware RAID on SD cards for AVR
============================================

**It is assumed that the cards are SDHC.**

### Notes

SD code ([sd.c](./sd.c)) is partly taken from [here](https://github.com/i350/ATMEGA328P-SD-Card-FAT32-SPI-ATMEL-Studio/tree/master).

It is cleaned up and styled a bit.

Other notes [here](./NOTES.md).

## Idea

### Project output

Have a working memory abstraction allowing redundancy using classic
RAID techniques like mirroring and possibly different parity
layouts like RAID5.

### Interface

> Still an open question, behave just like another SPI slave,
or allow for more high level API throgh UART.

### Assembly

During assembly there is going to be a sdraid superblock written
to all underlying SD cards holding the array configuration like:
 - magic
 - no. of SD cards
 - index in the array
 - level
 - layout
