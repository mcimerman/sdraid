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

#define MAX_INPUT_LEN 32

static void debug_menu(void);
static void read_input(void);
static sdraid_cfg_t fill_cfg(bool);
static void menu(void);
static void io_menu(void);
static uint8_t sdraid_create(sdraid_cfg_t *);
static void sdraid_print_status(void);
static void print_block(uint8_t *);

char input[MAX_INPUT_LEN];

sdvol_t vol;

int
main(void)
{
	_delay_ms(100);

	uart_init();

	spi_init();

	printf("Hello, Serial SDRAID World!\r\n");

	menu();

	printf("main() end\r\n");

	return (0);
}

static void
sdraid_print_status(void)
{
	print_vol_info(&vol);
	print_vol_state(&vol);
}


static void
read_input(void)
{
	char c;
	uint8_t i = 0;

	while ((c = uart_rx()) != '\r') {
		printf("%c", c);
		input[i++] = c;
	}

	printf("\r\n");

	input[i] = '\0';
}

static sdraid_cfg_t
fill_cfg(bool assembly)
{
	sdraid_cfg_t cfg = { 0 };

	printf("devno: ");
	read_input();
	cfg.devno = atoi(input);
	printf("\r\n");

	if (assembly)
		return cfg;

	printf("level: ");
	read_input();
	cfg.level = atoi(input);
	printf("\r\n");

	/* TODO: if RAID5 */
	/*
	printf("layout: ");
	int devno = uart_rx();
	printf("\r\n");
	*/

	return cfg;
}

static uint8_t
sdraid_create(sdraid_cfg_t *cfg)
{
	printf("creating volume with devno: %u, level: %u\r\n",
	    cfg->devno, cfg->level);

	vol.devno = cfg->devno;
	vol.level = cfg->level;

	switch (cfg->level) {
	case RAID0:
		vol.strip_size_bits = 12; /* 4K strips */
		vol.ops.create = raid0_create;
		break;
	case RAID1:
		vol.strip_size_bits = 0;
		vol.ops.create = raid1_create;
		break;
	default:
		printf("error: invalid level\r\n");
		return (1);
	}

	if (vol.ops.create(&vol) != 0)
		return (1);

	return (0);
}

/*
 * XXX: check disassembly: see how the prinf gets compiled, if it
 * doesnt generate too much code when printing only stringlits
 */

static void
menu(void)
{
	sdraid_cfg_t cfg = { 0 };

	printf("----- SDRAID -----\r\n");
	for (;;) {
		printf("----- select item -----\r\n");
		printf("'C'  -  Create new volume\r\n");
		printf("'A'  -  Assemble volume\r\n");
		char c = uart_rx();
		switch (c) {
		case 'c':
		case 'C':
			cfg = fill_cfg(false);
			if (sdraid_create(&cfg) != 0) {
				printf("error creating volume\r\n");
				break;
			}
			io_menu();
			break;
		/* TODO */
		/*
		case 'A':
			cfg = fill_cfg();
			sdraid_assemble(cfg);
			io_menu();
			break;
		*/
		default:
			printf("invalid command\r\n");
			break;
		}
	}
}

static void
io_menu(void)
{
	uint32_t off;

	printf("----- IO menu -----\r\n");
	for (;;) {
		printf("----- select item -----\r\n");
		printf("'S'  -  print status\r\n");
		printf("'R'  -  READ from volume\r\n");
		printf("'W'  -  WRITE to volume\r\n");
		printf("'D'  -  DEBUG menu\r\n");
		printf("'Q'  -  return to the default menu\r\n");
		char c = uart_rx();
		switch (c) {
		case 'S':
			sdraid_print_status();
			break;
		case 'r':
		case 'R':
			printf("offset (in blocks): ");
			read_input();
			off = strtol(input, NULL, 10);
			printf("\r\n");

			uint8_t readbuf[BLKSIZE] = { 0 };

			printf("len (in blocks): ");
			read_input();
			uint32_t len = strtol(input, NULL, 10);
			printf("\r\n");

			for (uint32_t i = 0; i < len; i++) {
				if (vol.dev_ops.vol_read_blk(&vol, off, readbuf)
				    != 0) {
					printf("failed reading\r\n");
					break;
				}
				printf("volume block: %lu\r\n", off + i);
				print_block(readbuf);
			}

			break;
		/* TODO */
		/*
		case 'w':
		case 'W':
			printf("block number (indexed from 0): ");
			read_input();
			off = strtol(input, NULL, 10);
			printf("\r\n");

			printf("content to fill the block with: ");
			read_input();
			uint8_t content = uart_rx();
			printf("%c\r\n", content);


			break;
		*/
		case 'd':
		case 'D':
			debug_menu();
			break;
		case 'q':
		case 'Q':
			return;
		default:
			printf("invalid command\r\n");
			break;
		}
	}
}

static void
print_block(uint8_t *buf)
{
	printf("+-----------+\r\n");
	for (uint8_t i = 0; i * 32 < 512; i += 4)
	printf("|%.2x %.2x %.2x %.2x|\r\n", buf[(i + 0)* 32],
	    buf[(i + 1) * 32], buf[(i + 2) * 32], buf[(i + 3) * 32]);
	printf("+-----------+\r\n");
}

static void
debug_menu(void)
{
	printf("----- debug menu -----\r\n");
	for (;;) {
		printf("----- select item -----\r\n");
		printf("'R'  -  READ (will go to extent selection)\r\n");
		printf("'Q'  -  return to the IO menu\r\n");

		char c = uart_rx();
		switch (c) {
		case 'r':
		case 'R':
			printf("extent: ");
			read_input();
			uint8_t extent = atoi(input);
			printf("\r\n");

			printf("offset (in blocks): ");
			read_input();
			uint32_t off = strtol(input, NULL, 10);
			printf("\r\n");

			printf("len (in blocks): ");
			read_input();
			uint32_t len = strtol(input, NULL, 10);
			printf("\r\n");

			uint8_t readbuf[BLKSIZE] = { 0 };

			for (uint32_t i = 0; i < len; i++) {
				if (sd_read(extent, off + i, readbuf) != 0) {
					printf("failed reading\r\n");
					break;
				}
				printf("ext: %u, block: %lu\r\n",
				    extent, off + i);
				print_block(readbuf);
			}

			break;
		case 'q':
		case 'Q':
			return;
		default:
			printf("invalid command\r\n");
			break;
		}
	}
}
