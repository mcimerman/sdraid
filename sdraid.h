#ifndef _SDRAID_H
#define _SDRAID_H

#include "var.h"

uint8_t sdraid_create(sdraid_cfg_t *, sdvol_t *, bool);
void sdraid_print_status(sdvol_t *);

#endif
