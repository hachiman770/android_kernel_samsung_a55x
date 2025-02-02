// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * drivers/media/radio/exynos/s621/fm_ctrl.c
 *
 * FM Radio Speedy Interface driver for SAMSUNG s621
 *
 * Copyright (c) 2022 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */
#include "s621_io_map.h"
#include "fm_low_struc.h"
#include "radio-s621.h"
#include "fm_ctrl.h"

void fm_pwron(void)
{
	fmspeedy_set_reg(FM_RESET_ASSERT, 1);
	fmspeedy_set_reg(FM_RESET_ASSERT, 0);
	fmspeedy_set_reg(PLL_CLK_EN, 0);
	fmspeedy_set_reg(FORCE_REG_CLK, 3);
	fmspeedy_set_reg(CLK_ENABLE, 0xFF);
}

void fm_pwroff(void)
{
	fmspeedy_set_reg(PLL_CLK_EN, 2);
	fmspeedy_set_reg(FORCE_REG_CLK, 0);
	fmspeedy_set_reg(CLK_ENABLE, 0);
	fmspeedy_set_reg_field(FM_RESET_ASSERT, 0, 0x0001, 0);
}

void fmspeedy_wakeup(void)
{
	write32(gradio->fmspeedy_base + FMSPDY_CTL, SPDY_WAKEUP);
	fmspeedy_set_reg(FM_SPEEDY_SLEEP_EN, 0);
	udelay(10);
}

void fm_en_speedy_m_int(void)
{
	SetBits(gradio->fmspeedy_base + FMSPDY_INTR_MASK,
		FM_SLV_INT_MASK_BIT, 1, 0);
}

void fm_dis_speedy_m_int(void)
{
	SetBits(gradio->fmspeedy_base + FMSPDY_INTR_MASK,
		FM_SLV_INT_MASK_BIT, 1, 1);
}

void fm_speedy_m_int_stat_clear(void)
{
	write32(gradio->fmspeedy_base + FMSPDY_STAT, 0x1F);
}

void fm_speedy_m_int_stat_clear_all(void)
{
	write32(gradio->fmspeedy_base + FMSPDY_STAT, 0x7F);
}

void fm_speedy_m_int_enable(void)
{
	fm_en_speedy_m_int();
	fm_speedy_m_int_stat_clear_all();
}

void fm_speedy_m_int_disable(void)
{
	fm_dis_speedy_m_int();
	fm_speedy_m_int_stat_clear_all();
}

u32 fm_speedy_addr_enc(u32 addr)
{
	return (((addr & 0x7000) >> 6) | ((addr & 0xFC) >> 2));
}

u32 fmspeedy_get_reg_core(u32 addr)
{
	u16 jj = 0;
	u32 status1;
	u32 ret = 0;

	fm_dis_speedy_m_int();

	fm_speedy_m_int_stat_clear();
	write32(gradio->fmspeedy_base + FMSPDY_CMD,
			FMSPDY_READ | FMSPDY_RANDOM
			| (fm_speedy_addr_enc(addr) << 7));

	for (jj = 0; jj < 100; jj++) {
		udelay(2);
		status1 = read32(gradio->fmspeedy_base + FMSPDY_STAT);
		if ((status1 & STAT_DONE) == 1)
			break;
	}

	if (jj >= 99) {
		dev_err(gradio->dev, "%s(), Failed addr:0x%xh\n",
			__func__, addr);
		ret = -1;
		goto get_fail;
	}

	ret = read32(gradio->fmspeedy_base + FMSPDY_DATA);

get_fail:
	fm_en_speedy_m_int();

	return ret;

}

u32 fmspeedy_get_reg(u32 addr)
{
	u32 data;

	SPEEDY_ENTRY(gradio);

	spin_lock_irq(&gradio->slock);

	atomic_set(&gradio->is_doing, 1);
	data = fmspeedy_get_reg_core(addr);
	if (data == -1)
		gradio->speedy_error++;
	atomic_set(&gradio->is_doing, 0);

	spin_unlock_irq(&gradio->slock);

	SPEEDYEBUG(gradio, "%s(), addr[0x%x], data[0x%x], speedy err[0x%x]",
		 __func__, addr, data, gradio->speedy_error);
	SPEEDY_EXIT(gradio);
	return data;
}

u32 fmspeedy_get_reg_work(u32 addr)
{
	u32 data;

	SPEEDY_ENTRY(gradio);

	data = fmspeedy_get_reg_core(addr);
	if (data == -1)
		gradio->speedy_error++;

	SPEEDYEBUG(gradio, "%s(), addr[0x%x], data[0x%x], speedy err[0x%x]",
		 __func__, addr, data, gradio->speedy_error);
	SPEEDY_EXIT(gradio);
	return data;
}

