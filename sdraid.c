#include "var.h"

#include <avr/io.h>
#include <util/delay.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sd.h"
#include "util.h"
#include "sdraid.h"

void
sdraid_print_status(sdvol_t *vol)
{
	print_vol_info(vol);
	print_vol_state(vol);
}

uint8_t
sdraid_create(sdraid_cfg_t *cfg, sdvol_t *vol, bool assemble)
{
	DPRINTF("sdraid_create()\r\n");

	for (uint8_t i = 0; i < MAX_DEVNO; i++)
		vol->extents[i].state = FAULTY;

	if (assemble) {
		DPRINTF("sdraid_create(): assembling\r\n");
		for (uint8_t i = 0; i < MAX_DEVNO; i++) {
			if (sd_init(i) == 0)
				vol->extents[i].state = OPTIMAL;
		}

		DPRINTF("sdraid_create(): reading\r\n");
		if (read_metadata(vol) != 0) {
			printf("failed reading metadata");
			return (1);
		}
	} else {
		DPRINTF("creating volume with devno: %u, level: %u\r\n",
		    cfg->devno, cfg->level);
		vol->devno = cfg->devno;
		vol->level = cfg->level;
		vol->layout = cfg->layout;
		vol->data_offset = DATA_OFFSET;

		for (uint8_t i = 0; i < vol->devno; i++) {
			if (sd_init(i) == 0)
				vol->extents[i].state = OPTIMAL;
			else
				printf("failed initing card no. %u\r\n", i);
		}
	}

	DPRINTF("sdraid_create(): switch\r\n");
	switch (vol->level) {
	case RAID0:
		if (vol->devno < 2) {
			printf("RAID0 needs at least 2 devices\r\n");
			return (1);
		}

		if (!assemble)
			vol->strip_size_bits = 10; /* 1K strips */

		vol->ops.create = raid0_create;
		vol->ops.init = raid0_init;
		break;
	case RAID1:
		if (vol->devno < 2) {
			printf("RAID1 needs at least 2 devices\r\n");
			return (1);
		}
		if (!assemble)
			vol->strip_size_bits = 0;
		vol->ops.create = raid1_create;
		vol->ops.init = raid1_init;
		break;
	case RAID5:
		if (vol->devno < 3) {
			printf("RAID{4,5} needs at least 3 devices\r\n");
			return (1);
		}

		if (!assemble)
			vol->strip_size_bits = 10; /* 1K strips */

		vol->ops.create = raid5_create;
		vol->ops.init = raid5_init;
		break;
	default:
		printf("error: invalid level\r\n");
		return (1);
	}

	if (vol->ops.init(vol) != 0) {
		printf("failed to init the volume\r\n");
		return (1);
	}

	if (!assemble) {
		if (vol->ops.create(vol) != 0)
			return (1);
	}

	DPRINTF("sdraid_create(): sucess\r\n");
	return (0);
}
