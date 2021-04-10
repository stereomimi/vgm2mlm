#include <math.h>
#include <inttypes.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "vgmcom.h"
#include "utils.h"
#include "def.h"
#include "driver.m1.h"
#include "driver.p1.h"
#include "driver.s1.h"

const char* VGM2MLM_STATUS_MESSAGES[VGM2MLM_STATUS_COUNT] =
{
	"SUCCESS",
	"ERROR: This vgm file doesn't use the YM2610 soundchip",
	"ERROR: MLM buffer overflow",
	"ERROR: Invalid VGM command",
	"ERROR: Failed reallocation",
	"ERROR: Unsupported VGM data block type",
	"ERROR: PCM Ram writes aren't supported",
	"ERROR: DAC stream writes aren't supported",
	"ERROR: Invalid or corrupted VGM file",
	"ERROR: Invalid or corrupted GD3 tag",
	"ERROR: Failed to load file into buffer",
	"ERROR: Failed to to write buffer to file",
	"ERROR: The VGM file is corrupted",
	"ERROR: Unsupported frequency",
	"ERROR: Failed to allocate memory",
	"ERROR: Metadata buffer overlow",
	"ERROR: Corrupted loop offset"
};

const uint16_t MLM_HEADER[14] = 
{
	0x0002, /* Song 0 offset */

	0x001F, /* Song 0, Channel 0  offset */
	0x0000, /* Song 0, Channel 1  offset */
	0x0000, /* Song 0, Channel 2  offset */
	0x0000, /* Song 0, Channel 3  offset */
	0x0000, /* Song 0, Channel 4  offset */
	0x0000, /* Song 0, Channel 5  offset */
	0x0000, /* Song 0, Channel 6  offset */
	0x0000, /* Song 0, Channel 7  offset */
	0x0000, /* Song 0, Channel 8  offset */
	0x0000, /* Song 0, Channel 9  offset */
	0x0000, /* Song 0, Channel 10 offset */
	0x0000, /* Song 0, Channel 11 offset */
	0x0000, /* Song 0, Channel 12 offset */
};

const uint8_t BLOCK_OFFSET = 1;
const uint8_t BLOCK_COUNT = (64-BLOCK_OFFSET);
const size_t BLOCK_SIZE = 0x2000;
const size_t WRAM_SIZE = 0x0200;
const size_t MLM_BUFFER_SIZE = BLOCK_SIZE * BLOCK_COUNT - WRAM_SIZE;
const size_t BIGGEST_COM_SIZE = 35; // Biggest possible size for a command (YM2610 Port A/B write x17)

// returns the number of characters in
// the source string, which is equal to
// the size of the destination string.
// the destination string is guaranteed
// to be null terminated.
size_t vgm2mlm_char16_to_char8_str(const char16_t* src, char* dest, size_t max_size)
{
	DEBUG_PRINTF("8to16: ");
	for (int i = 0; i < max_size; i++)
	{
		dest[i] = src[i] & 255;
		DEBUG_PRINTF("%c", src[i] & 255);

		if (src[i] == 0)
		{
			DEBUG_PRINTF("\n");
			return i+1;
		}
	}

	DEBUG_PRINTF("\n");
	dest[max_size-1] = 0;
	return max_size;
}

// The terminator character is included
// in the string length.
size_t vgm2mlm_char16_strlen(const char16_t* str)
{
	size_t len = 0;
	DEBUG_PRINTF("strlen: ");
	for (; *str != 0; str++)
	{
		DEBUG_PRINTF("%c", *str & 255);
		len++;
	}
	DEBUG_PRINTF("\n");
	return len+1;
}

// the size should be dividible by two. probably doesn't work if src and dest are the same array.
void vgm2mlm_swap_bytes(const char* src, char* dest, size_t size)
{
	for (int i = 0; i < size; i += 2)
	{
		dest[i]   = src[i+1];
		dest[i+1] = src[i];
	}
}

