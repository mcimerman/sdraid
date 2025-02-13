#include <stdio.h>

#include "sd.h"
#include "var.h"
#include "util.h"

static uint8_t raid1_write(sdvol_t *, uint32_t, void *);
static uint8_t raid1_read(sdvol_t *, uint32_t, void *);

uint8_t
raid1_create(sdvol_t *vol)
{
	printf("raid1_create()\r\n");

	/*
	 * XXX: for now take first card's nblocks
	 * TODO: truncate or assert same for all cards
	 */
	vol->blkno = sd_nblocks(0);
	vol->data_blkno = sd_nblocks(0) - DATA_OFFSET;

	if (write_metadata(vol) != 0) {
		printf("metadata write failed\r\n");
		return (1);
	}

	return (0);
}

uint8_t
raid1_init(sdvol_t *vol)
{
	printf("raid1_init()\r\n");
	vol->dev_ops.vol_write_blk = raid1_write;
	vol->dev_ops.vol_read_blk = raid1_read;
	vol->dev_ops.vol_blkno = sdraid_util_get_data_blkno;

	uint8_t healthy = count_dev_state(vol, OPTIMAL);

	if (healthy < 1) {
		vol->state = FAULTY;
		return (1);
	} else if (healthy < vol->devno) {
		vol->state = DEGRADED;
	} else {
		vol->state = OPTIMAL;
	}

	return (0);
}

static uint8_t
raid1_write(sdvol_t *vol, uint32_t ba, void *data)
{
	printf("raid1_write()\r\n");

	if (vol->state == FAULTY)
		return (1);

	if (ba >= vol->data_blkno)
		return (2);

	ba += DATA_OFFSET;

	uint8_t succesful = 0;
	for (uint8_t i = 0; i < vol->devno; i++) {
		if (vol->extents[i].state != OPTIMAL)
			continue;
		if (sd_write(i, ba, data) != 0) {
			vol->extents[i].state = FAULTY;
			continue;
		}
		succesful++;
	}

	if (succesful == 0) {
		vol->state = FAULTY;
		return (1);
	} else if (succesful < vol->devno) {
		vol->state = DEGRADED;
	}

	return (0);
}

static uint8_t
raid1_read(sdvol_t *vol, uint32_t ba, void *data)
{
	printf("raid1_read()\r\n");

	if (vol->state == FAULTY)
		return (1);

	if (ba >= vol->data_blkno)
		return (2);

	ba += DATA_OFFSET;

	for (uint8_t i = 0; i < vol->devno; i++) {
		if (vol->extents[i].state != OPTIMAL)
			continue;

		if (sd_read(i, ba, data) == 0)
			return (0);

		vol->extents[i].state = FAULTY;
	}

	vol->state = FAULTY;
	return (1);
}
