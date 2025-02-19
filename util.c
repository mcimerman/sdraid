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
	printf("metadata:\r\n");

	char magic[MAGIC_LEN + 1] = { 0 };
	memcpy(magic, metadata->magic, MAGIC_LEN);

	printf("magic: %s\r\n", magic);
	printf("version: %u\r\n", metadata->version);
	printf("devno: %u\r\n", metadata->devno);
	printf("level: %u\r\n", metadata->level);
	printf("strip_size: %u\r\n", STRIP_SIZE(metadata->strip_size_bits));
	printf("blkno: %lu\r\n", metadata->blkno);
	printf("data_blkno: %lu\r\n", metadata->data_blkno);
	printf("data_offset: %u\r\n", metadata->data_offset);
	printf("index: %u\r\n", metadata->index);
}

void
print_vol_info(sdvol_t *vol)
{
	printf("vol info:\r\n");
	printf("devno: %u\r\n", vol->devno);
	printf("level: %u\r\n", vol->level);
	printf("strip_size: %u\r\n", STRIP_SIZE(vol->strip_size_bits));
	printf("blkno: %lu\r\n", vol->blkno);
	printf("data_blkno: %lu\r\n", vol->data_blkno);
	printf("data_offset: %u\r\n", vol->data_offset);
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
		    i, state_str(vol->extents->state));
	}
}

void
fill_metadata(sdvol_t *vol, metadata_t *metadata)
{
	memcpy(metadata->magic, MAGIC, MAGIC_LEN);
	metadata->version = 0x00; /* XXX */
	metadata->devno = vol->devno;
	metadata->level = vol->level;
	metadata->strip_size_bits = vol->strip_size_bits;
	metadata->blkno = vol->blkno;
	metadata->data_blkno = vol->data_blkno;
	metadata->data_offset = vol->data_offset;
}

uint8_t
write_metadata(sdvol_t *vol)
{
	uint8_t meta_block[BLKSIZE];
	memset(meta_block, 0, BLKSIZE);

	metadata_t metadata = { 0 };

	fill_metadata(vol, &metadata);

	print_metadata(&metadata);

	for (uint8_t i = 0; i < vol->devno; i++) {
		if (vol->extents[i].state == FAULTY)
			continue;

		metadata.index = i;

		memcpy(meta_block, &metadata, sizeof(metadata_t));
		if (sd_write(i, META_OFFSET, meta_block) != 0) {
			printf("failed writing metadata to sd %u\r\n", i);
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

	printf("no succesful metadata read\r\n");
	return (1);

good:
	memcpy(&metadata, meta_block, sizeof(metadata));

	if (strncmp(metadata.magic, MAGIC, MAGIC_LEN) != 0) {
		printf("invalid magic\r\n");
		return (1);
	}

	print_metadata(&metadata);

	vol->devno = metadata.devno;
	vol->level = metadata.level;
	vol->strip_size_bits = metadata.strip_size_bits;
	vol->blkno = metadata.blkno;
	vol->data_blkno = metadata.data_blkno;
	vol->data_offset = metadata.data_offset;

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
