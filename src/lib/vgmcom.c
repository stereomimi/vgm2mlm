#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "def.h"
#include "utils.h"

vgm2mlm_status_code_t VGMCOM_invalid(vgm2mlm_ctx_t* ctx)
{
	VGMCOM_PRINTF("VGMCOM\tinvalid\n");

	return VGM2MLM_STERR_INVALID_VGM_COMMAND;
}

vgm2mlm_status_code_t VGMCOM_skip_argc0(vgm2mlm_ctx_t* ctx)
{	
	VGMCOM_PRINTF("VGMCOM\tskip_argc0\n");
	(*ctx->vgm_head)++; 
	return VGM2MLM_STSUCCESS;
}
vgm2mlm_status_code_t VGMCOM_skip_argc1(vgm2mlm_ctx_t* ctx)
{	
	VGMCOM_PRINTF("VGMCOM\tskip_argc1\n");
	*ctx->vgm_head += 2; 
	return VGM2MLM_STSUCCESS;
}
vgm2mlm_status_code_t VGMCOM_skip_argc2(vgm2mlm_ctx_t* ctx)
{	
	VGMCOM_PRINTF("VGMCOM\tskip_argc2\n");
	*ctx->vgm_head += 3; 
	return VGM2MLM_STSUCCESS;
}
vgm2mlm_status_code_t VGMCOM_skip_argc3(vgm2mlm_ctx_t* ctx)
{	
	VGMCOM_PRINTF("VGMCOM\tskip_argc3\n");
	*ctx->vgm_head += 4; 
	return VGM2MLM_STSUCCESS;
}
vgm2mlm_status_code_t VGMCOM_skip_argc4(vgm2mlm_ctx_t* ctx)
{	
	VGMCOM_PRINTF("VGMCOM\tskip_argc4\n");
	*ctx->vgm_head += 5; 
	return VGM2MLM_STSUCCESS;
}

vgm2mlm_status_code_t VGMCOM_ym2610_write_a(vgm2mlm_ctx_t* ctx)
{
	uint8_t address = ctx->vgm_head[1];
	uint8_t data = ctx->vgm_head[2];

	ctx->vgm_head += 3;

	if (address != 0xFF) return VGM2MLM_STSUCCESS;

	if (ctx->porta_reg_writes_idx < REG_WRITES_BUFFER_LEN-1) // if buffer isn't full...
	{
		ctx->porta_reg_writes_buffer[ctx->porta_reg_writes_idx] = (address<<8) | data;
		ctx->porta_reg_writes_idx++;
		ctx->is_porta_reg_writes_buffer_empty = false;
	}

	VGMCOM_PRINTF("VGMCOM\tym2610_write_a (addr: 0x%02X; data: 0x%02X)\n", (uint8_t)address, (uint8_t)data);

	return VGM2MLM_STSUCCESS;
}

vgm2mlm_status_code_t VGMCOM_ym2610_write_b(vgm2mlm_ctx_t* ctx)
{
	uint8_t address = ctx->vgm_head[1];
	uint8_t data = ctx->vgm_head[2];
	
	ctx->vgm_head += 3;

	if (address >= 0x31) return VGM2MLM_STSUCCESS;

	if (ctx->portb_reg_writes_idx < REG_WRITES_BUFFER_LEN-1) // if buffer isn't full...
	{
		ctx->portb_reg_writes_buffer[ctx->portb_reg_writes_idx] = (address<<8) | data;
		ctx->portb_reg_writes_idx++;
		ctx->is_portb_reg_writes_buffer_empty = false;
	}

	VGMCOM_PRINTF("VGMCOM\tym2610_write_b (addr: 0x%02X; data: 0x%02X)\n", (uint8_t)address, (uint8_t)data);
	
	return VGM2MLM_STSUCCESS;
}