vgm2mlm_status_code_t vgm2mlm_parse_gd3(vgm2mlm_ctx_t* ctx, char* vgm_buffer, size_t vgm_size)
{
	uint32_t gd3_offset =
		vmg2mlm_le_32bit_read(vgm_buffer+0x14);

	if (gd3_offset != 0)
	{
		gd3_offset += 0x14;

		DEBUG_PRINTF("gd3 offset\t0x%08X\n", gd3_offset);
		if (vgm_size < gd3_offset)
			return VGM2MLM_STERR_BAD_VGM_FILE;

		char* gd3_data = vgm_buffer+gd3_offset;
		
		if (strncmp(gd3_data, "Gd3 ", 4) != 0)
			return VGM2MLM_STERR_INVALID_GD3_TAG;
		
		char16_t* gd3_strings = (char16_t*)(gd3_data+12);
		
		gd3_strings += 
			vgm2mlm_char16_to_char8_str(gd3_strings, ctx->track_name, TRACK_INFO_LINE_LENGTH);
		gd3_strings +=
			vgm2mlm_char16_strlen(gd3_strings); // skip the japanese track name
		gd3_strings +=
			vgm2mlm_char16_strlen(gd3_strings); // skip the english game name
		gd3_strings +=
			vgm2mlm_char16_strlen(gd3_strings); // skip the japanese game name
		gd3_strings +=
			vgm2mlm_char16_strlen(gd3_strings); // skip the english system name
		gd3_strings +=
			vgm2mlm_char16_strlen(gd3_strings); // skip the japanese system name
		gd3_strings += 
			vgm2mlm_char16_to_char8_str(gd3_strings, ctx->track_author, TRACK_INFO_LINE_LENGTH);

		DEBUG_PRINTF("track name\t%s\n", ctx->track_name);
		DEBUG_PRINTF("track author\t%s\n", ctx->track_author);
	}

	return VGM2MLM_STSUCCESS;
}

vgm2mlm_status_code_t vgm2mlm_store_metadata_in_prom(const vgm2mlm_ctx_t* ctx, char* unswapped_prom)
{
	const char FORMAT[] = "Track: %s\nAuthor:%s\nMade with Delek's Deflemask Tracker";
	const uint16_t METADATA_OFFSET = 0x0A3E;
	const size_t METADATA_CAPACITY = 792;

	const size_t FORMAT_LENGTH = sizeof(FORMAT) - 4;
	size_t track_length = strlen(ctx->track_name);
	size_t author_length = strlen(ctx->track_author);
	size_t metadata_size = FORMAT_LENGTH + track_length + author_length;

	if (metadata_size > METADATA_CAPACITY)
		return VGM2MLM_STERR_METADATA_OVERFLOW;

	sprintf(unswapped_prom+METADATA_OFFSET, FORMAT, ctx->track_name, ctx->track_author);
	return VGM2MLM_STSUCCESS;
}

vgm2mlm_status_code_t vgm2mlm_parse_vgm_header(vgm2mlm_ctx_t* ctx, char* vgm_buffer, size_t vgm_size)
{
	vgm2mlm_status_code_t status = VGM2MLM_STSUCCESS;

	if (vgm_size <= 0x4C)
		return VGM2MLM_STERR_BAD_VGM_FILE;

	if (strncmp(vgm_buffer, "Vgm ", 4) != 0)
		return VGM2MLM_STERR_INVALID_VGM_FILE;

	uint32_t ym2610_clock = vmg2mlm_le_32bit_read(vgm_buffer+0x4C);
	if (!ym2610_clock)
		return VGM2MLM_STERR_NO_YM2610;

	uint32_t loop_samples = vmg2mlm_le_32bit_read(vgm_buffer+0x20);
	
	if (loop_samples == 0)
	{
		ctx->vgm_loop_offset = 0;
	}
	else
	{
		ctx->vgm_loop_offset = vmg2mlm_le_32bit_read(vgm_buffer+0x1C);
		if (ctx->vgm_loop_offset != 0)
			ctx->vgm_loop_offset += 0x1C;
	}

	DEBUG_PRINTF("loop samples\t0x%08X\n", loop_samples);
	DEBUG_PRINTF("vgm loop ofs\t0x%08X\n", ctx->vgm_loop_offset);

	ctx->vgm_data_offset =
		vmg2mlm_le_32bit_read(vgm_buffer+0x34);
	ctx->vgm_data_offset += 0x34;

	DEBUG_PRINTF("vgm data ofs\t0x%08X\n", ctx->vgm_data_offset);

	// If the frequency is 0 (thus treated as not specified) and if the automatic
	// detection of the frequency from the first wait command is off, then get the
	// frequency from the VGM header.
	if (ctx->frequency == 0)
	{
		uint32_t vgm_frequency = 
			vmg2mlm_le_32bit_read(vgm_buffer+0x24);
		
		status = vgm2mlm_set_timing(ctx, vgm_frequency);

		if (status != VGM2MLM_STSUCCESS)
			return status;
	}

	status = vgm2mlm_parse_gd3(ctx, vgm_buffer, vgm_size);
	return status;
}

