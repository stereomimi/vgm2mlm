#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

uint32_t vmg2mlm_le_32bit_read(const char* src)
{
	uint32_t integer = src[0] & 255;
	integer |= (src[1] & 255) << 8;
	integer |= (src[2] & 255) << 16;
	integer |= (src[3] & 255) << 24;

	return integer;
}

#endif