vgm2mlm_status_code_t VGMCOM_wait_nnnn_samples(vgm2mlm_ctx_t* ctx)
{
	uint16_t samples = *(uint16_t*)(ctx->vgm_head+1);

	int ticks = (uint)roundf(
		samples * ctx->frequency / 44100.0f);

	VGMCOM_PRINTF("VGMCOM\twait_nnnn_samples (samples: %d; ticks: %f (%d))\n", samples, samples * ctx->frequency / 44100.0, ticks);

	vgm2mlm_append_mlm_wait_com(ctx, ticks);

	ctx->vgm_head += 3;
	return VGM2MLM_STSUCCESS;
}

vgm2mlm_status_code_t VGMCOM_wait_735_samples(vgm2mlm_ctx_t* ctx)
{
	VGMCOM_PRINTF("VGMCOM\twait_735_samples\n");

	uint ticks = (uint)roundf(
		735 * ctx->frequency / 44100);

	vgm2mlm_append_mlm_wait_com(ctx, ticks);

	ctx->vgm_head += 1;
	return VGM2MLM_STSUCCESS;
}

vgm2mlm_status_code_t VGMCOM_wait_882_samples(vgm2mlm_ctx_t* ctx)
{
	VGMCOM_PRINTF("VGMCOM\twait_882_samples\n");

	uint ticks = (uint)roundf(
		882 * ctx->frequency / 44100);

	vgm2mlm_append_mlm_wait_com(ctx, ticks);

	ctx->vgm_head += 1;
	return VGM2MLM_STSUCCESS;
}

vgm2mlm_status_code_t VGMCOM_data_block(vgm2mlm_ctx_t* ctx)
{
	uint8_t  block_type  = ctx->vgm_head[2];
	uint32_t block_size  = 
		vmg2mlm_le_32bit_read(ctx->vgm_head+3);
	char*    block_data = ctx->vgm_head + 7;
	//printf("t: 0x%02X, s: %d bytes\n", (uint8_t)block_type, block_size);
	
	VGMCOM_PRINTF("VGMCOM\tdata_block (type: 0x%02X; size: 0x%06X; data: %p)\n", (uint8_t)block_type, block_size, block_data);

	switch(block_type)
	{
		case 0x82:   // YM2610 ADPCM ROM data
		case 0x83: ; // YM2610 DELTA-T ROM data
			{
				uint32_t rom_size = vmg2mlm_le_32bit_read(block_data);
				uint32_t start_addr_offset = vmg2mlm_le_32bit_read(block_data+4);
				char* adpcma_rom = block_data+8;
				size_t adpcma_rom_size = block_size - 8;

				ctx->vrom_buffer = (char*)realloc(ctx->vrom_buffer, rom_size);
				if (ctx->vrom_buffer == NULL)
					return VGM2MLM_STERR_FAILED_REALLOCATION;

				memcpy(ctx->vrom_buffer+start_addr_offset, adpcma_rom, adpcma_rom_size);
				ctx->vrom_buffer_size = rom_size;
				DEBUG_PRINTF("data_block\trom size: %d bytes, start addr ofs: 0x%08X\n", rom_size, start_addr_offset);
			}
			break;

		default:
			//fatal_printf("ERROR: data block type 0x%02X isn't supported.", (uint8_t)block_type);
			return VGM2MLM_STERR_UNSUPPORTED_VGM_DATA_BLOCK_TYPE;
			break;
	}

	ctx->vgm_head += 7 + block_size;

	return VGM2MLM_STSUCCESS;
}

vgm2mlm_status_code_t VGMCOM_pcm_ram_write(vgm2mlm_ctx_t* ctx)
{
	VGMCOM_PRINTF("VGMCOM\tpcm_ram_write\n");
	return VGM2MLM_STERR_PCM_WRITE;
}

vgm2mlm_status_code_t VGMCOM_wait_n_samples(vgm2mlm_ctx_t* ctx)
{
	uint samples = ctx->vgm_head[0];
	uint ticks = (uint)roundf(
		samples * ctx->frequency / 44100);

	VGMCOM_PRINTF("VGMCOM\twait_n_samples (samples: %d)\n", samples);

	ctx->mlm_head[0] = 0x10 | ticks;
	ctx->vgm_head += 1;
	ctx->mlm_head += 1;
	return VGM2MLM_STSUCCESS;
}