vgm2mlm_status_code_t vgm2mlm_create_rom(vgm2mlm_ctx_t* ctx, vgm2mlm_output_t* output)
{
	output->m1rom_size = (ctx->current_block+BLOCK_OFFSET+1)*BLOCK_SIZE;
	DEBUG_PRINTF("m1rom size\t%d\n", output->m1rom_size);
	
	output->m1rom_buffer = 
		(char*)malloc(output->m1rom_size);
	if (output->m1rom_buffer == NULL)
		return VGM2MLM_STERR_FAILED_MEM_ALLOCATION;

	memcpy(output->m1rom_buffer, driver_m1, driver_m1_size);
	memcpy(output->m1rom_buffer+(BLOCK_SIZE*BLOCK_OFFSET), ctx->mlm_buffer, (ctx->current_block+1)*BLOCK_SIZE);
	DEBUG_PRINTF("m1rom mlm ofs\t0x%05X\n", BLOCK_SIZE*BLOCK_OFFSET);
	DEBUG_PRINTF("m1rom mlm size\t0x%05X\n", (ctx->current_block+1)*BLOCK_SIZE);

	output->vrom_buffer = ctx->vrom_buffer;
	output->vrom_size = ctx->vrom_buffer_size;

	char* unswapped_prom =
		(char*)malloc(PROM_SIZE);
	if (unswapped_prom == NULL)
		return VGM2MLM_STERR_FAILED_MEM_ALLOCATION;

	memset(unswapped_prom, 0xFF, PROM_SIZE);
	memcpy(unswapped_prom, driver_p1, driver_p1_size);

	vgm2mlm_status_code_t status =
		vgm2mlm_store_metadata_in_prom(ctx, unswapped_prom);
	if (status != VGM2MLM_STSUCCESS)
		return status;

	output->prom_buffer =
		(char*)malloc(PROM_SIZE);
	if (output->prom_buffer == NULL)
	{
		free(unswapped_prom);
		return VGM2MLM_STERR_FAILED_MEM_ALLOCATION;
	}

	vgm2mlm_swap_bytes(unswapped_prom, output->prom_buffer, PROM_SIZE);
	
	free(unswapped_prom);
	return VGM2MLM_STSUCCESS;
}

bool vgm2mlm_is_block_full(vgm2mlm_ctx_t* ctx)
{
	return (ctx->mlm_head - ctx->mlm_buffer) >= BLOCK_SIZE * (ctx->current_block+1) - BIGGEST_COM_SIZE - 1;
}

