#include <stdio.h>

#include "var.h"
#include "util.h"
#include "sd.h"

static uint8_t raid5_write(sdvol_t *, uint32_t, void *);
static uint8_t raid5_read(sdvol_t *, uint32_t, void *);

uint8_t
raid5_create(sdvol_t *vol)
{
	DPRINTF("raid5_create()\r\n");

	/*
	 * XXX: for now take first card's nblocks
	 * TODO: truncate or assert same for all cards
	 */
	vol->blkno = sd_nblocks(0) * (vol->devno - 1);
	vol->data_blkno = (sd_nblocks(0) - vol->data_offset) * (vol->devno - 1);

	if (write_metadata(vol) != 0) {
		DPRINTF("metadata write failed\r\n");
		return (1);
	}

	return (0);
}

uint8_t
raid5_init(sdvol_t *vol)
{
	DPRINTF("raid5_init()\r\n");
	vol->dev_ops.vol_write_blk = raid5_write;
	vol->dev_ops.vol_read_blk = raid5_read;
	vol->dev_ops.vol_blkno = sdraid_util_get_data_blkno;

	uint8_t healthy = count_dev_state(vol, OPTIMAL);

	if (healthy < vol->devno - 1) {
		vol->state = FAULTY;
		printf("raid5_init(): not enough healthy extents\r\n");
		return (1);
	} else if (healthy == vol->devno - 1) {
		vol->state = DEGRADED;
	} else {
		vol->state = OPTIMAL;
	}

	return (0);
}

static uint8_t
raid5_op(sdvol_t *vol, uint32_t ba, void *data, bdop_type_t type)
{
	if (vol->state == FAULTY)
		return (1);

	if (ba >= vol->data_blkno)
		return (2);

	uint32_t strip_size = STRIP_SIZE(vol->strip_size_bits) / BLKSIZE; /* in blocks */
	uint32_t strip_no = ba / strip_size;
	uint32_t stripe = strip_no / (vol->devno - 1);
	uint32_t strip_off = ba % strip_size;

	uint32_t p_extent = 0;

	switch (vol->layout) {
	case HR_RLQ_RAID4_0:
		p_extent = 0;
		break;
	case HR_RLQ_RAID4_N:
		p_extent = (vol->devno - 1);
		break;
	case HR_RLQ_RAID5_0R:
		p_extent = ((strip_no / (vol->devno - 1)) % vol->devno);
		break;
	case HR_RLQ_RAID5_NR:
	case HR_RLQ_RAID5_NC:
		p_extent = ((vol->devno - 1) -
		    (strip_no / (vol->devno - 1)) % vol->devno);
		break;
	default:
		DPRINTF("invalid layout\r\n");
		return (1);
	}

	uint32_t extent = 0;

	switch (vol->layout) {
	case HR_RLQ_RAID4_0:
		extent = ((strip_no % (vol->devno - 1)) + 1);
		break;
	case HR_RLQ_RAID4_N:
		extent = (strip_no % (vol->devno - 1));
		break;
	case HR_RLQ_RAID5_0R:
	case HR_RLQ_RAID5_NR:
		if ((strip_no % (vol->devno - 1)) < p_extent)
			extent = (strip_no % (vol->devno - 1));
		else
			extent = ((strip_no % (vol->devno - 1)) + 1);
		break;
	case HR_RLQ_RAID5_NC:
		extent = (((strip_no % (vol->devno - 1)) + p_extent + 1) %
		    vol->devno);
		break;
	default:
		DPRINTF("invalid layout\r\n");
		return (1);
	}


	ba = stripe * strip_size + strip_off;

	ba += vol->data_offset;

	// XXX: add degraded
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

	if (vol->state == OPTIMAL)
		vol->state = DEGRADED;
	else
		vol->state = FAULTY;

	return (1);
}

static uint8_t
raid5_write(sdvol_t *vol, uint32_t ba, void *data)
{
	DPRINTF("raid5_write()\r\n");
	return (raid5_op(vol, ba, data, WRITE));
}

static uint8_t
raid5_read(sdvol_t *vol, uint32_t ba, void *data)
{
	DPRINTF("raid5_read()\r\n");
	return (raid5_op(vol, ba, data, READ));
}