vgm2mlm_status_code_t VGMCOM_dacstrm_cnt_write(vgm2mlm_ctx_t* ctx)
{
	VGMCOM_PRINTF("VGMCOM\tdatstrm_cnt_write\n");
	return VGM2MLM_STERR_DACSTRM_WRITE;
}

vgm2mlm_status_code_t (*VGM_COMMANDS[256])(vgm2mlm_ctx_t*) =
{
	VGMCOM_invalid,           // 0x00  invalid
	VGMCOM_invalid,           // 0x01  invalid
	VGMCOM_invalid,           // 0x02  invalid
	VGMCOM_invalid,           // 0x03  invalid
	VGMCOM_invalid,           // 0x04  invalid
	VGMCOM_invalid,           // 0x05  invalid
	VGMCOM_invalid,           // 0x06  invalid
	VGMCOM_invalid,           // 0x07  invalid
	VGMCOM_invalid,           // 0x08  invalid
	VGMCOM_invalid,           // 0x09  invalid
	VGMCOM_invalid,           // 0x0A  invalid
	VGMCOM_invalid,           // 0x0B  invalid
	VGMCOM_invalid,           // 0x0C  invalid
	VGMCOM_invalid,           // 0x0D  invalid
	VGMCOM_invalid,           // 0x0E  invalid
	VGMCOM_invalid,           // 0x0F  invalid
	VGMCOM_invalid,           // 0x10  invalid
	VGMCOM_invalid,           // 0x11  invalid
	VGMCOM_invalid,           // 0x12  invalid
	VGMCOM_invalid,           // 0x13  invalid
	VGMCOM_invalid,           // 0x14  invalid
	VGMCOM_invalid,           // 0x15  invalid
	VGMCOM_invalid,           // 0x16  invalid
	VGMCOM_invalid,           // 0x17  invalid
	VGMCOM_invalid,           // 0x18  invalid
	VGMCOM_invalid,           // 0x19  invalid
	VGMCOM_invalid,           // 0x1A  invalid
	VGMCOM_invalid,           // 0x1B  invalid
	VGMCOM_invalid,           // 0x1C  invalid
	VGMCOM_invalid,           // 0x1D  invalid
	VGMCOM_invalid,           // 0x1E  invalid
	VGMCOM_invalid,           // 0x1F  invalid
	VGMCOM_invalid,           // 0x20  invalid
	VGMCOM_invalid,           // 0x21  invalid
	VGMCOM_invalid,           // 0x22  invalid
	VGMCOM_invalid,           // 0x23  invalid
	VGMCOM_invalid,           // 0x24  invalid
	VGMCOM_invalid,           // 0x25  invalid
	VGMCOM_invalid,           // 0x26  invalid
	VGMCOM_invalid,           // 0x27  invalid
	VGMCOM_invalid,           // 0x28  invalid
	VGMCOM_invalid,           // 0x29  invalid
	VGMCOM_invalid,           // 0x2A  invalid
	VGMCOM_invalid,           // 0x2B  invalid
	VGMCOM_invalid,           // 0x2C  invalid
	VGMCOM_invalid,           // 0x2D  invalid
	VGMCOM_invalid,           // 0x2E  invalid
	VGMCOM_invalid,           // 0x2F  invalid
	VGMCOM_skip_argc1,        // 0x30  one operand, reserved for future use
	VGMCOM_skip_argc1,        // 0x31  one operand, reserved for future use
	VGMCOM_skip_argc1,        // 0x32  one operand, reserved for future use
	VGMCOM_skip_argc1,        // 0x33  one operand, reserved for future use
	VGMCOM_skip_argc1,        // 0x34  one operand, reserved for future use
	VGMCOM_skip_argc1,        // 0x35  one operand, reserved for future use
	VGMCOM_skip_argc1,        // 0x36  one operand, reserved for future use
	VGMCOM_skip_argc1,        // 0x37  one operand, reserved for future use
	VGMCOM_skip_argc1,        // 0x38  one operand, reserved for future use
	VGMCOM_skip_argc1,        // 0x39  one operand, reserved for future use
	VGMCOM_skip_argc1,        // 0x3A  one operand, reserved for future use
	VGMCOM_skip_argc1,        // 0x3B  one operand, reserved for future use
	VGMCOM_skip_argc1,        // 0x3C  one operand, reserved for future use
	VGMCOM_skip_argc1,        // 0x3D  one operand, reserved for future use
	VGMCOM_skip_argc1,        // 0x3E  one operand, reserved for future use
	VGMCOM_skip_argc1,        // 0x3F  one operand, reserved for future use
	VGMCOM_skip_argc2,        // 0x40  two operands, reserved for future use
	VGMCOM_skip_argc2,        // 0x41  two operands, reserved for future use
	VGMCOM_skip_argc2,        // 0x42  two operands, reserved for future use
	VGMCOM_skip_argc2,        // 0x43  two operands, reserved for future use
	VGMCOM_skip_argc2,        // 0x44  two operands, reserved for future use
	VGMCOM_skip_argc2,        // 0x45  two operands, reserved for future use
	VGMCOM_skip_argc2,        // 0x46  two operands, reserved for future use
	VGMCOM_skip_argc2,        // 0x47  two operands, reserved for future use
	VGMCOM_skip_argc2,        // 0x48  two operands, reserved for future use
	VGMCOM_skip_argc2,        // 0x49  two operands, reserved for future use
	VGMCOM_skip_argc2,        // 0x4A  two operands, reserved for future use
	VGMCOM_skip_argc2,        // 0x4B  two operands, reserved for future use
	VGMCOM_skip_argc2,        // 0x4C  two operands, reserved for future use
	VGMCOM_skip_argc2,        // 0x4D  two operands, reserved for future use
	VGMCOM_skip_argc2,        // 0x4E  two operands, reserved for future use
	VGMCOM_skip_argc1,        // 0x4F  Game Gear PSG stereo, write dd to port 0x06
	VGMCOM_skip_argc1,        // 0x50  PSG (SN76489/SN76496) write value dd
	VGMCOM_skip_argc2,        // 0x51  YM2413, write value dd to register aa
	VGMCOM_skip_argc2,        // 0x52  YM2612 port 0, write value dd to register aa
	VGMCOM_skip_argc2,        // 0x53  YM2612 port 1, write value dd to register aa
	VGMCOM_skip_argc2,        // 0x54  YM2151, write value dd to register aa
	VGMCOM_skip_argc2,        // 0x55  YM2203, write value dd to register aa
	VGMCOM_skip_argc2,        // 0x56  YM2608 port 0, write value dd to register aa
	VGMCOM_skip_argc2,        // 0x57  YM2608 port 1, write value dd to register aa
	VGMCOM_ym2610_write_a,    // 0x58  YM2610 port 0, write value dd to register aa
	VGMCOM_ym2610_write_b,    // 0x59  YM2610 port 1, write value dd to register aa
	VGMCOM_skip_argc2,        // 0x5A  YM3812, write value dd to register aa
	VGMCOM_skip_argc2,        // 0x5B  YM3526, write value dd to register aa
	VGMCOM_skip_argc2,        // 0x5C  Y8950, write value dd to register aa
	VGMCOM_skip_argc2,        // 0x5D  YMZ280B, write value dd to register aa
	VGMCOM_skip_argc2,        // 0x5E  YMF262 port 0, write value dd to register aa
	VGMCOM_skip_argc2,        // 0x5F  YMF262 port 1, write value dd to register aa
	VGMCOM_invalid,           // 0x60  invalid
	VGMCOM_wait_nnnn_samples, // 0x61  Wait nnnn samples
	VGMCOM_wait_735_samples,  // 0x62  wait 735 samples (60th of a second)
	VGMCOM_wait_882_samples,  // 0x63  wait 882 samples (50th of a second)
	VGMCOM_invalid,           // 0x64  invalid
	VGMCOM_invalid,           // 0x65  invalid
	VGMCOM_invalid,           // 0x66  end of sound data (shouldn't be reached)
	VGMCOM_data_block,        // 0x67  data block
	VGMCOM_pcm_ram_write,     // 0x68  PCM RAM write
	VGMCOM_invalid,           // 0x69  invalid
	VGMCOM_invalid,           // 0x6A  invalid
	VGMCOM_invalid,           // 0x6B  invalid
	VGMCOM_invalid,           // 0x6C  invalid
	VGMCOM_invalid,           // 0x6D  invalid
	VGMCOM_invalid,           // 0x6E  invalid
	VGMCOM_invalid,           // 0x6F  invalid
	VGMCOM_wait_n_samples,    // 0x70  wait n+1 samples
	VGMCOM_wait_n_samples,    // 0x71  wait n+1 samples
	VGMCOM_wait_n_samples,    // 0x72  wait n+1 samples
	VGMCOM_wait_n_samples,    // 0x73  wait n+1 samples
	VGMCOM_wait_n_samples,    // 0x74  wait n+1 samples
	VGMCOM_wait_n_samples,    // 0x75  wait n+1 samples
	VGMCOM_wait_n_samples,    // 0x76  wait n+1 samples
	VGMCOM_wait_n_samples,    // 0x77  wait n+1 samples
	VGMCOM_wait_n_samples,    // 0x78  wait n+1 samples
	VGMCOM_wait_n_samples,    // 0x79  wait n+1 samples
	VGMCOM_wait_n_samples,    // 0x7A  wait n+1 samples
	VGMCOM_wait_n_samples,    // 0x7B  wait n+1 samples
	VGMCOM_wait_n_samples,    // 0x7C  wait n+1 samples
	VGMCOM_wait_n_samples,    // 0x7D  wait n+1 samples
	VGMCOM_wait_n_samples,    // 0x7E  wait n+1 samples
	VGMCOM_wait_n_samples,    // 0x7F  wait n+1 samples
	VGMCOM_skip_argc0,        // 0x80  YM2612 port 0 address 2A write from the data bank, then wait n samples
	VGMCOM_skip_argc0,        // 0x81  YM2612 port 0 address 2A write from the data bank, then wait n samples
	VGMCOM_skip_argc0,        // 0x82  YM2612 port 0 address 2A write from the data bank, then wait n samples
	VGMCOM_skip_argc0,        // 0x83  YM2612 port 0 address 2A write from the data bank, then wait n samples
	VGMCOM_skip_argc0,        // 0x84  YM2612 port 0 address 2A write from the data bank, then wait n samples
	VGMCOM_skip_argc0,        // 0x85  YM2612 port 0 address 2A write from the data bank, then wait n samples
	VGMCOM_skip_argc0,        // 0x86  YM2612 port 0 address 2A write from the data bank, then wait n samples
	VGMCOM_skip_argc0,        // 0x87  YM2612 port 0 address 2A write from the data bank, then wait n samples
	VGMCOM_skip_argc0,        // 0x88  YM2612 port 0 address 2A write from the data bank, then wait n samples
	VGMCOM_skip_argc0,        // 0x89  YM2612 port 0 address 2A write from the data bank, then wait n samples
	VGMCOM_skip_argc0,        // 0x8A  YM2612 port 0 address 2A write from the data bank, then wait n samples
	VGMCOM_skip_argc0,        // 0x8B  YM2612 port 0 address 2A write from the data bank, then wait n samples
	VGMCOM_skip_argc0,        // 0x8C  YM2612 port 0 address 2A write from the data bank, then wait n samples
	VGMCOM_skip_argc0,        // 0x8D  YM2612 port 0 address 2A write from the data bank, then wait n samples
	VGMCOM_skip_argc0,        // 0x8E  YM2612 port 0 address 2A write from the data bank, then wait n samples
	VGMCOM_skip_argc0,        // 0x8F  YM2612 port 0 address 2A write from the data bank, then wait n samples
	VGMCOM_dacstrm_cnt_write, // 0x90  DAC Stream Control Write
	VGMCOM_dacstrm_cnt_write, // 0x91  DAC Stream Control Write
	VGMCOM_dacstrm_cnt_write, // 0x92  DAC Stream Control Write
	VGMCOM_dacstrm_cnt_write, // 0x93  DAC Stream Control Write
	VGMCOM_dacstrm_cnt_write, // 0x94  DAC Stream Control Write
	VGMCOM_dacstrm_cnt_write, // 0x95  DAC Stream Control Write
	VGMCOM_invalid,           // 0x96  invalid 
	VGMCOM_invalid,           // 0x97  invalid
	VGMCOM_invalid,           // 0x98  invalid
	VGMCOM_invalid,           // 0x99  invalid
	VGMCOM_invalid,           // 0x9A  invalid
	VGMCOM_invalid,           // 0x9B  invalid
	VGMCOM_invalid,           // 0x9C  invalid
	VGMCOM_invalid,           // 0x9D  invalid
	VGMCOM_invalid,           // 0x9E  invalid
	VGMCOM_invalid,           // 0x9F  invalid
	VGMCOM_skip_argc2,        // 0xA0  AY8910, write value dd to register aa 0xA0
	VGMCOM_skip_argc2,        // 0xA1  two operands, reserved for future use 
	VGMCOM_skip_argc2,        // 0xA2  two operands, reserved for future use 
	VGMCOM_skip_argc2,        // 0xA3  two operands, reserved for future use 
	VGMCOM_skip_argc2,        // 0xA4  two operands, reserved for future use 
	VGMCOM_skip_argc2,        // 0xA5  two operands, reserved for future use 
	VGMCOM_skip_argc2,        // 0xA6  two operands, reserved for future use 
	VGMCOM_skip_argc2,        // 0xA7  two operands, reserved for future use 
	VGMCOM_skip_argc2,        // 0xA8  two operands, reserved for future use 
	VGMCOM_skip_argc2,        // 0xA9  two operands, reserved for future use 
	VGMCOM_skip_argc2,        // 0xAA  two operands, reserved for future use 
	VGMCOM_skip_argc2,        // 0xAB  two operands, reserved for future use 
	VGMCOM_skip_argc2,        // 0xAC  two operands, reserved for future use 
	VGMCOM_skip_argc2,        // 0xAD  two operands, reserved for future use 
	VGMCOM_skip_argc2,        // 0xAE  two operands, reserved for future use 
	VGMCOM_skip_argc2,        // 0xAF  two operands, reserved for future use 
	VGMCOM_skip_argc2,        // 0xB0  RF5C68, write value dd to register aa
	VGMCOM_skip_argc2,        // 0xB1  RF5C164, write value dd to register aa
	VGMCOM_skip_argc2,        // 0xB2  PWM, write value ddd to register a (d is MSB, dd is LSB) 
	VGMCOM_skip_argc2,        // 0xB3  GameBoy DMG, write value dd to register aa
	VGMCOM_skip_argc2,        // 0xB4  NES APU, write value dd to register aa
	VGMCOM_skip_argc2,        // 0xB5  MultiPCM, write value dd to register aa
	VGMCOM_skip_argc2,        // 0xB6  uPD7759, write value dd to register aa
	VGMCOM_skip_argc2,        // 0xB7  OKIM6258, write value dd to register aa
	VGMCOM_skip_argc2,        // 0xB8  OKIM6295, write value dd to register aa 
	VGMCOM_skip_argc2,        // 0xB9  HuC6280, write value dd to register aa
	VGMCOM_skip_argc2,        // 0xBA  K053260, write value dd to register aa
	VGMCOM_skip_argc2,        // 0xBB  Pokey, write value dd to register aa
	VGMCOM_skip_argc2,        // 0xBC  WonderSwan, write value dd to register aa
	VGMCOM_skip_argc2,        // 0xBD  SAA1099, write value dd to register aa
	VGMCOM_skip_argc2,        // 0xBE  ES5506, write value dd to register aa
	VGMCOM_skip_argc2,        // 0xBF  GA20, write value dd to register aa
	VGMCOM_skip_argc3,        // 0xC0  Sega PCM, write value dd to memory offset aabb
	VGMCOM_skip_argc3,        // 0xC1  RF5C68, write value dd to memory offset aabb
	VGMCOM_skip_argc3,        // 0xC2  RF5C164, write value dd to memory offset aabb
	VGMCOM_skip_argc3,        // 0xC3  MultiPCM, write set bank offset aabb to channel cc
	VGMCOM_skip_argc3,        // 0xC4  QSound, write value mmll to register rr
	VGMCOM_skip_argc3,        // 0xC5  SCSP, write value dd to memory offset mmll
	VGMCOM_skip_argc3,        // 0xC6  WonderSwan, write value dd to memory offset
	VGMCOM_skip_argc3,        // 0xC7  VSU, write value dd to memory offset mmll
	VGMCOM_skip_argc3,        // 0xC8  X1-010, write value dd to memory offset mmll
	VGMCOM_skip_argc3,        // 0xC9  three operands, reserved for future use 
	VGMCOM_skip_argc3,        // 0xCA  three operands, reserved for future use 
	VGMCOM_skip_argc3,        // 0xCB  three operands, reserved for future use 
	VGMCOM_skip_argc3,        // 0xCC  three operands, reserved for future use 
	VGMCOM_skip_argc3,        // 0xCD  three operands, reserved for future use 
	VGMCOM_skip_argc3,        // 0xCE  three operands, reserved for future use 
	VGMCOM_skip_argc3,        // 0xCF  three operands, reserved for future use 
	VGMCOM_skip_argc3,        // 0xD0  YMF278B, port pp, write value dd to register aa
	VGMCOM_skip_argc3,        // 0xD1  YMF271, port pp, write value dd to register aa
	VGMCOM_skip_argc3,        // 0xD2  SCC1, port pp, write value dd to register aa
	VGMCOM_skip_argc3,        // 0xD3  K054539, write value dd to register ppaa
	VGMCOM_skip_argc3,        // 0xD4  C140, write value dd to register ppaa
	VGMCOM_skip_argc3,        // 0xD5  ES5503, write value dd to register ppaa
	VGMCOM_skip_argc3,        // 0xD6  ES5506, write value aadd to register pp
	VGMCOM_skip_argc3,        // 0xD7  three operands, reserved for future use
	VGMCOM_skip_argc3,        // 0xD8  three operands, reserved for future use
	VGMCOM_skip_argc3,        // 0xD9  three operands, reserved for future use
	VGMCOM_skip_argc3,        // 0xDA  three operands, reserved for future use
	VGMCOM_skip_argc3,        // 0xDB  three operands, reserved for future use
	VGMCOM_skip_argc3,        // 0xDC  three operands, reserved for future use
	VGMCOM_skip_argc3,        // 0xDD  three operands, reserved for future use
	VGMCOM_skip_argc3,        // 0xDE  three operands, reserved for future use
	VGMCOM_skip_argc3,        // 0xDF  three operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xE0  Seek to offset dddddddd (Intel byte order) in PCM
	VGMCOM_skip_argc4,        // 0xE1  C352, write value aadd to register mmll
	VGMCOM_skip_argc4,        // 0xE2  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xE3  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xE4  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xE5  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xE6  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xE7  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xE8  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xE9  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xEA  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xEB  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xEC  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xED  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xEE  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xEF  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xF0  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xF1  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xF2  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xF3  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xF4  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xF5  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xF6  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xF7  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xF8  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xF9  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xFA  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xFB  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xFC  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xFD  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xFE  four operands, reserved for future use
	VGMCOM_skip_argc4,        // 0xFF  four operands, reserved for future use       
};