vgm2mlm_status_code_t vgm2mlm(char* vgm_buffer, size_t vgm_size, int frequency, vgm2mlm_output_t* output, uint32_t flags)
{
	char* final_block_coms[BLOCK_COUNT];
	vgm2mlm_ctx_t ctx;
	
	MEMCLEAR(final_block_coms, sizeof(final_block_coms)); // clear final_block_coms

	ctx.frequency = frequency;
	ctx.base_time = 1;
	ctx.conversion_flags = flags;
	ctx.porta_reg_writes_idx = 0;
	ctx.portb_reg_writes_idx = 0;
	ctx.is_porta_reg_writes_buffer_empty = true;
	ctx.is_portb_reg_writes_buffer_empty = true;

	vgm2mlm_status_code_t status =
		vgm2mlm_parse_vgm_header(&ctx, vgm_buffer, vgm_size);
	if (status != VGM2MLM_STSUCCESS)
		return status;

	DEBUG_PRINTF("ctx.base_time: %d\n", ctx.base_time);

	char* vgm_data = vgm_buffer + ctx.vgm_data_offset;

	ctx.mlm_buffer = (char*)malloc(MLM_BUFFER_SIZE);
	if (ctx.mlm_buffer == NULL)
		return VGM2MLM_STERR_FAILED_MEM_ALLOCATION;

	memcpy(ctx.mlm_buffer, MLM_HEADER, sizeof(MLM_HEADER));
	char* mlm_event_list = ctx.mlm_buffer + sizeof(MLM_HEADER);

	bool was_loop_point_reached = false;
	ctx.mlm_head = mlm_event_list;
	ctx.mlm_loop_start = NULL;
	ctx.vrom_buffer = NULL;
	ctx.vgm_head = vgm_data;
	ctx.current_block = 0;

	// Leave some space to add the 
	// timer a counter load and
	// base time values later
	ctx.mlm_head += 3;
	
	VGMCOM_PRINTF("========================\n");
	for (; *ctx.vgm_head != 0x66;)
	{
		if (ctx.mlm_head - ctx.mlm_buffer >= MLM_BUFFER_SIZE - BIGGEST_COM_SIZE)
		{
			status = VGM2MLM_STERR_MLM_BUFFER_OVERFLOW;
			break;
		}

		if (ctx.vgm_loop_offset != 0 && ctx.vgm_loop_offset == (ctx.vgm_head - vgm_data))                                // if loop point was reached...
		{
			ctx.mlm_loop_offset = ctx.mlm_head - ctx.mlm_buffer;
			ctx.mlm_loop_block = ctx.current_block+BLOCK_OFFSET;
			was_loop_point_reached = true;
			DEBUG_PRINTF("loop start reached (vgm_loop_offset: 0x%04X; mlm_loop_offset: 0x%04X; mlm_loop_block: 0x%02X)\n",
				ctx.vgm_loop_offset, (uint16_t)ctx.mlm_loop_offset, (uint8_t)ctx.mlm_loop_block);
		}
		else if (ctx.vgm_loop_offset != 0 && (ctx.vgm_head - vgm_data) > ctx.vgm_loop_offset && !was_loop_point_reached) // if loop point was reached but is now behind the head...
		{
			DEBUG_PRINTF("WARNING! loop start passed (vgm_loop_offset: 0x%04X; current vgm offset: 0x%04X)\n",
				ctx.vgm_loop_offset, (uint16_t)(ctx.vgm_head - vgm_buffer));
			status = VGM2MLM_STERR_CORRUPTED_LOOP_OFS;
			break;
		}

		if (vgm2mlm_is_block_full(&ctx))
		{
			final_block_coms[ctx.current_block] = ctx.mlm_head;

			DEBUG_PRINTF("MLMCOM 34 (purpose: continue; address: 0x0000; block: 0x%02X)\n", ctx.current_block + 2 + BLOCK_OFFSET);

			ctx.mlm_head[0] = 0x22;                                 // Command 34
			ctx.mlm_head[1] = 0x00;                                 // Address LSB
			ctx.mlm_head[2] = 0x00;                                 // Address MSB
			ctx.mlm_head[3] = ctx.current_block + 2 + BLOCK_OFFSET; // Block

			ctx.current_block++;
			ctx.mlm_head = ctx.mlm_buffer + BLOCK_SIZE*ctx.current_block;
		}

		// if the Port A write buffer isn't empty AND if a command that 
		// isn't a YM2610 Port A register write/the buffer is full then...
		if ((*ctx.vgm_head != 0x58 || ctx.porta_reg_writes_idx >= REG_WRITES_BUFFER_LEN-1) && !ctx.is_porta_reg_writes_buffer_empty)
		{
			int write_count = ctx.porta_reg_writes_idx;

			VGMCOM_PRINTF("\n--------[PORT A MLM WRITE]--------\n");

			if (write_count == 1)
			{
				ctx.mlm_head[0] = 0x0D; // YM2610 Port A write command
				ctx.mlm_head[1] = ctx.porta_reg_writes_buffer[0] >> 8;   // address
				ctx.mlm_head[2] = ctx.porta_reg_writes_buffer[0] & 0xFF; // data

				VGMCOM_PRINTF("\t0\t0x%02X; 0x%02X\t0x%04X\n", (uint8_t)ctx.mlm_head[1], (uint8_t)ctx.mlm_head[2], ctx.porta_reg_writes_buffer[0]);
				ctx.mlm_head += 3;
			}
			else
			{
				*ctx.mlm_head = 0x30 | (write_count - 2);
				ctx.mlm_head++;

				for (int i = 0; i < write_count; i++)
				{
					ctx.mlm_head[0] = ctx.porta_reg_writes_buffer[i] >> 8;  // address
					ctx.mlm_head[1] = ctx.porta_reg_writes_buffer[i] & 0xFF;

					VGMCOM_PRINTF("\t%d\t0x%02X; 0x%02X\t0x%04X\n", i, (uint8_t)ctx.mlm_head[0], (uint8_t)ctx.mlm_head[1], ctx.porta_reg_writes_buffer[i]);
					ctx.mlm_head += 2;
				}
			}

			VGMCOM_PRINTF("----------------------------------\n\n");

			ctx.porta_reg_writes_idx = 0;
			ctx.is_porta_reg_writes_buffer_empty = true;
		}

		// if the Port B write buffer isn't empty AND if a command that 
		// isn't a YM2610 Port B register write/the buffer is full then...
		if ((*ctx.vgm_head != 0x59 || ctx.portb_reg_writes_idx >= REG_WRITES_BUFFER_LEN-1) && !ctx.is_portb_reg_writes_buffer_empty)
		{
			int write_count = ctx.portb_reg_writes_idx;

			VGMCOM_PRINTF("\n--------[PORT B MLM WRITE]--------\n");

			if (write_count == 1)
			{
				ctx.mlm_head[0] = 0x0E; // YM2610 Port B write command
				ctx.mlm_head[1] = ctx.portb_reg_writes_buffer[0] >> 8;   // address
				ctx.mlm_head[2] = ctx.portb_reg_writes_buffer[0] & 0xFF; // data

				VGMCOM_PRINTF("\t0\t0x%02X; 0x%02X\t0x%04X\n", (uint8_t)ctx.mlm_head[1], (uint8_t)ctx.mlm_head[2], ctx.portb_reg_writes_buffer[0]);
				ctx.mlm_head += 3;
			}
			else
			{
				*ctx.mlm_head = 0x40 | (write_count - 2);
				ctx.mlm_head++;

				for (int i = 0; i < write_count; i++)
				{
					ctx.mlm_head[0] = ctx.portb_reg_writes_buffer[i] >> 8;  // address
					ctx.mlm_head[1] = ctx.portb_reg_writes_buffer[i] & 0xFF;

					VGMCOM_PRINTF("\t%d\t0x%02X; 0x%02X\t0x%04X\n", i, (uint8_t)ctx.mlm_head[0], (uint8_t)ctx.mlm_head[1], ctx.portb_reg_writes_buffer[i]);
					ctx.mlm_head += 2;
				}
			}

			VGMCOM_PRINTF("----------------------------------\n\n");

			ctx.portb_reg_writes_idx = 0;
			ctx.is_portb_reg_writes_buffer_empty = true;
		}

		status = VGM_COMMANDS[*ctx.vgm_head](&ctx);
		if (status != VGM2MLM_STSUCCESS)
			break;
	}

	if (status != VGM2MLM_STSUCCESS)
		return status;

	if (ctx.vgm_loop_offset == 0)                     // if the song doesn't loop...
	{
		DEBUG_PRINTF("Loop: None\n");
		ctx.mlm_head[0] = 0x00; // End of event list
	}
	else if (ctx.current_block == ctx.mlm_loop_block) // if the song loops in a single block...
	{
		DEBUG_PRINTF("Loop: InZone\n");
		uint16_t block_ofs = ctx.mlm_loop_offset - (ctx.mlm_loop_block * BLOCK_SIZE);

		ctx.mlm_head[0] = 0x21; // Command 33
		ctx.mlm_head[1] = block_ofs & 0xFF;
		ctx.mlm_head[2] = block_ofs >> 8;
	}
	else                                              // if the song loops in more than one block...
	{
		DEBUG_PRINTF("Loop: Banked\n");
		uint16_t block_ofs = ctx.mlm_loop_offset - (ctx.mlm_loop_block * BLOCK_SIZE);
		char* precedent_com34 = final_block_coms[ctx.current_block-1];

		precedent_com34[1] = block_ofs & 0xFF;                   // Address LSB
		precedent_com34[2] = block_ofs >> 8;                     // Address MSB
		precedent_com34[3] = ctx.mlm_loop_block + BLOCK_OFFSET;  // Block

		DEBUG_PRINTF("MLMCOM 34 (purpose: loop; address: 0x0000; block: 0x%02X)\n", ctx.mlm_loop_block + 1 + BLOCK_OFFSET);

		ctx.mlm_head[0] = 0x22;                                  // Command 34
		ctx.mlm_head[1] = 0x00;                                  // Address LSB
		ctx.mlm_head[2] = 0x00;                                  // Address MSB
		ctx.mlm_head[3] = ctx.mlm_loop_block + 1 + BLOCK_OFFSET; // Block
	}

	uint16_t tma_load = (uint16_t)roundf(
		1024.0 - (1.0 / ctx.frequency / 72.0 * 4000000.0));
	DEBUG_PRINTF("tma_load: %d\n", tma_load);

	mlm_event_list[0] = tma_load & 0xFF; // Timer A counter load LSB
	mlm_event_list[1] = tma_load >> 8;   // Timer A counter load MSB
	mlm_event_list[2] = ctx.base_time;   // Base Time

	status = vgm2mlm_create_rom(&ctx, output);
	if (status != VGM2MLM_STSUCCESS)
		return status;

	free(ctx.mlm_buffer);
	return status;
}

