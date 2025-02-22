#ifndef _VAR_H
#define _VAR_H

#undef WRITE_ENABLED

#define F_CPU		16000000UL

#include <avr/io.h>

#include "common.h"

#define META_SIZE	1 /* in blocks */
#define META_OFFSET	3 /* in blocks */
#define DATA_OFFSET	((META_SIZE) + (META_OFFSET))
#define MAGIC "RAID"
#define MAGIC_LEN	4

#define STRIP_SIZE(sz)	(1 << (sz))


struct sdvol;
typedef struct sdvol sdvol_t;

/* the API for now */
typedef struct blkdev_ops {
	uint8_t (*vol_write_blk)(sdvol_t *, uint32_t, void *);
	uint8_t (*vol_read_blk)(sdvol_t *, uint32_t, void *);
	uint32_t (*vol_blkno)(sdvol_t *);
} blkdev_ops_t;

typedef struct sdraid_ops {
	uint8_t (*create)(sdvol_t *);
	uint8_t (*init)(sdvol_t *);
} sdraid_ops_t;

typedef enum {
	READ,
	WRITE
} bdop_type_t;

typedef enum {
	OPTIMAL,
	DEGRADED,
	FAULTY
} state_t;

typedef struct extent {
	state_t state;
} extent_t;

struct sdvol {
	sdraid_ops_t ops;
	blkdev_ops_t dev_ops;
	uint8_t devno;
	uint8_t level;
	uint32_t blkno;
	uint32_t data_blkno;
	uint8_t data_offset;
	uint8_t strip_size_bits; /* bit shifted strip size */
	extent_t extents[MAX_DEVNO];
	state_t state;
} __attribute__((packed));

typedef struct metadata {
	char magic[4];
	uint8_t version;
	uint8_t devno;
	uint8_t level;
	uint8_t strip_size_bits;
	uint32_t blkno;
	uint32_t data_blkno;
	uint8_t data_offset;
	uint8_t index; /* currently not used */
} __attribute__((packed)) metadata_t;

typedef struct sdraid_cfg {
	uint8_t devno;
	uint8_t level;
} sdraid_cfg_t;

extern uint8_t raid0_create(sdvol_t *);
extern uint8_t raid1_create(sdvol_t *);

extern uint8_t raid0_init(sdvol_t *);
extern uint8_t raid1_init(sdvol_t *);

#endif
