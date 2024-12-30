#ifndef _SPI_H
#define _SPI_H

#include "var.h"

#define SPI_DDR		DDRB
#define SPI_PORT	PORTB
#define SPI_MOSI	PB3
#define SPI_MISO	PB4
#define SPI_SCK		PB5
#define SPI_SS		PB2
#define SPI_SS2		PB1
#define SPI_SS3		PB0

#define SS_ENABLE()	(SPI_PORT &= ~(1 << SPI_SS))
#define SS_DISABLE()	(SPI_PORT |= (1 << SPI_SS))

#define SS_ENABLE2()	(SPI_PORT &= ~(1 << SPI_SS2))
#define SS_DISABLE2()	(SPI_PORT |= (1 << SPI_SS2))

#define SS_ENABLE3()	(SPI_PORT &= ~(1 << SPI_SS3))
#define SS_DISABLE3()	(SPI_PORT |= (1 << SPI_SS3))

void spi_init(void);
uint8_t spi_tx(uint8_t);
uint8_t spi_rx(void);
void ss_enable(uint8_t);
void ss_disable(uint8_t);

#endif