void vgm2mlm_output_free(vgm2mlm_output_t* output)
{
	free(output->m1rom_buffer);
	free(output->vrom_buffer);
	free(output->prom_buffer);
}

char* vgm2mlm_file2buffer(const char* filepath, size_t* size)
{
	FILE* file = NULL;
	char* buffer = NULL;
	int no_size_given = size == NULL;

	if (no_size_given)
		size = (size_t*)malloc(sizeof(size_t));

	file = fopen(filepath, "rb");

	if (file == NULL)
		return NULL;

	fseek(file, 0, SEEK_END);
	*size = ftell(file);
	rewind(file);
	buffer = (char*)malloc(*size);
	
	if (fread(buffer, 1, *size, file) != *size)
	{
		free(buffer);
		buffer = NULL;
	}
	fclose(file);

	if (no_size_given)
		free(size);

	return buffer;
}

vgm2mlm_status_code_t vgm2mlm_write_buffer_to_file(const char* filename, const char* buffer, size_t buffer_size)
{
	FILE* out_file = NULL;
	out_file = fopen(filename, "wb"); 

	if (out_file == NULL)
		return VGM2MLM_STERR_FAILED_TO_WRITE_TO_FILE;

	if (fwrite(buffer, 1, buffer_size, out_file) != buffer_size)
	{
		fclose(out_file);
		return VGM2MLM_STERR_FAILED_TO_WRITE_TO_FILE;
	}

	fclose(out_file);

	return VGM2MLM_STSUCCESS;
}

