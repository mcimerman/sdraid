#ifndef _SD_H
#define _SD_H

#include "var.h"

#define GO_IDLE_STATE		0
#define SEND_OP_COND		1
#define SEND_IF_COND		8
#define SEND_CSD		9
#define STOP_TRANSMISSION	12
#define SEND_STATUS		13
#define SET_BLOCK_LEN		16
#define READ_SINGLE_BLOCK	17
#define READ_MULTIPLE_BLOCKS	18
#define WRITE_SINGLE_BLOCK	24
#define WRITE_MULTIPLE_BLOCKS	25
#define ERASE_BLOCK_START_ADDR	32
#define ERASE_BLOCK_END_ADDR	33
#define ERASE_SELECTED_BLOCKS	38
#define SD_SEND_OP_COND		41
#define APP_CMD			55
#define READ_OCR		58
#define CRC_ON_OFF		59

uint8_t sd_init(uint8_t);
uint8_t sd_cmd(uint8_t, uint8_t, uint32_t);
uint32_t sd_nblocks(uint8_t);
uint8_t sd_read(uint8_t, uint32_t, uint8_t *);
uint8_t sd_read_and_xor(uint8_t, uint32_t, uint8_t *);
uint8_t sd_write(uint8_t, uint32_t, uint8_t *);

#endif
