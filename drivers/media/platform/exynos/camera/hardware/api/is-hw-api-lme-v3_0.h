/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * lme HW control APIs
 *
 * Copyright (C) 2022 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IS_HW_API_LME_V3_0_H
#define IS_HW_API_LME_V3_0_H

#include "is-hw-lme-v3.h"
#include "is-hw-common-dma.h"

#define COREX_IGNORE			(0)
#define COREX_COPY			(1)
#define COREX_SWAP			(2)

#define HW_TRIGGER			(0)
#define SW_TRIGGER			(1)

#define QUEUE_MODE			(0)
#define SEQUENCE_MODE			(1)

#define HBLANK_CYCLE			(0x2D)

#define DMA_CLIENT_LME_MEMORY_MIT_WIDTH	32
#define DMA_CLIENT_LME_BYTE_ALIGNMENT	16 /* 16B(128b) align for perf */

#define UTL_ALIGN_UP(a, b)		(DIV_ROUND_UP(a, b) * (b))

#define LSB_4BITS_MASK			0x0000000FUL
#define MSB_32BITS_MASK			0xFFFFFFFF0UL

enum set_status {
	SET_SUCCESS,
	SET_ERROR
};

enum lme_event_type {
	INTR_FRAME_START,
	INTR_FRAME_END,
	INTR_COREX_END_0,
	INTR_COREX_END_1,
	INTR_ERR
};

enum lme_sps_out_mode {
	LME_OUTPUT_MODE_8_4 = 0,
	LME_OUTPUT_MODE_2_2
};

int lme_hw_s_reset(void __iomem *base);
void lme_hw_s_clock(void __iomem *base, bool on);
unsigned int lme_hw_is_occurred(unsigned int status, enum lme_event_type type);
int lme_hw_wait_idle(void __iomem *base, u32 set_id);
void lme_hw_dump(void __iomem *base,  enum corex_set set_id);
void lme_hw_s_core(void __iomem *base, u32 set_id);
int lme_hw_s_rdma_init(void __iomem *base, struct lme_param_set *param_set, u32 enable, u32 id, u32 set_id);
int lme_hw_s_rdma_addr(void __iomem *base, dma_addr_t *addr, u32 id, u32 set_id);
int lme_hw_s_wdma_init(void __iomem *base, struct lme_param_set *param_set, u32 enable, u32 id, u32 set_id,
			enum lme_sps_out_mode sps_mode);
int lme_hw_s_wdma_addr(void __iomem *base, struct lme_param_set *param_set,
	u32 id, u32 set_id, u32 lme_mode, enum lme_sps_out_mode sps_mode);
void lme_hw_s_cmdq(void __iomem *base, u32 set_id, dma_addr_t clh, u32 noh);
int lme_hw_s_corex_update_type(void __iomem *base, u32 set_id, u32 type);
void lme_hw_s_corex_init(void __iomem *base, bool enable);
void lme_hw_s_corex_start(void __iomem *base, bool enable);
unsigned int lme_hw_g_int_state(void __iomem *base, bool clear, u32 num_buffers,
	u32 *irq_state, u32 set_id);
unsigned int lme_hw_g_int_mask(void __iomem *base, u32 set_id);
void lme_hw_s_cache(void __iomem *base, u32 set_id, bool enable, u32 prev_width, u32 cur_width);
void lme_hw_s_cache_size(void __iomem *base, u32 set_id, u32 prev_width,
	u32 prev_height, u32 cur_width, u32 cur_height);
void lme_hw_s_mvct_size(void __iomem *base, u32 set_id, u32 width, u32 height);
void lme_hw_s_mvct(void __iomem *base, u32 set_id);
void lme_hw_s_first_frame(void __iomem *base, u32 set_id, bool first_frame);
void lme_hw_s_first_frame_forcely(void __iomem *base, u32 set_id);
void lme_hw_s_crc(void __iomem *base, u32 seed);
struct is_reg *lme_hw_get_reg_struct(void);
unsigned int lme_hw_get_reg_cnt(void);
void lme_hw_s_block_bypass(void __iomem *base, u32 set_id);
unsigned int lme_hw_g_reg_cnt(void);
void lme_hw_s_init(struct pablo_mmio *base, u32 set_id);
bool lme_hw_use_corex_set(void);
bool lme_hw_use_mmio(void);
void lme_hw_init_pmio_config(struct pmio_config *cfg);
#endif
