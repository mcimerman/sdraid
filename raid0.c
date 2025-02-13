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

	/*
	 * XXX: for now take first card's nblocks
	 * TODO: truncate or assert same for all cards
	 */
	vol->blkno = sd_nblocks(0) * vol->devno;
	vol->data_blkno = (sd_nblocks(0) - DATA_OFFSET) * vol->devno;

	if (write_metadata(vol) != 0) {
		printf("metadata write failed\r\n");
		return (1);
	}

	return (0);
}

uint8_t
raid0_init(sdvol_t *vol)
{
	printf("raid0_init()\r\n");
	vol->dev_ops.vol_write_blk = raid0_write;
	vol->dev_ops.vol_read_blk = raid0_read;
	vol->dev_ops.vol_blkno = sdraid_util_get_data_blkno;

	uint8_t healthy = count_dev_state(vol, OPTIMAL);

	if (healthy != vol->devno) {
		vol->state = FAULTY;
		return (1);
	} else {
		vol->state = OPTIMAL;
	}

	return (0);
}

static uint8_t
raid0_op(sdvol_t *vol, uint32_t ba, void *data, bdop_type_t type)
{
	if (vol->state == FAULTY)
		return (1);

	if (ba >= vol->data_blkno)
		return (2);

	uint32_t strip_size = STRIP_SIZE(vol->strip_size_bits) / BLKSIZE; /* in blocks */
	uint32_t strip_no = ba / strip_size;
	uint32_t extent = strip_no % vol->devno;
	uint32_t stripe = strip_no / vol->devno;
	uint32_t strip_off = ba % strip_size;

	ba = stripe * strip_size + strip_off;

	ba += DATA_OFFSET;

	switch (type) {
	case READ:
		if (sd_read(extent, ba, data) != 0)
			goto error;
		break;
	case WRITE:
		if (sd_write(extent, ba, data) != 0)
			goto error;
		break;
	default:
		return (3);
	}

	return (0);
error:
	vol->extents[extent].state = FAULTY;
	vol->state = FAULTY;
	return (1);
}

static uint8_t
raid0_write(sdvol_t *vol, uint32_t ba, void *data)
{
	printf("raid0_write()\r\n");
	return (raid0_op(vol, ba, data, WRITE));
}

static uint8_t
raid0_read(sdvol_t *vol, uint32_t ba, void *data)
{
	printf("raid0_read()\r\n");
	return (raid0_op(vol, ba, data, READ));
}
