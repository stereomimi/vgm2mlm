#ifndef VGM2MLM_CTX_H
#define VGM2MLM_CTX_H

#define YM2610B_CLOCK_BITMASK 0x80000000
#define VROM_BUFFER_SIZE 16777216
#define M1_BUFFER_SIZE 262144
#define UINT4_MAX 16
#define TRACK_INFO_LINE_LENGTH 30 // 29 characters + 1 NULL character
#define TRACK_NOTES_LINES 10
#define PROM_SIZE 524288
#define DEFLEMASK 1

//#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#define DEBUG_PRINTF(...)

typedef unsigned int uint;

typedef struct {
	char* vgm_head;
	char* mlm_head;
	char* vrom_buffer;
	char* mlm_loop_start;
	size_t vrom_buffer_size;
	int frequency;
	uint32_t vgm_data_offset;
	uint32_t vgm_loop_offset;
	char track_name[TRACK_INFO_LINE_LENGTH];
	char track_author[TRACK_INFO_LINE_LENGTH];
} vgm2mlm_ctx_t;

typedef struct {
	char* m1rom_buffer;
	char* vrom_buffer;
	char* prom_buffer;
	size_t m1rom_size;
	size_t vrom_size;
} vgm2mlm_output_t;

typedef enum {
	VGM2MLM_STSUCCESS,
	VGM2MLM_STERR_NO_YM2610,
	VGM2MLM_STERR_MLM_BUFFER_OVERFLOW,
	VGM2MLM_STERR_INVALID_VGM_COMMAND,
	VGM2MLM_STERR_FAILED_REALLOCATION,
	VGM2MLM_STERR_UNSUPPORTED_VGM_DATA_BLOCK_TYPE,
	VGM2MLM_STERR_PCM_WRITE,
	VGM2MLM_STERR_DACSTRM_WRITE,
	VGM2MLM_STERR_INVALID_VGM_FILE,
	VGM2MLM_STERR_INVALID_GD3_TAG,
	VGM2MLM_STERR_FAILED_TO_READ_FILE,
	VGM2MLM_STERR_FAILED_TO_WRITE_TO_FILE,
	VGM2MLM_STERR_BAD_VGM_FILE,
	VGM2MLM_STATUS_COUNT
} vgm2mlm_status_code_t;

#endif