#include <stdio.h>
#include <string.h>

#include "util.h"
#include "var.h"
#include "sd.h"

uint32_t
sdraid_util_get_data_blkno(sdvol_t *vol)
{
	return (vol->data_blkno);
}

void
print_metadata(metadata_t *metadata)
{
	DPRINTF("metadata:\r\n");

	char magic[MAGIC_LEN + 1] = { 0 };
	memcpy(magic, metadata->magic, MAGIC_LEN);

	DPRINTF("magic: %s\r\n", magic);
	DPRINTF("version: %u\r\n", metadata->version);
	DPRINTF("devno: %u\r\n", metadata->devno);
	DPRINTF("level: %u\r\n", metadata->level);
	if (metadata->level == RAID5) {
		DPRINTF("layout: %u\r\n", metadata->layout);
	}
	DPRINTF("strip_size: %u\r\n", STRIP_SIZE(metadata->strip_size_bits));
	DPRINTF("blkno: %lu\r\n", metadata->blkno);
	DPRINTF("data_blkno: %lu\r\n", metadata->data_blkno);
	DPRINTF("data_offset (in blocks): %u\r\n", metadata->data_offset);
	DPRINTF("index: %u\r\n", metadata->index);
	DPRINTF("file_size: %lu\r\n", metadata->file_size);
}

void
print_vol_info(sdvol_t *vol)
{
	printf("vol info:\r\n");
	printf("devno: %u\r\n", vol->devno);
	printf("level: %u\r\n", vol->level);
	if (vol->level == RAID5)
		printf("layout: %u\r\n", vol->layout);
	printf("strip_size: %u\r\n", STRIP_SIZE(vol->strip_size_bits));
	printf("blkno: %lu\r\n", vol->blkno);
	printf("data_blkno: %lu\r\n", vol->data_blkno);
	printf("data_offset (in blocks): %u\r\n", vol->data_offset);
	printf("file_size: %lu\r\n", vol->file_size);
}

static const char *
state_str(state_t state)
{
	switch (state) {
	case OPTIMAL:
		return "OPTIMAL";
	case DEGRADED:
		return "DEGRADED";
	case FAULTY:
		return "FAULTY";
	default:
		return "Invalid state";
	}
}

void
print_vol_state(sdvol_t *vol)
{
	printf("volume status: %s\r\n", state_str(vol->state));
	printf("extents status: index status\r\n");
	for (uint8_t i = 0; i < vol->devno; i++) {
		printf("                  %d    %s\r\n",
		    i, state_str(vol->extents[i].state));
	}
}

void
fill_metadata(sdvol_t *vol, metadata_t *metadata)
{
	memcpy(metadata->magic, MAGIC, MAGIC_LEN);
	metadata->version = 0x00; /* XXX */
	metadata->devno = vol->devno;
	metadata->level = vol->level;
	metadata->layout = vol->layout;
	metadata->strip_size_bits = vol->strip_size_bits;
	metadata->blkno = vol->blkno;
	metadata->data_blkno = vol->data_blkno;
	metadata->data_offset = vol->data_offset;
	metadata->file_size = vol->file_size;
}

uint8_t
write_metadata(sdvol_t *vol)
{
	uint8_t meta_block[BLKSIZE];
	memset(meta_block, 0, BLKSIZE);

	metadata_t metadata = { 0 };

	fill_metadata(vol, &metadata);

	for (uint8_t i = 0; i < vol->devno; i++) {
		if (vol->extents[i].state == FAULTY)
			continue;

		metadata.index = i;

		memcpy(meta_block, &metadata, sizeof(metadata_t));
		if (sd_write(i, META_OFFSET, meta_block) != 0) {
			DPRINTF("failed writing metadata to sd %u\r\n", i);
			return (1);
		}
	}

	return (0);
}

uint8_t
read_metadata(sdvol_t *vol)
{
	uint8_t meta_block[BLKSIZE];
	metadata_t metadata;

	/* XXX: assume devs are in the same order as assembled */
	for (uint8_t i = 0; i < MAX_DEVNO; i++) {
		if (sd_read(i, META_OFFSET, meta_block) == 0)
			goto good;
	}

	DPRINTF("no succesful metadata read\r\n");
	return (1);

good:
	memcpy(&metadata, meta_block, sizeof(metadata));

	if (strncmp(metadata.magic, MAGIC, MAGIC_LEN) != 0) {
		printf("invalid magic\r\n");
		return (1);
	}

	vol->devno = metadata.devno;
	vol->level = metadata.level;
	vol->layout = metadata.layout;
	vol->strip_size_bits = metadata.strip_size_bits;
	vol->blkno = metadata.blkno;
	vol->data_blkno = metadata.data_blkno;
	vol->data_offset = metadata.data_offset;
	vol->file_size = metadata.file_size;

	return (0);
}

uint8_t
count_dev_state(sdvol_t *vol, state_t state)
{
	uint8_t k = 0;

	for (uint8_t i = 0; i < vol->devno; i++) {
		if (vol->extents[i].state == state)
			k++;
	}

	return (k);
}

uint8_t
get_state_dev(sdvol_t *vol, state_t state)
{
	for (uint8_t i = 0; i < vol->devno; i++) {
		if (vol->extents[i].state == state)
			return (i);
	}

	return (vol->devno);
}

int
stack_left(void)
{
	extern int *__brkval;
	extern int __heap_start;
	int local_var;

	if ((int)__brkval == 0)
		return (int)&local_var - (int)&__heap_start;
	else
		return (int)&local_var - (int) __brkval;
}
