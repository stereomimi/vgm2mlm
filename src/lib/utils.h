#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "def.h"

typedef unsigned short char16_t;
uint32_t vmg2mlm_le_32bit_read(const char* src);
vgm2mlm_status_code_t vgm2mlm_set_timing(vgm2mlm_ctx_t* ctx, uint32_t frequency);
void vgm2mlm_append_mlm_wait_com(vgm2mlm_ctx_t* ctx, uint ticks);

#endif