int fmspeedy_set_reg_core(u32 addr, u32 data)
{
	u16 jj;
	u32 status1;
	int ret = 0;

	fm_dis_speedy_m_int();

	fm_speedy_m_int_stat_clear();
	write32(gradio->fmspeedy_base + FMSPDY_DATA, data);
	write32(gradio->fmspeedy_base + FMSPDY_CMD,
		FMSPDY_WRITE | FMSPDY_RANDOM
		| (fm_speedy_addr_enc(addr) << 7));

	for (jj = 0; jj < 100; jj++) {
		udelay(2);
		status1 = read32(gradio->fmspeedy_base + FMSPDY_STAT);
		if ((status1 & STAT_DONE) == 1)
			break;
	}

	if (jj >= 99) {
		dev_err(gradio->dev, "%s(), Failed addr:0x%xh, data:0x%xh\n",
			__func__, addr, data);
		ret = -1;
	}

	fm_en_speedy_m_int();

	return ret;
}

int fmspeedy_set_reg(u32 addr, u32 data)
{
	int ret = 0;

	SPEEDY_ENTRY(gradio);

	spin_lock_irq(&gradio->slock);

	atomic_set(&gradio->is_doing, 1);
	ret = fmspeedy_set_reg_core(addr, data);
	if (ret == -1)
		gradio->speedy_error++;
	atomic_set(&gradio->is_doing, 0);

	spin_unlock_irq(&gradio->slock);
	SPEEDYEBUG(gradio,
		"%s(), addr[0x%x], data[0x%x], ret[0x%x], speedy err[0x%x]",
		 __func__, addr, data, ret, gradio->speedy_error);
	SPEEDY_EXIT(gradio);
	return ret;
}

int fmspeedy_set_reg_work(u32 addr, u32 data)
{
	int ret = 0;

	SPEEDY_ENTRY(gradio);
	ret = fmspeedy_set_reg_core(addr, data);
	if (ret == -1)
		gradio->speedy_error++;

	SPEEDYEBUG(gradio, "%s(), addr[0x%x], data[0x%x], ret[0x%x]", __func__, addr, data, ret);
	SPEEDY_EXIT(gradio);
	return ret;
}

u32 fmspeedy_get_reg_field_core(u32 addr, u32 shift, u32 mask)
{
	u16 jj;
	u32 status1;
	u32 ret = 0;

	fm_dis_speedy_m_int();

	fm_speedy_m_int_stat_clear();
	write32(gradio->fmspeedy_base + FMSPDY_CMD,
			FMSPDY_READ | FMSPDY_RANDOM
			| (fm_speedy_addr_enc(addr) << 7));
	for (jj = 0; jj < 100; jj++) {
		udelay(2);
		status1 = read32(gradio->fmspeedy_base + FMSPDY_STAT);
		if ((status1 & STAT_DONE) == 1)
			break;
	}

	if (jj >= 99) {
		dev_err(gradio->dev, "%s(), Failed addr:0x%xh\n",
			__func__, addr);
		ret = -1;
		goto read_fail_f;
	}
	ret = (read32(gradio->fmspeedy_base + FMSPDY_DATA) & (mask)) >> shift;

read_fail_f:
	fm_en_speedy_m_int();

	return ret;
}

u32 fmspeedy_get_reg_field(u32 addr, u32 shift, u32 mask)
{
	u32 data;

	SPEEDY_ENTRY(gradio);

	spin_lock_irq(&gradio->slock);

	atomic_set(&gradio->is_doing, 1);
	data = fmspeedy_get_reg_field_core(addr, shift, mask);
	if (data == -1)
		gradio->speedy_error++;
	atomic_set(&gradio->is_doing, 0);

	spin_unlock_irq(&gradio->slock);

	SPEEDYEBUG(gradio, "%s(), addr[0x%x], data[0x%x], speedy err[0x%x]",
		 __func__, addr, data, gradio->speedy_error);
	SPEEDY_EXIT(gradio);
	return data;
}

u32 fmspeedy_get_reg_field_work(u32 addr, u32 shift, u32 mask)
{
	u32 data;

	SPEEDY_ENTRY(gradio);

	data = fmspeedy_get_reg_field_core(addr, shift, mask);
	if (data == -1)
		gradio->speedy_error++;

	SPEEDYEBUG(gradio, "%s(), addr[0x%x], data[0x%x]", __func__, addr, data);
	SPEEDY_EXIT(gradio);

	return data;
}

