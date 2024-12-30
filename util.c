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
	memset(meta_block, 0xff, BLKSIZE);

	metadata_t metadata = { 0 };

	fill_metadata(vol, &metadata);

	print_metadata(&metadata);

	for (uint8_t i = 0; i < vol->devno; i++) {
		metadata.index = i;

		memcpy(meta_block, &metadata, sizeof(metadata_t));
#if WRITE_ENABLED
		if (sd_write(i, META_OFFSET, meta_block) != 0) {
			printf("failed writing metadata to sd %u\r\n", i);
			return (1);
		}
#else
		printf("not writing metadata: WRITEs are DISABLED\r\n");
#endif
	}

	return (0);
}
