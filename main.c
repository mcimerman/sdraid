#include "var.h"

#include <avr/io.h>
#include <util/delay.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "uart.h"
#include "util.h"
#include "spi.h"
#include "sd.h"
#include "sdraid.h"

static void init_menu(void);
static void normal_menu(void);
static void debug_menu(void);
static sdraid_cfg_t fill_cfg(bool);
static void receive_and_write_block(uint32_t);
static void put(void);
static void get(void);
static void read1(uint8_t *buf);
static void read2(uint8_t *buf);

sdvol_t vol; /* laziness */

int
main(void)
{
	_delay_ms(100);

	uart_init();

	spi_init();

	init_menu();

	return (0);
}

static void
init_menu(void)
{
	sdraid_cfg_t cfg = { 0 };

	char c;

	printf("----- SDRAID -----\r\n");
	for (;;) {
		c = uart_rx();

		switch (c) {
		case 'c':
		case 'C':
			cfg = fill_cfg(false);
			if (sdraid_create(&cfg, &vol, false) != 0) {
				printf("error creating volume\r\n");
				break;
			}

			_delay_ms(600);
			printf("OK\r\n");

			normal_menu();
			break;
		case 'a':
		case 'A':
			if (sdraid_create(&cfg, &vol, true) != 0) {
				printf("error assembling volume\r\n");
				break;
			}

			_delay_ms(600);
			printf("OK\r\n");

			normal_menu();
			break;
		default:
			printf("invalid cmd\r\n");
			break;
		}
	}
}

static void 
normal_menu(void)
{
	for (;;) {
		char c = uart_rx();
		switch (c) {
		case 'S':
			printf("stack left: %d\r\n", stack_left());
			sdraid_print_status(&vol);
			break;
		case 'P': /* PUT (receive) */
			put();
			break;
		case 'G': /* GET (send out) */
			get();
			break;
		case 'D':
			debug_menu();
			break;
		default:
			printf("invalid cmd\r\n");
			break;
		}
	}
}

__attribute__ ((noinline))
static void
debug_menu(void)
{
	uint8_t buf[BLKSIZE];
	char c;

	for (;;) {
		c = uart_rx();
		switch (c) {
		case 'r':
			read1(buf);
			break;
		case 'R':
			read2(buf);
			break;
		case 'E':
			return;
		default:
			printf("invalid cmd\r\n");
			break;
		}
	}
}

static sdraid_cfg_t
fill_cfg(bool assembly)
{
	sdraid_cfg_t cfg = { 0 };

	cfg.devno = uart_rx();

	if (assembly)
		return (cfg);

	cfg.level = uart_rx();

	if (cfg.level == RAID5)
		cfg.layout = uart_rx();

	return (cfg);
}

__attribute__ ((noinline))
static void
receive_and_write_block(uint32_t off)
{
	uint8_t buf[BLKSIZE];

	for (uint16_t i = 0; i < BLKSIZE; i++) {
		buf[i] = uart_rx();
	}

	if (vol.dev_ops.vol_write_blk(&vol, off, buf) == 0)
		printf("OK\r\n");
	else
		printf("NOK\r\n");
}

static void
put(void)
{
	uint32_t file_size, blks;

	file_size = rx_uint32_t();
	blks = rx_uint32_t();
	if (blks >= vol.data_blkno) {
		printf("file too big: %lu >= vol->data_blkno (%lu)\r\n",
		    blks, vol.data_blkno);
		return;
	}

	vol.file_size = file_size;

	_delay_ms(600);

	if (write_metadata(&vol) != 0) {
		printf("NOK\r\n");
		return;
	}

	printf("OK\r\n");

	for (uint32_t i = 0; i < blks; i++)
		receive_and_write_block(i);

	_delay_ms(500);

	printf("OK\r\n");
}

__attribute__ ((noinline))
static void
get(void)
{
	uint8_t buf[BLKSIZE];

	tx_uint32_t(vol.file_size);
	if (vol.file_size == 0)
		return;

	uint32_t blks = vol.file_size / BLKSIZE;
	if (vol.file_size % BLKSIZE != 0)
		blks++;

	int rc;

	for (uint32_t i = 0; i < blks; i++) { 
		if ((rc = vol.dev_ops.vol_read_blk(&vol, i, buf)) != 0) {
			printf("failed reading: %u\r\n", rc);
			return;
		}
		uint16_t j = 0;
		for (j = 0; j < 512; j++) {
			(void)uart_tx(buf[j]);
		}
	}

	_delay_ms(500);

	printf("OK\r\n");
}

static void
read1(uint8_t *buf)
{
	uint32_t offset;
	uint8_t rc;

	offset = rx_uint32_t();

	if (offset >= vol.data_blkno) {
		printf("error: offset (%lu) >= vol->data_blkno (%lu)\r\n",
		    offset, vol.data_blkno);
		return;
	}

	if ((rc = vol.dev_ops.vol_read_blk(&vol, offset, buf)) != 0) {
		printf("failed reading: %u\r\n", rc);
		return;
	}

	printf("OK\r\n");

	_delay_ms(3000);

	for (uint16_t i = 0; i < 512; i++)
		(void)uart_tx(buf[i]);

	printf("OK\r\n");
}

static void
read2(uint8_t *buf)
{
	uint32_t offset;
	uint8_t extent, rc;

	extent = uart_rx();

	offset = rx_uint32_t();

	if (extent >= vol.devno) {
		printf("error: %u >= vol->devno (%u)\r\n",
		    extent, vol.devno);
		return;
	}

	uint32_t sd_noblks = sd_nblocks(extent);
	if (sd_noblks == 0) {
		printf("error: sd[%u] has 0 blocks (check status)\r\n", extent);
		return;
	}
	if (offset >= sd_noblks) {
		printf("error: offset (%lu) >= sd no. of blocks (%lu)\r\n",
		    offset, sd_noblks);
		return;
	}

	if ((rc = sd_read(extent, offset, buf)) != 0) {
		printf("failed reading: %u\r\n", rc);
		return;
	}

	printf("OK\r\n");

	_delay_ms(3000);

	for (uint16_t i = 0; i < 512; i++)
		(void)uart_tx(buf[i]);

	printf("OK\r\n");
}
