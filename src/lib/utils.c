#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "def.h"

uint32_t vmg2mlm_le_32bit_read(const char* src)
{
	uint32_t integer = src[0] & 255;
	integer |= (src[1] & 255) << 8;
	integer |= (src[2] & 255) << 16;
	integer |= (src[3] & 255) << 24;

	return integer;
}

// Sets frequency, and may also set the base time
vgm2mlm_status_code_t vgm2mlm_set_timing(vgm2mlm_ctx_t* ctx, uint32_t frequency)
{
	ctx->frequency = frequency;

	if (ctx->frequency == 0)
		ctx->frequency = 60;
	else if (ctx->frequency > TMA_MAX_FREQ)
		return VGM2MLM_STERR_UNSUPPORTED_FREQUENCY;
	else if (ctx->frequency < TMA_MIN_FREQ)
	{
		for (int i = 2; i < 256; ++i)
		{
			if (ctx->frequency*i > TMA_MAX_FREQ)
				return VGM2MLM_STERR_UNSUPPORTED_FREQUENCY;
			else if (ctx->frequency*i > TMA_MIN_FREQ)
			{
				ctx->base_time = i;
				ctx->frequency *= i;
				break;
			}
		}

		if (ctx->frequency < TMA_MIN_FREQ)
			return VGM2MLM_STERR_UNSUPPORTED_FREQUENCY;
	}

	return VGM2MLM_STSUCCESS;
}

#endif