// the directory itself has to be created before-hand
vgm2mlm_status_code_t vgm2mlm_df_intf(char* vgm_path, char* output_path)
{
	const char* FILENAMES[] = 
	{
		"/prom.bin",  "/vrom.bin",  "/m1rom.bin",
		"/c1rom.bin", "/c2rom.bin", "/srom.bin"
	};

	const char DUMMY_ROM[1] = { 0x00 };

	size_t vgm_buffer_size;
	char* vgm_buffer = vgm2mlm_file2buffer(vgm_path, &vgm_buffer_size);
	if (vgm_buffer == NULL)
		return VGM2MLM_STERR_FAILED_TO_READ_FILE;

	vgm2mlm_output_t output;
	vgm2mlm_status_code_t status =
		vgm2mlm(vgm_buffer, vgm_buffer_size, 0, &output, VGM2MLM_FLAG_NONE); 

	if (status != VGM2MLM_STSUCCESS)
		return status;

	int filepath_count = sizeof(FILENAMES) / sizeof(FILENAMES[0]);
	int output_dir_path_len = strlen(output_path);
	char* filepaths[filepath_count];
	
	const char* FILEBUFFERS[] = 
	{
		output.prom_buffer, output.vrom_buffer, output.m1rom_buffer,
		DUMMY_ROM,          DUMMY_ROM,          driver_s1,
	};

	const size_t FILESIZES[] =
	{
		PROM_SIZE,         output.vrom_size,  output.m1rom_size,
		sizeof(DUMMY_ROM), sizeof(DUMMY_ROM), sizeof(driver_s1)
	};

	for (int i = 0; i < filepath_count; i++)
	{	
		size_t filepath_size = output_dir_path_len + strlen(FILENAMES[i]) + 1;

		filepaths[i] =
			(char*)malloc(filepath_size);

		strncpy(filepaths[i], output_path, filepath_size);
		strncat(filepaths[i], FILENAMES[i], filepath_size);
		
		status =
			vgm2mlm_write_buffer_to_file(filepaths[i], FILEBUFFERS[i], FILESIZES[i]);
	
		free(filepaths[i]);
	}

	vgm2mlm_output_free(&output);
	free(vgm_buffer);
	
	return VGM2MLM_STSUCCESS;
}