int fmspeedy_set_reg_field_core(u32 addr, u32 shift, u32 mask, u32 data)
{
	u32 value, value1;
	u16 jj;
	u32 status1;
	int ret = 0;

	fm_dis_speedy_m_int();

	fm_speedy_m_int_stat_clear();
	write32(gradio->fmspeedy_base + FMSPDY_CMD,
			FMSPDY_READ | FMSPDY_RANDOM
			| (fm_speedy_addr_enc(addr) << 7));

	for (jj = 0; jj < 100; jj++) {
		udelay(2);
		status1 = read32(gradio->fmspeedy_base + FMSPDY_STAT);
		if ((status1 & STAT_DONE) == 1)
			break;
	}

	if (jj >= 99) {
		dev_err(gradio->dev, "%s(), Failed addr:0x%xh, data:0x%xh, cnt:%d\n",
			__func__, addr, data, jj);
		ret = -1;
		goto set_fail_f;
	}

	value1 = read32(gradio->fmspeedy_base + FMSPDY_DATA);
	value = (value1 & ~(mask)) | ((data) << (shift));

	write32(gradio->fmspeedy_base + FMSPDY_DATA, value);
	write32(gradio->fmspeedy_base + FMSPDY_STAT, 0x1F);
	write32(gradio->fmspeedy_base + FMSPDY_CMD,
		FMSPDY_WRITE | FMSPDY_RANDOM
		| (fm_speedy_addr_enc(addr) << 7));

	for (jj = 0; jj < 100; jj++) {
		udelay(2);
		status1 = read32(gradio->fmspeedy_base + FMSPDY_STAT);
		if ((status1 & STAT_DONE) == 1)
			break;
	}

	if (jj >= 99) {
		dev_err(gradio->dev, "%s(), Failed addr:0x%xh, data:0x%xh, cnt:%d\n",
			__func__, addr, data, jj);
		ret = -1;
	}

set_fail_f:
	fm_en_speedy_m_int();

	return ret;
}

int fmspeedy_set_reg_field(u32 addr, u32 shift, u32 mask, u32 data)
{
	int ret = 0;

	SPEEDY_ENTRY(gradio);
	spin_lock_irq(&gradio->slock);

	atomic_set(&gradio->is_doing, 1);
	ret = fmspeedy_set_reg_field_core(addr, shift, mask, data);
	if (ret == -1)
		gradio->speedy_error++;
	atomic_set(&gradio->is_doing, 0);

	spin_unlock_irq(&gradio->slock);

	SPEEDYEBUG(gradio,
		"%s(), addr[0x%x], data[0x%x], ret[0x%x], speedy err[0x%x]",
		 __func__, addr, data, ret, gradio->speedy_error);
	SPEEDY_EXIT(gradio);
	return ret;
}

int fmspeedy_set_reg_field_work(u32 addr, u32 shift, u32 mask, u32 data)
{
	int ret = 0;

	SPEEDY_ENTRY(gradio);
	ret = fmspeedy_set_reg_field_core(addr, shift, mask, data);
	if (ret == -1)
		gradio->speedy_error++;

	SPEEDYEBUG(gradio, "%s(), addr[0x%x], data[0x%x], ret[0x%x]", __func__, addr, data, ret);
	SPEEDY_EXIT(gradio);

	return ret;
}

void fm_audio_check_work(void)
{
	u32 read;

	API_ENTRY(gradio);

	read = read32(gradio->fmspeedy_base + AUDIO_FIFO);
	dev_err(gradio->dev, "AUDIO_FIFO : 0x%08x\n", read);

	read = read32(gradio->fmspeedy_base + AUDIO_LR_DATA);
	dev_err(gradio->dev, "AUDIO_LR_DATA : 0x%08x\n", read);

	API_EXIT(gradio);
}

void fm_audio_check(void)
{
	u32 read;

	API_ENTRY(gradio);

	spin_lock_irq(&gradio->slock);
	atomic_set(&gradio->is_doing, 1);

	read = read32(gradio->fmspeedy_base + AUDIO_FIFO);
	dev_err(gradio->dev, "AUDIO_FIFO : 0x%08x\n", read);

	read = read32(gradio->fmspeedy_base + AUDIO_LR_DATA);
	dev_err(gradio->dev, "AUDIO_LR_DATA : 0x%08x\n", read);

	atomic_set(&gradio->is_doing, 0);
	spin_unlock_irq(&gradio->slock);

	API_EXIT(gradio);
}

/****************************************************************************
 NAME
 fm_audio_control   -  Audio out enable/disable

 FUNCTION
 Setting registers for Audio
 ****************************************************************************/
void fm_audio_control(struct s621_radio *radio,
		bool audio_out, bool lr_switch,
		u32 req_time, u32 audio_addr)
{
	write32(radio->fmspeedy_base + AUDIO_CTRL,
		((audio_out << 21) | (lr_switch << 20)
		| ((req_time & 0x07FF) << 9)
		| (audio_addr & 0x01FF)));
	udelay(15);
}

void fm_set_audio_enable(bool enable)
{
	if (enable)
		fm_audio_control(gradio, 1, 1, 0x100, 0x140);
	else
		fm_audio_control(gradio, 0, 1, 0x100, 0x140);
}

