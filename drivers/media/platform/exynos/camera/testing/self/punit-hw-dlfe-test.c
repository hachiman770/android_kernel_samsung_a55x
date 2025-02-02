// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Exynos Pablo image subsystem functions
 *
 * Copyright (c) 2022 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>

#include "punit-test-hw-ip.h"
#include "punit-test-file-io.h"
#include "pablo-framemgr.h"
#include "is-hw.h"

static int pst_set_hw_dlfe(const char *val, const struct kernel_param *kp);
static int pst_get_hw_dlfe(char *buffer, const struct kernel_param *kp);
static const struct kernel_param_ops pablo_param_ops_hw_dlfe = {
	.set = pst_set_hw_dlfe,
	.get = pst_get_hw_dlfe,
};
module_param_cb(test_hw_dlfe, &pablo_param_ops_hw_dlfe, NULL, 0644);

static const struct mcfp_param mcfp_param_preset[] = {
	[0].otf_input.width = 1920,
	[0].otf_input.height = 1440,
};

/* DLFE shares the RTA related data structures of MCFP */
static struct mcfp_config_set {
	u32 size;
	struct is_mcfp_config cfg;
} mcfg_cfg;
static struct mcfp_cr_set {
	struct size_cr_set mcfp_cr_set;
	struct size_cr_set dlfe_cr_set;
} mcfp_cr;

static struct is_frame *frame_dlfe;
static DECLARE_BITMAP(result, 1);

static void pst_init_param_dlfe(unsigned int index, enum pst_hw_ip_type type)
{
	u32 cr_i = 0;

	mcfp_cr.dlfe_cr_set.cr[cr_i].reg_addr = 0x3404;
	mcfp_cr.dlfe_cr_set.cr[cr_i].reg_data = 1;
	cr_i++;
	mcfp_cr.dlfe_cr_set.cr[cr_i].reg_addr = 0x3404;
	mcfp_cr.dlfe_cr_set.cr[cr_i].reg_data = 0;
	cr_i++;

	mcfp_cr.dlfe_cr_set.size = cr_i;
}

static void pst_set_config_dlfe(struct is_frame *frame)
{
	mcfg_cfg.cfg.dlfe_en = 1;

	frame->kva_mcfp_rta_info[PLANE_INDEX_CONFIG] = (u64)&mcfg_cfg;
}

static void pst_set_rta_info_dlfe(struct is_frame *frame, struct size_cr_set *cr_set)
{
	frame->kva_mcfp_rta_info[PLANE_INDEX_CR_SET] = (u64)cr_set;
}

static void pst_set_param_dlfe(struct is_frame *frame)
{
	struct is_param_region *p_region;
	struct mcfp_param *param;
	const struct mcfp_param *preset;

	p_region = frame->parameter;
	param = &p_region->mcfp;
	preset = mcfp_param_preset;

	memcpy(&param->otf_input, (u32 *)&preset[0].otf_input, PARAMETER_MAX_SIZE);
	set_bit(PARAM_MCFP_OTF_INPUT, frame->pmap);

	pst_set_config_dlfe(frame);
	pst_set_rta_info_dlfe(frame, &mcfp_cr.mcfp_cr_set);
}

static const struct pst_callback_ops pst_cb_dlfe = {
	.init_param = pst_init_param_dlfe,
	.set_param = pst_set_param_dlfe,
	.set_rta_info = pst_set_rta_info_dlfe,
};

const struct pst_callback_ops *pst_get_hw_dlfe_cb(void)
{
	return &pst_cb_dlfe;
}

static int pst_set_hw_dlfe(const char *val, const struct kernel_param *kp)
{
	return pst_set_hw_ip(val,
			DEV_HW_DLFE,
			frame_dlfe,
			NULL,
			&mcfp_cr.mcfp_cr_set,
			1,
			result,
			&pst_cb_dlfe);
}

static int pst_get_hw_dlfe(char *buffer, const struct kernel_param *kp)
{
	return pst_get_hw_ip(buffer, "DLFE", 1, result);
}
