#include <stdio.h>

#include "sd.h"
#include "spi.h"
#include "uart.h"
#include "util.h"


#define WRITE_ENABLED


uint8_t
sd_init(uint8_t dev)
{
	uint8_t response;
	uint16_t retry = 0;

	for(uint8_t i = 0; i < 10; i++)
		spi_tx(0xff);

	ss_enable(dev);

	do {
		response = sd_cmd(dev, GO_IDLE_STATE, 0);

		if (retry++ > 0x20) {
			ss_disable(dev);
			return (1);
		}

	} while (response != 0x01);

	ss_disable(dev);
	spi_tx(0xff);
	spi_tx(0xff);

	retry = 0;
	do {
		response = sd_cmd(dev, SEND_IF_COND, 0x000001AA);

		if (retry++ > 0xfe)
			break;
	} while (response != 0x01);

	retry = 0;
	do {
		response = sd_cmd(dev, APP_CMD, 0);
		response = sd_cmd(dev, SD_SEND_OP_COND, 0x40000000);

		if(retry++ > 0xfe)
			return (1);

	} while (response != 0x00);

	return (0);
}

uint8_t
sd_cmd(uint8_t dev, uint8_t cmd, uint32_t arg)
{
	uint8_t response, retry = 0;

	ss_enable(dev);

	spi_tx(cmd | 0x40);
	spi_tx(arg >> 24 & 0xff);
	spi_tx(arg >> 16 & 0xff);
	spi_tx(arg >> 8 & 0xff);
	spi_tx(arg & 0xff);

	if(cmd == SEND_IF_COND)
		spi_tx(0x87);
	else 
		spi_tx(0x95); 

	while((response = spi_rx()) == 0xff)
		if(retry++ > 0xfe) break;

	spi_rx();
	ss_disable(dev);

	return (response);
}

uint32_t
sd_nblocks(uint8_t dev)
{
	uint8_t response;
	uint16_t retry = 0;
	uint8_t csd[16];

	response = sd_cmd(dev, SEND_CSD, 0);

	if (response != 0x00)
		return (0);

	ss_enable(dev);

	while(spi_rx() != 0xfe)
		if (retry++ > 0xfffe)
			goto error;

	for(uint8_t i = 0; i < 16; i++)
		csd[i] = spi_rx();

	spi_rx();
	spi_rx();

	spi_rx();

	ss_disable(dev);

	uint32_t c_size;
	uint32_t block_count = 0;

	if ((csd[0] >> 6) == 1) {
		c_size =
		    ((uint32_t)(csd[7] & 0x3f) << 16) |
		    ((uint32_t)csd[8] << 8) |
		    (uint32_t)csd[9];

		block_count = (c_size + 1) << 10;
	} else {
		goto error;
	}

	return (block_count);
error:
	ss_disable(dev);
	DPRINTF("error while getting blkno of sd[%u]\r\n", dev);
	return (0);
}

uint8_t
sd_read(uint8_t dev, uint32_t ba, uint8_t *buf)
{
	uint8_t response;
	uint16_t retry = 0;

	response = sd_cmd(dev, READ_SINGLE_BLOCK, ba);

	if(response != 0x00)
		return (response);

	ss_enable(dev);

	while(spi_rx() != 0xfe)
		if (retry++ > 0xfffe)
			goto error;

	for(uint16_t i = 0; i < BLKSIZE; i++)
		buf[i] = spi_rx();

	spi_rx();
	spi_rx();

	spi_rx();
	ss_disable(dev);

	return (0);
error:
	ss_disable(dev);
	return (1);
}

uint8_t
sd_read_and_xor(uint8_t dev, uint32_t ba, uint8_t *buf)
{
	uint8_t response;
	uint16_t retry = 0;

	response = sd_cmd(dev, READ_SINGLE_BLOCK, ba);

	if(response != 0x00)
		return (response);

	ss_enable(dev);

	while(spi_rx() != 0xfe)
		if (retry++ > 0xfffe)
			goto error;

	for(uint16_t i = 0; i < BLKSIZE; i++)
		buf[i] ^= spi_rx();

	spi_rx();
	spi_rx();

	spi_rx();
	ss_disable(dev);

	return (0);
error:
	ss_disable(dev);
	return (1);
}

uint8_t
sd_write(uint8_t dev, uint32_t ba, uint8_t *buf)
{
#ifndef WRITE_ENABLED
	return (0);
#endif
	uint8_t response;
	uint16_t retry = 0;

	response = sd_cmd(dev, WRITE_SINGLE_BLOCK, ba);

	if(response != 0x00)
		return (response);

	ss_enable(dev);

	spi_tx(0xfe);

	for(uint16_t i = 0; i < BLKSIZE; i++)
		spi_tx(buf[i]);

	spi_tx(0xff);
	spi_tx(0xff);

	response = spi_rx();

	if( (response & 0x1f) != 0x05) {
		ss_disable(dev);
		return (response);
	}

	while(!spi_rx())
		if (retry++ > 0xfffe) 
			goto error;

	ss_disable(dev);
	spi_tx(0xff);
	ss_enable(dev);

	while(!spi_rx())
		if (retry++ > 0xfffe)
			goto error;

	ss_disable(dev);

	return (0);
error:
	ss_disable(dev);
	return (1);
}
