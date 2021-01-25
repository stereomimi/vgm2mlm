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
	"ERROR: The VGM file is corrupted"
};

const uint16_t MLM_HEADER[14] = 
{
	0x0002, /* Song 0 offset */

	0x001C, /* Song 0, Channel 0  offset */
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

// the size should be dividible by two.
void vgm2mlm_swap_bytes(const char* src, char* dest, size_t size)
{
	for (int i = 0; i < size; i += 2)
	{
		dest[i]   = src[i+1];
		dest[i+1] = src[i];
	}
}

vgm2mlm_status_code_t vgm2mlm_parse_vgm_header(vgm2mlm_ctx_t* ctx, char* vgm_buffer, size_t vgm_size)
{
	if (vgm_size <= 0x4C)
		return VGM2MLM_STERR_BAD_VGM_FILE;

	if (strncmp(vgm_buffer, "Vgm ", 4) != 0)
		return VGM2MLM_STERR_INVALID_VGM_FILE;

	uint32_t ym2610_clock =
		vmg2mlm_le_32bit_read(vgm_buffer+0x4C);
	if (!ym2610_clock)
		return VGM2MLM_STERR_NO_YM2610;

	ctx->vgm_loop_offset =
		vmg2mlm_le_32bit_read(vgm_buffer+0x1C);
	if (ctx->vgm_loop_offset != 0)
		ctx->vgm_loop_offset += 0x1C;

	ctx->vgm_data_offset =
		vmg2mlm_le_32bit_read(vgm_buffer+0x34);
	ctx->vgm_data_offset += 0x34;

	DEBUG_PRINTF("vgm data ofs\t0x%08X\n",ctx->vgm_data_offset);
	if (ctx->frequency == 0)
	{
		ctx->frequency = 
			vmg2mlm_le_32bit_read(vgm_buffer+0x24);

		if (ctx->frequency == 0)
			ctx->frequency = 60;
	}

	uint32_t gd3_offset =
		vmg2mlm_le_32bit_read(vgm_buffer+0x14);

	if (gd3_offset != 0)
	{
		gd3_offset += 0x14;

		if (vgm_size < gd3_offset)
			return VGM2MLM_STERR_BAD_VGM_FILE;

		char* gd3_data = vgm_buffer+gd3_offset;
		DEBUG_PRINTF("gd3 offset\t0x%08X\n", gd3_offset);
		
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

vgm2mlm_status_code_t vgm2mlm(char* vgm_buffer, size_t vgm_size, int frequency, vgm2mlm_output_t* output)
{
	const uint8_t BANK_OFFSET = 2;
	const size_t ZONE3_SIZE = 0x4000;
	const size_t MLM_BUFFER_SIZE = ZONE3_SIZE*(16-BANK_OFFSET);
	const int PROM_TRACK_AUTHOR_OFS = 0x0BB2;
	const int PROM_TRACK_NAME_OFS   = 0x0BD0;

	vgm2mlm_ctx_t ctx;
	ctx.frequency = frequency;

	vgm2mlm_status_code_t status =
		vgm2mlm_parse_vgm_header(&ctx, vgm_buffer, vgm_size);
	if (status != VGM2MLM_STSUCCESS)
		return status;

	char* vgm_data = vgm_buffer + ctx.vgm_data_offset;

	char* mlm_buffer = (char*)malloc(MLM_BUFFER_SIZE);	
	memcpy(mlm_buffer, MLM_HEADER, sizeof(MLM_HEADER));
	char* mlm_event_list = mlm_buffer + sizeof(MLM_HEADER);
	
	uint16_t tma_load = (uint16_t)roundf(
		1024.0 - (1.0 / ctx.frequency / 72.0 * 4000000.0));
	
	ctx.mlm_head = mlm_event_list;
	ctx.mlm_loop_start = NULL;
	ctx.vrom_buffer = NULL;
	ctx.vgm_head = vgm_data;

	/*ctx.mlm_head[0] = 0x09; // Set timer b command
	ctx.mlm_head[1] = tmb_load;
	ctx.mlm_head[2] = 0;    // execute the next event immediately
	ctx.mlm_head += 3;*/

	ctx.mlm_head[0] = 0x0F; // Set timer a command
	ctx.mlm_head[1] = tma_load >> 2;
	ctx.mlm_head[2] = tma_load & 2;   
	ctx.mlm_head += 3;

	uint8_t current_bank = 0;

	//printf("%d\t%d\t0x%04X\n", current_bank+BANK_OFFSET, ctx.mlm_head - mlm_buffer, (uint16_t)(ZONE3_SIZE*(current_bank+BANK_OFFSET)));
	for (; *ctx.vgm_head != 0x66;)
	{
		if (ctx.mlm_head - mlm_buffer >= MLM_BUFFER_SIZE)
		{
			status = 
				VGM2MLM_STERR_MLM_BUFFER_OVERFLOW;
			break;
		}
		
		char* precedent_vgm_head = ctx.vgm_head;
		char* precedent_mlm_head = ctx.mlm_head;

		status =
			VGM_COMMANDS[*ctx.vgm_head](&ctx);
		if (status != VGM2MLM_STSUCCESS)
			break;

		if (ctx.mlm_head - mlm_buffer >= ZONE3_SIZE * (current_bank+1) - 2)
		{
			current_bank++;
			ctx.mlm_head = mlm_buffer + ZONE3_SIZE*current_bank;
			precedent_mlm_head[0] = 0x20;
			precedent_mlm_head[1] = current_bank+BANK_OFFSET;
			ctx.vgm_head = precedent_vgm_head;
		}
	}

	*ctx.mlm_head = 0x00; // end of event list command

	output->m1rom_size = (current_bank+BANK_OFFSET+1)*ZONE3_SIZE;
	DEBUG_PRINTF("m1rom size\t%d\n", output->m1rom_size);
	
	output->m1rom_buffer = 
		(char*)malloc(output->m1rom_size);
	memcpy(output->m1rom_buffer, driver_m1, driver_m1_size);
	memcpy(output->m1rom_buffer+(ZONE3_SIZE*BANK_OFFSET), mlm_buffer, (current_bank+1)*ZONE3_SIZE);
	DEBUG_PRINTF("m1rom mlm ofs\t0x%05X\n", ZONE3_SIZE*BANK_OFFSET);
	DEBUG_PRINTF("m1rom mlm size\t0x%05X\n", (current_bank+1)*ZONE3_SIZE);

	output->vrom_buffer = ctx.vrom_buffer;
	output->vrom_size = ctx.vrom_buffer_size;

	char* unswapped_prom =
		(char*)malloc(PROM_SIZE);
	memset(unswapped_prom, 0xFF, PROM_SIZE);
	memcpy(unswapped_prom, driver_p1, driver_p1_size);
	memcpy(unswapped_prom+PROM_TRACK_AUTHOR_OFS, ctx.track_author, sizeof(ctx.track_author));
	memcpy(unswapped_prom+PROM_TRACK_NAME_OFS, ctx.track_name, sizeof(ctx.track_name));

	output->prom_buffer =
		(char*)malloc(PROM_SIZE);
	vgm2mlm_swap_bytes(unswapped_prom, output->prom_buffer, PROM_SIZE);
	
	free(mlm_buffer);
	free(unswapped_prom);

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
		vgm2mlm(vgm_buffer, vgm_buffer_size, 0, &output);

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

		strcpy(filepaths[i], output_path);
		strcat(filepaths[i], FILENAMES[i]);
		
		status =
			vgm2mlm_write_buffer_to_file(filepaths[i], FILEBUFFERS[i], FILESIZES[i]);
	
		free(filepaths[i]);
	}

	vgm2mlm_output_free(&output);
	free(vgm_buffer);
	
	return VGM2MLM_STSUCCESS;
}