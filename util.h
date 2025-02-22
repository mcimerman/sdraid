#ifndef _UTIL_H
#define _UTIL_H

#include "var.h"

#ifdef DEBUG
#define DPRINTF(format, ...) \
    printf(format, ##__VA_ARGS__)
#else
#define DPRINTF(...)
#endif

uint32_t sdraid_util_get_data_blkno(sdvol_t *);
void print_metadata(metadata_t *);
void print_vol_info(sdvol_t *);
void fill_metadata(sdvol_t *, metadata_t *);
uint8_t write_metadata(sdvol_t *);
uint8_t read_metadata(sdvol_t *);
void print_vol_state(sdvol_t *);
uint8_t count_dev_state(sdvol_t *, state_t);

#endif
