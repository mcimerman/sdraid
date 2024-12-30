#include "var.h"

#include <avr/io.h>
#include <util/delay.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "uart.h"
#include "util.h"
#include "spi.h"
#include "sd.h"

int
main(void)
{
	sdvol_t vol = { 0 };

	uart_init();

	spi_init();

	sdlevel_t level = RAID0;
	uint8_t devno = 2;

	vol.devno = devno;

	printf("hello sdraid!\r\n");

	switch (level) {
	case RAID0:
		vol.strip_size_bits = 12; /* 4K strips */
		vol.ops.create = raid0_create;
		break;
	case RAID1:
		vol.ops.create = raid1_create;
		break;
	default:
		printf("error: invalid level\r\n");
		return (1);
	}

	if (vol.ops.create(&vol) != 0) {
		printf("error: failed to create sdraid volume\r\n");
		return (1);
	}

	_delay_ms(100);

	printf("main() end\r\n");

	return (0);
}
