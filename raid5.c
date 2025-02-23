#include <stdio.h>
#include <string.h>

#include "var.h"
#include "util.h"
#include "sd.h"

static uint8_t raid5_read(sdvol_t *, uint32_t, void *);
static uint8_t raid5_write(sdvol_t *, uint32_t, void *);
static uint8_t calculate_data_extent(sdvol_t *, uint8_t, uint32_t);
static uint8_t calculate_parity_extent(sdvol_t *, uint32_t);
static uint32_t raid5_geometry(sdvol_t *, uint32_t, uint8_t *, uint8_t *);
static uint8_t raid5_op(sdvol_t *, uint32_t, void *, bdop_type_t);
static uint8_t raid5_internal_read(sdvol_t *, uint32_t, uint8_t, void *,
    uint8_t *);
static uint8_t raid5_internal_write(sdvol_t *, uint32_t, uint8_t, uint8_t,
    void *, uint8_t *);


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
		printf("not enough healthy extents\r\n");
		return (1);
	} else if (healthy == vol->devno - 1) {
		vol->state = DEGRADED;
	} else {
		vol->state = OPTIMAL;
	}

	return (0);
}

static uint8_t
raid5_read(sdvol_t *vol, uint32_t ba, void *data)
{
	DPRINTF("raid5_read()\r\n");
	return (raid5_op(vol, ba, data, READ));
}

static uint8_t
raid5_write(sdvol_t *vol, uint32_t ba, void *data)
{
	DPRINTF("raid5_write()\r\n");
	return (raid5_op(vol, ba, data, WRITE));
}

static uint8_t
calculate_parity_extent(sdvol_t *vol, uint32_t strip_no)
{
	uint8_t p_extent;

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
		p_extent = 0xff;
	}

	return (p_extent);
}

static uint8_t
calculate_data_extent(sdvol_t *vol, uint8_t p_extent, uint32_t strip_no)
{
	uint8_t extent;

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
		extent = 0xff;
	}

	return (extent);
}

/*
 * Returns physical block address
 */
static uint32_t
raid5_geometry(sdvol_t *vol, uint32_t ba, uint8_t *extent, uint8_t *p_extent)
{
	uint32_t strip_size = STRIP_SIZE(vol->strip_size_bits) / BLKSIZE; /* in blocks */
	uint32_t strip_no = ba / strip_size;
	uint32_t stripe = strip_no / (vol->devno - 1);
	uint32_t strip_off = ba % strip_size;

	*p_extent = calculate_parity_extent(vol, strip_no);

	*extent = calculate_data_extent(vol, *p_extent, strip_no);

	return (stripe * strip_size + strip_off);
}

static uint8_t
raid5_op(sdvol_t *vol, uint32_t ba, void *data, bdop_type_t type)
{
	uint8_t extent, p_extent;
	uint8_t xorbuf[BLKSIZE];

	if (vol->state == FAULTY)
		return (1);

	if (ba >= vol->data_blkno)
		return (2);

	ba = raid5_geometry(vol, ba, &extent, &p_extent);

	ba += vol->data_offset;

	switch (type) {
	case READ:
		return (raid5_internal_read(vol, ba, extent, data, xorbuf));
	case WRITE:
		return (raid5_internal_write(vol, ba, extent, p_extent, data,
		    xorbuf));
	default:
		return (3);
	}
}

static uint8_t
raid5_internal_read(sdvol_t *vol, uint32_t ba, uint8_t extent, void *data,
    uint8_t *xorbuf)
{
retry_read:
	memset(xorbuf, 0, BLKSIZE);
	if (vol->extents[extent].state == OPTIMAL) {
		if (sd_read(extent, ba, data) != 0) {
			vol->extents[extent].state = FAULTY;
			if (vol->state == DEGRADED) {
				goto error;
			} else {
				vol->state = DEGRADED;
				goto retry_read;
			}
		}
	} else {
		for (uint8_t i = 0; i < vol->devno; i++) {
			if (i == extent)
				continue;
			if (sd_read_and_xor(i, ba, xorbuf) != 0) {
				vol->extents[i].state = FAULTY;
				goto error;
			}
		}
		memcpy(data, xorbuf, BLKSIZE);
	}

	return (0);
error:
	vol->state = FAULTY;

	return (1);
}

static uint8_t
raid5_internal_write(sdvol_t *vol, uint32_t ba, uint8_t extent,
    uint8_t p_extent, void *data, uint8_t *xorbuf)
{
	uint8_t bad;

retry_write:
	memset(xorbuf, 0, BLKSIZE);
	switch (vol->state) {
	case OPTIMAL:
		if (sd_write(extent, ba, data) != 0) {
			vol->extents[extent].state = FAULTY;
			vol->state = DEGRADED;
			goto retry_write;
		}

		memcpy(xorbuf, data, BLKSIZE);

		/* calculate parity */
		for (uint8_t i = 0; i < vol->devno; i++) {
			if (i == p_extent || i == extent)
				continue;
			if (sd_read_and_xor(i, ba, xorbuf) != 0) {
				vol->extents[i].state = FAULTY;
				vol->state = DEGRADED;
				goto retry_write;
			}
		}

		if (vol->extents[p_extent].state != OPTIMAL)
			break;
		/* write parity */
		if (sd_write(p_extent, ba, xorbuf) != 0) {
			vol->extents[p_extent].state = FAULTY;
			vol->state = DEGRADED;
			goto retry_write;
		}
		break;
	case DEGRADED:
		bad = get_state_dev(vol, FAULTY);
		if (bad == vol->devno)
			return (3); /* EINVAL */

		if (extent == bad) {
			/*
			 * new parity = read other and xor in new data
			 *
			 * write new parity
			 */
			memcpy(xorbuf, data, BLKSIZE);
			for (uint8_t i = 0; i < vol->devno; i++) {
				if (i == bad || i == p_extent)
					continue;
				if (sd_read_and_xor(i, ba, xorbuf) != 0) {
					vol->extents[i].state = FAULTY; 
					vol->state = FAULTY;
					goto error;
				}
			}

			if (vol->extents[p_extent].state != OPTIMAL)
				goto error;
			/* write parity */
			if (sd_write(p_extent, ba, xorbuf) != 0) {
				vol->extents[p_extent].state = FAULTY; 
				goto error;
			}
		} else { /* another extent is FAULTY */
			/*
			 * new parity = original data ^ old parity ^ new data
			 *
			 * write parity, new data
			 */
			if (sd_read(extent, ba, xorbuf) != 0) {
				vol->extents[extent].state = FAULTY; 
				goto error;
			}

			if (sd_read_and_xor(p_extent, ba, xorbuf) != 0) {
				vol->extents[extent].state = FAULTY; 
				goto error;
			}

			uint8_t *data_ = data;

			for (uint16_t i = 0; i < BLKSIZE; i++)
				xorbuf[i] ^= data_[i];

			if (sd_write(extent, ba, data_) != 0) {
				vol->extents[extent].state = FAULTY; 
				goto error;
			}

			if (sd_write(p_extent, ba, xorbuf) != 0) {
				vol->extents[p_extent].state = FAULTY; 
				goto error;
			}
		}
		break;
	default:
		return (1); /* FAULTY */
	}

	return (0);
error:
	vol->state = FAULTY;

	return (1);
}
