#include "spi.h"

void
spi_init(void)
{
	SPI_DDR |= (1 << SPI_MOSI) | (1 << SPI_SCK) | (1 << SPI_SS) | (1 << SPI_SS2) | (1 << SPI_SS3);

	SPI_DDR &= ~(1 << SPI_MISO);

	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);

	for (uint8_t i = 0; i < MAX_DEVNO; i++)
		ss_disable(i);

	SPSR = 0;
}

uint8_t
spi_tx(uint8_t data)
{
	SPDR = data;

	while(!(SPSR & (1 << SPIF)))
		;

	return (SPDR);
}

uint8_t
spi_rx(void)
{
	uint8_t data;

	SPDR = 0xff;
	while(!(SPSR & (1<<SPIF)))
		;

	data = SPDR;

	return (data);
}

void
ss_enable(uint8_t slave)
{
	switch (slave) {
	case 0:
		SS_ENABLE();
		break;
	case 1:
		SS_ENABLE2();
		break;
	case 2:
		SS_ENABLE3();
		break;
	}
}

void 
ss_disable(uint8_t slave)
{
	switch (slave) {
	case 0:
		SS_DISABLE();
		break;
	case 1:
		SS_DISABLE2();
		break;
	case 2:
		SS_DISABLE3();
		break;
	}
}
