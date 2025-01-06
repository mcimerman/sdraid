#include <stdio.h>

#include "var.h"
#include "util.h"
#include "sd.h"

static uint8_t raid0_write(sdvol_t *, uint32_t, void *);
static uint8_t raid0_read(sdvol_t *, uint32_t, void *);

uint8_t
raid0_create(sdvol_t *vol)
{
	printf("raid0_create()\r\n");
	vol->dev_ops.vol_write_blk = raid0_write;
	vol->dev_ops.vol_read_blk = raid0_read;
	vol->dev_ops.vol_blkno = sdraid_util_get_data_blkno;

	for (uint8_t i = 0; i < vol->devno; i++) {
		if (sd_init(i) != 0) {
			printf("initing sd %u failed\r\n", i);
			return (1);
		}
	}

	if (write_metadata(vol) != 0)
		return (1);

	return (0);
}

static uint8_t
raid0_write(sdvol_t *, uint32_t, void *)
{
	printf("raid0_write()\r\n");
	return (0);
}

static uint8_t
raid0_read(sdvol_t *, uint32_t, void *)
{
	printf("raid0_read()\r\n");
	return (0);
}
