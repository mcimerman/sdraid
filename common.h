#ifndef _COMMON_H
#define _COMMON_H

#define MAX_DEVNO	3
#define BLKSIZE		512

typedef enum {
	RAID0 = 0,
	RAID1 = 1,
	RAID5 = 5
} sdlevel_t;

/*
 * SNIA
 * Common RAID Disk Data Format
 * Specification
 * Version 2.0 Revision 19
 */
typedef enum {
	HR_RLQ_RAID4_0 = 0,
	HR_RLQ_RAID4_N,
	HR_RLQ_RAID5_0R,
	HR_RLQ_RAID5_NR,
	HR_RLQ_RAID5_NC
} layout_t;

#endif
