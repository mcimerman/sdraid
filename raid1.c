#include "var.h"
#include "util.h"

static uint8_t raid1_write(sdvol_t *, uint32_t, void *);
static uint8_t raid1_read(sdvol_t *, uint32_t, void *);

uint8_t
raid1_create(sdvol_t *vol)
{
	vol->dev_ops.vol_write_blk = raid1_write;
	vol->dev_ops.vol_read_blk = raid1_read;
	vol->dev_ops.vol_blkno = sdraid_util_get_data_blkno;

	(void)write_metadata(vol);
	return (0);
}

static uint8_t
raid1_write(sdvol_t *, uint32_t, void *)
{
	return (0);
}

static uint8_t
raid1_read(sdvol_t *, uint32_t, void *)
{
	return (0);
}
