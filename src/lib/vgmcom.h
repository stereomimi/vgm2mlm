#ifndef VGMCOM_H
#define VGMCOM_H

#include "def.h"

extern vgm2mlm_status_code_t (*VGM_COMMANDS[256])(vgm2mlm_ctx_t*);

vgm2mlm_status_code_t VGMCOM_invalid(vgm2mlm_ctx_t* ctx);
vgm2mlm_status_code_t VGMCOM_skip_argc0(vgm2mlm_ctx_t* ctx);
vgm2mlm_status_code_t VGMCOM_skip_argc1(vgm2mlm_ctx_t* ctx);
vgm2mlm_status_code_t VGMCOM_skip_argc2(vgm2mlm_ctx_t* ctx);
vgm2mlm_status_code_t VGMCOM_skip_argc3(vgm2mlm_ctx_t* ctx);
vgm2mlm_status_code_t VGMCOM_skip_argc4(vgm2mlm_ctx_t* ctx);
vgm2mlm_status_code_t VGMCOM_ym2610_write_a(vgm2mlm_ctx_t* ctx);
vgm2mlm_status_code_t VGMCOM_ym2610_write_b(vgm2mlm_ctx_t* ctx);
vgm2mlm_status_code_t VGMCOM_wait_nnnn_samples(vgm2mlm_ctx_t* ctx);
vgm2mlm_status_code_t VGMCOM_wait_735_samples(vgm2mlm_ctx_t* ctx);
vgm2mlm_status_code_t VGMCOM_wait_882_samples(vgm2mlm_ctx_t* ctx);
vgm2mlm_status_code_t VGMCOM_data_block(vgm2mlm_ctx_t* ctx);
vgm2mlm_status_code_t VGMCOM_pcm_ram_write(vgm2mlm_ctx_t* ctx);
vgm2mlm_status_code_t VGMCOM_wait_n_samples(vgm2mlm_ctx_t* ctx);
vgm2mlm_status_code_t VGMCOM_dacstrm_cnt_write(vgm2mlm_ctx_t* ctx);

#endif