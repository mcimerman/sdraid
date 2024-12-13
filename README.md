SD RAID: a hardware RAID on SD cards made for AVR
=================================================

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
