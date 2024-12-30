#include "var.h"

#include <avr/io.h>
#include <util/delay.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "uart.h"
#include "spi.h"
#include "sd.h"

#define SD 0

/* simple read and write test */

int
main(void)
{
	uint8_t buf[BLKSIZE];

	uart_init();

	_delay_ms(100);

	spi_init();

	_delay_ms(100);

	if (sd_init(SD) != 0) {
		printf("initing failed\r\n");
		return (1);
	}

	_delay_ms(100);

	uint32_t nblocks = 0;
	if ((nblocks = sd_nblocks(SD)) == 0)
		return (1);
	else
		printf("sd no. %d has %lu\r\n", SD, nblocks);

	_delay_ms(100);

	printf("reading block 64\r\n");
	if (sd_read(SD, 64, buf) == 0) {
		printf("read ok:\r\n");
		for (size_t i = 0; i < 512; i++) {
			if (i != 0 && i % 32 == 0)
				printf("\r\n");
			printf("%.2x ", buf[i]);
		}
		printf("\r\n");
	} else {
		printf("read failed\r\n");
	}

	printf("enter what you want to fill in: ");
	char c = uart_rx();
	printf("\r\n");

	memset((void *)buf, c, 512);

	_delay_ms(100);

	uint8_t retval = sd_write(SD, 64, buf);
	if (retval != 0) {
		printf("write failed\r\n");
		return (1);
	}
	printf("write ok\r\n");

	_delay_ms(100);

	printf("reading 64 again\r\n");
	if (sd_read(SD, 64, buf) == 0) {
		printf("read ok:\r\n");
		for (size_t i = 0; i < 512; i++) {
			if (i != 0 && i % 32 == 0)
				printf("\r\n");
			printf("%.2x ", buf[i]);
		}
		printf("\r\n");
	} else {
		printf("read failed\r\n");
	}

	return (0);
}
