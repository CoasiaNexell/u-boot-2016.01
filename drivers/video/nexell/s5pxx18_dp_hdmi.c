/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 *
 * Author: junghyun, kim <jhkim@nexell.co.kr>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <config.h>
#include <common.h>
#include <errno.h>

#include <asm/arch/nexell.h>
#include <asm/arch/tieoff.h>
#include <asm/arch/reset.h>
#include <asm/arch/display.h>

#include <linux/hdmi.h>

#include "soc/s5pxx18_soc_dpc.h"
#include "soc/s5pxx18_soc_hdmi.h"
#include "soc/s5pxx18_soc_disptop.h"
#include "soc/s5pxx18_soc_disptop_clk.h"

#include "soc/s5pxx18_hdmi.h"

#define DEFAULT_SAMPLE_RATE			48000
#define DEFAULT_BITS_PER_SAMPLE		16
#define DEFAULT_AUDIO_CODEC			HDMI_AUDIO_PCM

#define	__io_address(a)	(void *)(uintptr_t)(a)

static const u8 hdmiphy_preset_74_25[32] = {
	0xd1, 0x1f, 0x10, 0x40, 0x40, 0xf8, 0xc8, 0x81,
	0xe8, 0xba, 0xd8, 0x45, 0xa0, 0xac, 0x80, 0x08,
	0x80, 0x09, 0x84, 0x05, 0x22, 0x24, 0x86, 0x54,
	0xa5, 0x24, 0x01, 0x00, 0x00, 0x01, 0x80, 0x10,
};

static const u8 hdmiphy_preset_83_5[32] = {
	0xd1, 0x23, 0x11, 0x40, 0x0c, 0xfb, 0xc8, 0x85,
	0xe8, 0xd1, 0xd8, 0x45, 0xa0, 0xac, 0x80, 0x08,
	0x80, 0x09, 0x84, 0x05, 0x22, 0x24, 0x86, 0x54,
	0x93, 0x24, 0x01, 0x00, 0x00, 0x01, 0x80, 0x10,
};

static const u8 hdmiphy_preset_106_5[32] = {
	0xD1, 0x2C, 0x12, 0x40, 0x0C, 0x09, 0xC8, 0x84,
	0xE8, 0x0A, 0xD9, 0x45, 0xA0, 0xAC, 0x80, 0x08,
	0x80, 0x09, 0x84, 0x05, 0x22, 0x24, 0x86, 0x54,
	0x73, 0x24, 0x01, 0x00, 0x00, 0x01, 0x80, 0x10,
};

static const u8 hdmiphy_preset_108[32] = {
	0x51, 0x2d, 0x15, 0x40, 0x01, 0x00, 0xc8, 0x82,
	0xc8, 0x0e, 0xd9, 0x45, 0xa0, 0xac, 0x80, 0x08,
	0x80, 0x09, 0x84, 0x05, 0x22, 0x24, 0x86, 0x54,
	0xc7, 0x25, 0x03, 0x00, 0x00, 0x01, 0x80, 0x10,
};

static const u8 hdmiphy_preset_108_108[32] = {
	0xD1, 0x2D, 0x12, 0x40, 0x64, 0x12, 0xC8, 0x43,
	0xE8, 0x0E, 0xD9, 0x45, 0xA0, 0xAC, 0x80, 0x56,
	0x80, 0x09, 0x84, 0x05, 0x22, 0x24, 0x86, 0x54,
	0xC7, 0x25, 0x03, 0x00, 0x00, 0x01, 0x80, 0x10,
};

static const u8 hdmiphy_preset_148_352[32] = {
	0xd1, 0x1f, 0x00, 0x40, 0x5b, 0xef, 0xc8, 0x81,
	0xe8, 0xb9, 0xd8, 0x45, 0xa0, 0xac, 0x80, 0x0a,
	0x80, 0x09, 0x84, 0x05, 0x22, 0x24, 0x86, 0x54,
	0x4b, 0x25, 0x03, 0x00, 0x00, 0x01, 0x80, 0x10,
};

static const u8 hdmiphy_preset_148_5[32] = {
	0xd1, 0x1f, 0x00, 0x40, 0x40, 0xf8, 0xc8, 0x81,
	0xe8, 0xba, 0xd8, 0x45, 0xa0, 0xac, 0x80, 0x08,
	0x80, 0x09, 0x84, 0x05, 0x22, 0x24, 0x86, 0x54,
	0x4b, 0x25, 0x03, 0x00, 0x00, 0x01, 0x80, 0x10,
};

#define HDMIPHY_PRESET_TABLE_SIZE   (32)

enum NXP_HDMI_PRESET {
	NXP_HDMI_PRESET_720P = 0,	/* 1280 x 720 */
	NXP_HDMI_PRESET_1080P,	/* 1920 x 1080 */
	NXP_HDMI_PRESET_1920x720P,	/* 1920 x 720 */
	NXP_HDMI_PRESET_MAX
};

static void hdmi_reset(void)
{
	nx_rstcon_setrst(RESET_ID_HDMI_VIDEO, RSTCON_ASSERT);
	nx_rstcon_setrst(RESET_ID_HDMI_SPDIF, RSTCON_ASSERT);
	nx_rstcon_setrst(RESET_ID_HDMI_TMDS, RSTCON_ASSERT);
	nx_rstcon_setrst(RESET_ID_HDMI_VIDEO, RSTCON_NEGATE);
	nx_rstcon_setrst(RESET_ID_HDMI_SPDIF, RSTCON_NEGATE);
	nx_rstcon_setrst(RESET_ID_HDMI_TMDS, RSTCON_NEGATE);
}

static int hdmi_phy_enable(int preset, int enable)
{
	const u8 *table = NULL;
	int size = 0;
	u32 addr, i = 0;

	if (!enable)
		return 0;

	switch (preset) {
	case NXP_HDMI_PRESET_720P:
		table = hdmiphy_preset_74_25;
		size = 32;
		break;
	case NXP_HDMI_PRESET_1080P:
		table = hdmiphy_preset_148_5;
		size = 31;
		break;
	case NXP_HDMI_PRESET_1920x720P:
		table = hdmiphy_preset_83_5;
		size = 32;
		break;
	default:
		printf("hdmi: phy not support preset %d\n", preset);
		return -EINVAL;
	}

	nx_hdmi_set_reg(0, HDMI_PHY_REG7C, (0 << 7));
	nx_hdmi_set_reg(0, HDMI_PHY_REG7C, (0 << 7));
	nx_hdmi_set_reg(0, HDMI_PHY_REG04, (0 << 4));
	nx_hdmi_set_reg(0, HDMI_PHY_REG04, (0 << 4));
	nx_hdmi_set_reg(0, HDMI_PHY_REG24, (1 << 7));
	nx_hdmi_set_reg(0, HDMI_PHY_REG24, (1 << 7));

	for (i = 0, addr = HDMI_PHY_REG04; size > i; i++, addr += 4) {
		nx_hdmi_set_reg(0, addr, table[i]);
		nx_hdmi_set_reg(0, addr, table[i]);
	}

	nx_hdmi_set_reg(0, HDMI_PHY_REG7C, 0x80);
	nx_hdmi_set_reg(0, HDMI_PHY_REG7C, 0x80);
	nx_hdmi_set_reg(0, HDMI_PHY_REG7C, (1 << 7));
	nx_hdmi_set_reg(0, HDMI_PHY_REG7C, (1 << 7));
	debug("%s: preset = %d\n", __func__, preset);

	return 0;
}

static inline int hdmi_wait_phy_ready(void)
{
	int count = 500;
	do {
		u32 val = nx_hdmi_get_reg(0, HDMI_LINK_PHY_STATUS_0);
		if (val & 0x01) {
#ifndef QUICKBOOT
			printf("HDMI: phy ready...\n");
#endif
			return 1;
		}
		mdelay(10);
	} while (count--);

	return 0;
}

static inline int hdmi_get_vsync(int preset,
				 struct dp_sync_info *sync,
				 struct dp_ctrl_info *ctrl,
				 u8 *vic)
{
	switch (preset) {
	case NXP_HDMI_PRESET_720P:	/* 720p: 1280x720 */
		sync->h_active_len = 1280;
		sync->h_sync_width = 40;
		sync->h_back_porch = 220;
		sync->h_front_porch = 110;
		sync->h_sync_invert = 0;
		sync->v_active_len = 720;
		sync->v_sync_width = 5;
		sync->v_back_porch = 20;
		sync->v_front_porch = 5;
		sync->v_sync_invert = 0;
		if (vic)
			*vic = 4;
		break;

	case NXP_HDMI_PRESET_1080P:	/* 1080p: 1920x1080 */
		sync->h_active_len = 1920;
		sync->h_sync_width = 44;
		sync->h_back_porch = 148;
		sync->h_front_porch = 88;
		sync->h_sync_invert = 0;
		sync->v_active_len = 1080;
		sync->v_sync_width = 5;
		sync->v_back_porch = 36;
		sync->v_front_porch = 4;
		sync->v_sync_invert = 0;
		if (vic)
			*vic = 16;
		break;

	case NXP_HDMI_PRESET_1920x720P:
		sync->h_active_len = 1920;
		sync->h_sync_width = 40;
		sync->h_back_porch = 44;
		sync->h_front_porch = 44;
		sync->h_sync_invert = 0;
		sync->v_active_len = 720;
		sync->v_sync_width = 2;
		sync->v_back_porch = 4;
		sync->v_front_porch = 4;
		sync->v_sync_invert = 0;
		break;

	default:
		printf("HDMI: not support preset sync %d\n", preset);
		return -EINVAL;
	}

	ctrl->clk_src_lv0 = 4;
	ctrl->clk_div_lv0 = 1;
	ctrl->clk_src_lv1 = 7;
	ctrl->clk_div_lv1 = 1;

	ctrl->out_format = outputformat_rgb888;
	ctrl->delay_mask = (DP_SYNC_DELAY_RGB_PVD | DP_SYNC_DELAY_HSYNC_CP1 |
			    DP_SYNC_DELAY_VSYNC_FRAM | DP_SYNC_DELAY_DE_CP);
	ctrl->d_rgb_pvd = 0;
	ctrl->d_hsync_cp1 = 0;
	ctrl->d_vsync_fram = 0;
	ctrl->d_de_cp2 = 7;

	/* HFP + HSW + HBP + AVWidth-VSCLRPIXEL- 1; */
	ctrl->vs_start_offset = (sync->h_front_porch + sync->h_sync_width +
				 sync->h_back_porch + sync->h_active_len - 1);
	ctrl->vs_end_offset = 0;

	/* HFP + HSW + HBP + AVWidth-EVENVSCLRPIXEL- 1 */
	ctrl->ev_start_offset = (sync->h_front_porch + sync->h_sync_width +
				 sync->h_back_porch + sync->h_active_len - 1);
	ctrl->ev_end_offset = 0;
	debug("%s: preset: %d\n", __func__, preset);

	return 0;
}

static void hdmi_clock(void)
{
	void *base =
	    __io_address(nx_disp_top_clkgen_get_physical_address
			 (to_mipi_clkgen));

	nx_disp_top_clkgen_set_base_address(to_mipi_clkgen, base);
	nx_disp_top_clkgen_set_clock_divisor_enable(to_mipi_clkgen, 0);
	nx_disp_top_clkgen_set_clock_pclk_mode(to_mipi_clkgen,
					       nx_pclkmode_always);
	nx_disp_top_clkgen_set_clock_source(to_mipi_clkgen, HDMI_SPDIF_CLKOUT,
					    2);
	nx_disp_top_clkgen_set_clock_divisor(to_mipi_clkgen, HDMI_SPDIF_CLKOUT,
					     2);
	nx_disp_top_clkgen_set_clock_source(to_mipi_clkgen, 1, 7);
	nx_disp_top_clkgen_set_clock_divisor_enable(to_mipi_clkgen, 1);

	/* must initialize this !!! */
	nx_disp_top_hdmi_set_vsync_hsstart_end(0, 0);
	nx_disp_top_hdmi_set_vsync_start(0);
	nx_disp_top_hdmi_set_hactive_start(0);
	nx_disp_top_hdmi_set_hactive_end(0);
}

static void hdmi_vsync(struct dp_sync_info *sync)
{
	int width = sync->h_active_len;
	int hsw = sync->h_sync_width;
	int hbp = sync->h_back_porch;
	int height = sync->v_active_len;
	int vsw = sync->v_sync_width;
	int vbp = sync->v_back_porch;

	int v_sync_s = vsw + vbp + height - 1;
	int h_active_s = hsw + hbp;
	int h_active_e = width + hsw + hbp;
	int v_sync_hs_se0 = hsw + hbp + 1;
	int v_sync_hs_se1 = hsw + hbp + 2;

	nx_disp_top_hdmi_set_vsync_start(v_sync_s);
	nx_disp_top_hdmi_set_hactive_start(h_active_s);
	nx_disp_top_hdmi_set_hactive_end(h_active_e);
	nx_disp_top_hdmi_set_vsync_hsstart_end(v_sync_hs_se0, v_sync_hs_se1);
}

static int hdmi_prepare(struct dp_sync_info *sync)
{
	int width = sync->h_active_len;
	int hsw = sync->h_sync_width;
	int hfp = sync->h_front_porch;
	int hbp = sync->h_back_porch;
	int height = sync->v_active_len;
	int vsw = sync->v_sync_width;
	int vfp = sync->v_front_porch;
	int vbp = sync->v_back_porch;

	u32 h_blank, h_line, h_sync_start, h_sync_end;
	u32 v_blank, v2_blank, v_line;
	u32 v_sync_line_bef_1, v_sync_line_bef_2;

	u32 fixed_ffff = 0xffff;

	/* calculate sync variables */
	h_blank = hfp + hsw + hbp;
	v_blank = vfp + vsw + vbp;
	v2_blank = height + vfp + vsw + vbp;
	v_line = height + vfp + vsw + vbp;	/* total v */
	h_line = width + hfp + hsw + hbp;	/* total h */
	h_sync_start = hfp;
	h_sync_end = hfp + hsw;
	v_sync_line_bef_1 = vfp;
	v_sync_line_bef_2 = vfp + vsw;

	/* no blue screen mode, encoding order as it is */
	nx_hdmi_set_reg(0, HDMI_LINK_HDMI_CON_0, (0 << 5) | (1 << 4));

	/* set HDMI_LINK_BLUE_SCREEN_* to 0x0 */
	nx_hdmi_set_reg(0, HDMI_LINK_BLUE_SCREEN_R_0, 0x5555);
	nx_hdmi_set_reg(0, HDMI_LINK_BLUE_SCREEN_R_1, 0x5555);
	nx_hdmi_set_reg(0, HDMI_LINK_BLUE_SCREEN_G_0, 0x5555);
	nx_hdmi_set_reg(0, HDMI_LINK_BLUE_SCREEN_G_1, 0x5555);
	nx_hdmi_set_reg(0, HDMI_LINK_BLUE_SCREEN_B_0, 0x5555);
	nx_hdmi_set_reg(0, HDMI_LINK_BLUE_SCREEN_B_1, 0x5555);

	/* set HDMI_CON_1 to 0x0 */
	nx_hdmi_set_reg(0, HDMI_LINK_HDMI_CON_1, 0x0);
	nx_hdmi_set_reg(0, HDMI_LINK_HDMI_CON_2, 0x0);

	/* set interrupt : enable hpd_plug, hpd_unplug */
	nx_hdmi_set_reg(0, HDMI_LINK_INTC_CON_0,
			(1 << 6) | (1 << 3) | (1 << 2));

	/* set STATUS_EN to 0x17 */
	nx_hdmi_set_reg(0, HDMI_LINK_STATUS_EN, 0x17);

	/* TODO set HDP to 0x0 : later check hpd */
	nx_hdmi_set_reg(0, HDMI_LINK_HPD, 0x0);

	/* set MODE_SEL to 0x02 */
	nx_hdmi_set_reg(0, HDMI_LINK_MODE_SEL, 0x2);

	/* set H_BLANK_*, V1_BLANK_*, V2_BLANK_*, V_LINE_*,
	 * H_LINE_*, H_SYNC_START_*, H_SYNC_END_ *
	 * V_SYNC_LINE_BEF_1_*, V_SYNC_LINE_BEF_2_*
	 */
	nx_hdmi_set_reg(0, HDMI_LINK_H_BLANK_0, h_blank % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_H_BLANK_1, h_blank >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_V1_BLANK_0, v_blank % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_V1_BLANK_1, v_blank >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_V2_BLANK_0, v2_blank % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_V2_BLANK_1, v2_blank >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_V_LINE_0, v_line % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_V_LINE_1, v_line >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_H_LINE_0, h_line % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_H_LINE_1, h_line >> 8);

	if (width == 1280) {
		nx_hdmi_set_reg(0, HDMI_LINK_HSYNC_POL, 0x1);
		nx_hdmi_set_reg(0, HDMI_LINK_VSYNC_POL, 0x1);
	} else {
		nx_hdmi_set_reg(0, HDMI_LINK_HSYNC_POL, 0x0);
		nx_hdmi_set_reg(0, HDMI_LINK_VSYNC_POL, 0x0);
	}

	nx_hdmi_set_reg(0, HDMI_LINK_INT_PRO_MODE, 0x0);

	nx_hdmi_set_reg(0, HDMI_LINK_H_SYNC_START_0, (h_sync_start % 256) - 2);
	nx_hdmi_set_reg(0, HDMI_LINK_H_SYNC_START_1, h_sync_start >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_H_SYNC_END_0, (h_sync_end % 256) - 2);
	nx_hdmi_set_reg(0, HDMI_LINK_H_SYNC_END_1, h_sync_end >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_BEF_1_0,
			v_sync_line_bef_1 % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_BEF_1_1,
			v_sync_line_bef_1 >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_BEF_2_0,
			v_sync_line_bef_2 % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_BEF_2_1,
			v_sync_line_bef_2 >> 8);

	/* Set V_SYNC_LINE_AFT*, V_SYNC_LINE_AFT_PXL*, VACT_SPACE* */
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_1_0, fixed_ffff % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_1_1, fixed_ffff >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_2_0, fixed_ffff % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_2_1, fixed_ffff >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_3_0, fixed_ffff % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_3_1, fixed_ffff >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_4_0, fixed_ffff % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_4_1, fixed_ffff >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_5_0, fixed_ffff % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_5_1, fixed_ffff >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_6_0, fixed_ffff % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_6_1, fixed_ffff >> 8);

	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_1_0, fixed_ffff % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_1_1, fixed_ffff >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_2_0, fixed_ffff % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_2_1, fixed_ffff >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_3_0, fixed_ffff % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_3_1, fixed_ffff >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_4_0, fixed_ffff % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_4_1, fixed_ffff >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_5_0, fixed_ffff % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_5_1, fixed_ffff >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_6_0, fixed_ffff % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_V_SYNC_LINE_AFT_PXL_6_1, fixed_ffff >> 8);

	nx_hdmi_set_reg(0, HDMI_LINK_VACT_SPACE1_0, fixed_ffff % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_VACT_SPACE1_1, fixed_ffff >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_VACT_SPACE2_0, fixed_ffff % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_VACT_SPACE2_1, fixed_ffff >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_VACT_SPACE3_0, fixed_ffff % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_VACT_SPACE3_1, fixed_ffff >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_VACT_SPACE4_0, fixed_ffff % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_VACT_SPACE4_1, fixed_ffff >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_VACT_SPACE5_0, fixed_ffff % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_VACT_SPACE5_1, fixed_ffff >> 8);
	nx_hdmi_set_reg(0, HDMI_LINK_VACT_SPACE6_0, fixed_ffff % 256);
	nx_hdmi_set_reg(0, HDMI_LINK_VACT_SPACE6_1, fixed_ffff >> 8);

	nx_hdmi_set_reg(0, HDMI_LINK_CSC_MUX, 0x0);
	nx_hdmi_set_reg(0, HDMI_LINK_SYNC_GEN_MUX, 0x0);

	nx_hdmi_set_reg(0, HDMI_LINK_SEND_START_0, 0xfd);
	nx_hdmi_set_reg(0, HDMI_LINK_SEND_START_1, 0x01);
	nx_hdmi_set_reg(0, HDMI_LINK_SEND_END_0, 0x0d);
	nx_hdmi_set_reg(0, HDMI_LINK_SEND_END_1, 0x3a);
	nx_hdmi_set_reg(0, HDMI_LINK_SEND_END_2, 0x08);

	/* Set DC_CONTROL to 0x00 */
	nx_hdmi_set_reg(0, HDMI_LINK_DC_CONTROL, 0x0);

#if CONFIG_HDMI_PATTERN
	nx_hdmi_set_reg(0, HDMI_LINK_VIDEO_PATTERN_GEN, 0x1);
#else
	nx_hdmi_set_reg(0, HDMI_LINK_VIDEO_PATTERN_GEN, 0x0);
#endif

	nx_hdmi_set_reg(0, HDMI_LINK_GCP_CON, 0x0a);
	return 0;
}

static void hdmi_init(void)
{
	void *base;
   /**
     * [SEQ 2] set the HDMI CLKGEN's PCLKMODE to always enabled
     */
	base =
	    __io_address(nx_disp_top_clkgen_get_physical_address(hdmi_clkgen));
	nx_disp_top_clkgen_set_base_address(hdmi_clkgen, base);
	nx_disp_top_clkgen_set_clock_pclk_mode(hdmi_clkgen, nx_pclkmode_always);

	base = __io_address(nx_hdmi_get_physical_address(0));
	nx_hdmi_set_base_address(0, base);

    /**
     * [SEQ 3] set the 0xC001100C[0] to 1
     */
	nx_tieoff_set(NX_TIEOFF_DISPLAYTOP0_i_HDMI_PHY_REFCLK_SEL, 1);

    /**
     * [SEQ 4] release the resets of HDMI.i_PHY_nRST and HDMI.i_nRST
     */
	nx_rstcon_setrst(RESET_ID_HDMI_PHY, RSTCON_ASSERT);
	nx_rstcon_setrst(RESET_ID_HDMI, RSTCON_ASSERT);
	nx_rstcon_setrst(RESET_ID_HDMI_PHY, RSTCON_NEGATE);
	nx_rstcon_setrst(RESET_ID_HDMI, RSTCON_NEGATE);
}

static void hdmi_stop_vsi(void)
{
	nx_hdmi_set_regb(0, HDMI_LINK_VSI_CON, HDMI_VSI_CON_DO_NOT_TRANSMIT);
}

static u8 hdmi_chksum(u32 start, u8 len, u32 hdr_sum)
{
	int i;

	/* hdr_sum : header0 + header1 + header2
	 * start : start address of packet byte1
	 * len : packet bytes - 1
	 */
	for (i = 0; i < len; ++i)
		hdr_sum += nx_hdmi_get_reg(0, start + i * 4);

	return (u8) (0x100 - (hdr_sum & 0xff));
}

static inline bool hdmi_valid_ratio_4_3(const struct dp_sync_info *sync)
{
	if ((sync->h_active_len == 720 &&
	     sync->v_active_len == 480 &&
	     sync->pixel_clock_hz == 27000000) ||
	    (sync->h_active_len == 720 &&
	     sync->v_active_len == 480 && sync->pixel_clock_hz == 27027000))
		return true;

	return false;
}

static void hdmi_reg_infoframe(union hdmi_infoframe *infoframe,
			       const struct hdmi_format *format,
				   const struct dp_sync_info *sync,
				   const u8 *preset_vic)
{
	const enum color_range preset_color_range = AVI_LIMITED_RANGE;
	const enum hdmi_picture_aspect preset_aspect_ratio =
		HDMI_AVI_PICTURE_ASPECT_16_9;
	bool dvi_mode = false;
	u32 hdr_sum;
	u8 chksum;
	u32 aspect_ratio;
	u8 vic;

	debug("%s: infoframe type = 0x%x, %s\n", __func__,
		infoframe->any.type, dvi_mode ? "dvi monitor" : "hdmi monitor");

	if (dvi_mode) {
		nx_hdmi_set_regb(0, HDMI_LINK_VSI_CON, HDMI_VSI_CON_DO_NOT_TRANSMIT);
		nx_hdmi_set_regb(0, HDMI_LINK_AVI_CON, HDMI_AVI_CON_DO_NOT_TRANSMIT);
		nx_hdmi_set_reg(0, HDMI_LINK_AUI_CON, HDMI_AUI_CON_NO_TRAN);
		return;
	}

	switch (infoframe->any.type) {
	case HDMI_INFOFRAME_TYPE_VENDOR:
		nx_hdmi_set_regb(0, HDMI_LINK_VSI_CON, HDMI_VSI_CON_EVERY_VSYNC);
		nx_hdmi_set_regb(0, HDMI_LINK_VSI_HEADER0, infoframe->any.type);
		nx_hdmi_set_regb(0, HDMI_LINK_VSI_HEADER1, infoframe->any.version);

		/* 0x000C03 : 24bit IEEE Registration Identifier */
		nx_hdmi_set_regb(0, HDMI_LINK_VSI_DATA01, 0x03);
		nx_hdmi_set_regb(0, HDMI_LINK_VSI_DATA02, 0x0c);
		nx_hdmi_set_regb(0, HDMI_LINK_VSI_DATA03, 0x00);
		nx_hdmi_set_regb(0, HDMI_LINK_VSI_DATA04,
			    HDMI_VSI_DATA04_VIDEO_FORMAT(format->vformat));
		nx_hdmi_set_regb(0, HDMI_LINK_VSI_DATA05,
			    HDMI_VSI_DATA05_3D_STRUCTURE(format->type_3d));

		if (format->type_3d == HDMI_3D_TYPE_SB_HALF) {
			infoframe->any.length += 1;
			nx_hdmi_set_regb(0, HDMI_LINK_VSI_DATA06,
				    (u8)
				    HDMI_VSI_DATA06_3D_EXT_DATA
				    (HDMI_H_SUB_SAMPLE));
		}

		nx_hdmi_set_regb(0, HDMI_LINK_VSI_HEADER2, infoframe->any.length);
		hdr_sum =
		    infoframe->any.type + infoframe->any.version +
		    infoframe->any.length;
		chksum =
		    hdmi_chksum(HDMI_LINK_VSI_DATA01, infoframe->any.length,
				hdr_sum);

		printf("%s: VSI checksum = 0x%x\n", __func__, chksum);
		nx_hdmi_set_regb(0, HDMI_LINK_VSI_DATA00, chksum);
		break;

	case HDMI_INFOFRAME_TYPE_AVI:
		nx_hdmi_set_regb(0, HDMI_LINK_AVI_CON, HDMI_AVI_CON_EVERY_VSYNC);
		nx_hdmi_set_regb(0, HDMI_LINK_AVI_HEADER0, infoframe->any.type);
		nx_hdmi_set_regb(0, HDMI_LINK_AVI_HEADER1, infoframe->any.version);
		nx_hdmi_set_regb(0, HDMI_LINK_AVI_HEADER2, infoframe->any.length);
		hdr_sum = infoframe->any.type +
		    infoframe->any.version + infoframe->any.length;

		nx_hdmi_set_regb(0, HDMI_LINK_AVI_BYTE01, HDMI_OUTPUT_RGB888 << 5 |
			    AVI_ACTIVE_FORMAT_VALID |
			    AVI_UNDERSCANNED_DISPLAY_VALID);

		if (preset_aspect_ratio == HDMI_PICTURE_ASPECT_4_3 &&
		    hdmi_valid_ratio_4_3(sync)) {
			aspect_ratio = HDMI_AVI_PICTURE_ASPECT_4_3;
			/* 17 : 576P50Hz 4:3 aspect ratio */
			vic = 17;
		} else {
			aspect_ratio = HDMI_AVI_PICTURE_ASPECT_16_9;
			if (preset_vic)	vic = *preset_vic;
			else			vic = 0;
		}

		nx_hdmi_set_regb(0, HDMI_LINK_AVI_BYTE02, HDMI_PICTURE_ASPECT_16_9 |
			    AVI_SAME_AS_PIC_ASPECT_RATIO | AVI_ITU709);

		if (preset_color_range == AVI_FULL_RANGE)
			nx_hdmi_set_regb(0, HDMI_LINK_AVI_BYTE03, AVI_FULL_RANGE);
		else
			nx_hdmi_set_regb(0, HDMI_LINK_AVI_BYTE03, AVI_LIMITED_RANGE);

		nx_hdmi_set_regb(0, HDMI_LINK_AVI_BYTE04, vic);
		chksum = hdmi_chksum(HDMI_LINK_AVI_BYTE01,
				     infoframe->any.length, hdr_sum);

		debug("%s: AVI checksum = 0x%x\n", __func__, chksum);
		nx_hdmi_set_regb(0, HDMI_LINK_AVI_CHECK_SUM, chksum);
		break;

	case HDMI_INFOFRAME_TYPE_AUDIO:
		nx_hdmi_set_reg(0, HDMI_LINK_AUI_CON, HDMI_AUI_CON_TRANS_EVERY_VSYNC);
		nx_hdmi_set_regb(0, HDMI_LINK_AUI_HEADER0, infoframe->any.type);
		nx_hdmi_set_regb(0, HDMI_LINK_AUI_HEADER1, infoframe->any.version);
		nx_hdmi_set_regb(0, HDMI_LINK_AUI_HEADER2, infoframe->any.length);

#ifdef SPEAKER_PLACEMENT
		/* speaker placement */
		if (audio_channel_count == 6)
			nx_hdmi_set_regb(0, (HDMI_LINK_AUI_BYTEX + 4 * (4 - 1)), 0x0b);
		else if (audio_channel_count == 8)
			nx_hdmi_set_regb(0, (HDMI_LINK_AUI_BYTEX + 4 * (4 - 1)), 0x13);
		else
			nx_hdmi_set_regb(0, (HDMI_LINK_AUI_BYTEX + 4 * (4 - 1)), 0x00);
#endif
		hdr_sum = infoframe->any.type + infoframe->any.version +
		    infoframe->any.length;
		chksum = hdmi_chksum(HDMI_LINK_AUI_BYTEX,
				     infoframe->any.length, hdr_sum);
		debug("%s: AUI checksum = 0x%x\n", __func__, chksum);
		nx_hdmi_set_regb(0, HDMI_LINK_AUI_CHECK_SUM, chksum);
		break;

	default:
		printf("%s: unknown type(0x%x)\n",
		       __func__, infoframe->any.type);
		break;
	}
}

static void hdmi_infoframe_set(const struct dp_sync_info *sync,
				 const u8 *preset_vic)
{
	union hdmi_infoframe infoframe;
	struct hdmi_format format;

	/* Fixed as 2D */
	memset(&format, 0, sizeof(struct hdmi_format));
	format.vformat = HDMI_VIDEO_FORMAT_2D;

	debug("%s: format [%s]\n", __func__,
		 format.vformat == HDMI_VIDEO_FORMAT_3D ? "3D" : "2D");

	/* vendor infoframe */
	if (format.vformat != HDMI_VIDEO_FORMAT_3D) {
		hdmi_stop_vsi();
	} else {
		infoframe.any.type = HDMI_INFOFRAME_TYPE_VENDOR;
		infoframe.any.version = HDMI_VSI_VERSION;
		infoframe.any.length = HDMI_VSI_LENGTH;
		hdmi_reg_infoframe(&infoframe, &format, sync, preset_vic);
	}

	/* avi infoframe */
	infoframe.any.type = HDMI_INFOFRAME_TYPE_AVI;
	infoframe.any.version = HDMI_AVI_VERSION;
	infoframe.any.length = HDMI_AVI_LENGTH;
	hdmi_reg_infoframe(&infoframe, &format, sync, preset_vic);

	/* audio infoframe */
	infoframe.any.type = HDMI_INFOFRAME_TYPE_AUDIO;
	infoframe.any.version = HDMI_AUI_VERSION;
	infoframe.any.length = HDMI_AUI_LENGTH;
	hdmi_reg_infoframe(&infoframe, &format, sync, preset_vic);
}

static void hdmi_audio_enable(bool on)
{
	if (on)
		nx_hdmi_write_mask(0, HDMI_LINK_HDMI_CON_0, ~0, HDMI_ASP_ENABLE);
	else
		nx_hdmi_write_mask(0, HDMI_LINK_HDMI_CON_0, 0, HDMI_ASP_ENABLE);
}

static void hdmi_set_acr(int sample_rate, bool dvi_mode)
{
	u32 n, cts;

	debug("%s %s\n",
		 __func__, dvi_mode ? "dvi monitor" : "hdmi monitor");

	if (dvi_mode) {
		nx_hdmi_set_reg(0, HDMI_LINK_ACR_CON, HDMI_ACR_CON_TX_MODE_NO_TX);
		return;
	}

	if (sample_rate == 32000) {
		n = 4096;
		cts = 27000;
	} else if (sample_rate == 44100) {
		n = 6272;
		cts = 30000;
	} else if (sample_rate == 48000) {
		n = 6144;
		cts = 27000;
	} else if (sample_rate == 88200) {
		n = 12544;
		cts = 30000;
	} else if (sample_rate == 96000) {
		n = 12288;
		cts = 27000;
	} else if (sample_rate == 176400) {
		n = 25088;
		cts = 30000;
	} else if (sample_rate == 192000) {
		n = 24576;
		cts = 27000;
	} else {
		n = 0;
		cts = 0;
	}

	nx_hdmi_set_reg(0, HDMI_LINK_ACR_N0, HDMI_ACR_N0_VAL(n));
	nx_hdmi_set_reg(0, HDMI_LINK_ACR_N1, HDMI_ACR_N1_VAL(n));
	nx_hdmi_set_reg(0, HDMI_LINK_ACR_N2, HDMI_ACR_N2_VAL(n));

	/* dsi_transfer ACR packet */
	nx_hdmi_set_reg(0, HDMI_LINK_ACR_CON, HDMI_ACR_CON_TX_MODE_MESURED_CTS);
}

static void hdmi_spdif_init(int audio_codec, int bits_per_sample)
{
	u32 val;
	int bps, rep_time;

	nx_hdmi_set_reg(0, HDMI_LINK_I2S_CLK_CON, HDMI_I2S_CLK_ENABLE);

	val = HDMI_SPDIFIN_CFG_NOISE_FILTER_2_SAMPLE |
	    HDMI_SPDIFIN_CFG_PCPD_MANUAL |
	    HDMI_SPDIFIN_CFG_WORD_LENGTH_MANUAL |
	    HDMI_SPDIFIN_CFG_UVCP_REPORT |
	    HDMI_SPDIFIN_CFG_HDMI_2_BURST | HDMI_SPDIFIN_CFG_DATA_ALIGN_32;

	nx_hdmi_set_reg(0, HDMI_LINK_SPDIFIN_CONFIG_1, val);
	nx_hdmi_set_reg(0, HDMI_LINK_SPDIFIN_CONFIG_2, 0);

	bps = audio_codec == HDMI_AUDIO_PCM ? bits_per_sample : 16;
	rep_time = audio_codec == HDMI_AUDIO_AC3 ? 1536 * 2 - 1 : 0;
	val = HDMI_SPDIFIN_USER_VAL_REPETITION_TIME_LOW(rep_time) |
	    HDMI_SPDIFIN_USER_VAL_WORD_LENGTH_24;
	nx_hdmi_set_reg(0, HDMI_LINK_SPDIFIN_USER_VALUE_1, val);
	val = HDMI_SPDIFIN_USER_VAL_REPETITION_TIME_HIGH(rep_time);
	nx_hdmi_set_reg(0, HDMI_LINK_SPDIFIN_USER_VALUE_2, val);
	nx_hdmi_set_reg(0, HDMI_LINK_SPDIFIN_USER_VALUE_3, 0);
	nx_hdmi_set_reg(0, HDMI_LINK_SPDIFIN_USER_VALUE_4, 0);

	val = HDMI_I2S_IN_ENABLE | HDMI_I2S_AUD_SPDIF | HDMI_I2S_MUX_ENABLE;
	nx_hdmi_set_reg(0, HDMI_LINK_I2S_MUX_CON, val);

	nx_hdmi_set_reg(0, HDMI_LINK_I2S_MUX_CH, HDMI_I2S_CH_ALL_EN);
	nx_hdmi_set_reg(0, HDMI_LINK_I2S_MUX_CUV, HDMI_I2S_CUV_RL_EN);

	nx_hdmi_write_mask(0, HDMI_LINK_SPDIFIN_CLK_CTRL, 0, HDMI_SPDIFIN_CLK_ON);
	nx_hdmi_write_mask(0, HDMI_LINK_SPDIFIN_CLK_CTRL, ~0, HDMI_SPDIFIN_CLK_ON);

	nx_hdmi_set_reg(0, HDMI_LINK_SPDIFIN_OP_CTRL, HDMI_SPDIFIN_STATUS_CHECK_MODE);
	nx_hdmi_set_reg(0, HDMI_LINK_SPDIFIN_OP_CTRL, HDMI_SPDIFIN_STATUS_CHECK_MODE_HDMI);
}

static void hdmi_audio_init(bool dvi_mode)
{
	u32 sample_rate, bits_per_sample;
	u32 audio_codec;

	sample_rate = DEFAULT_SAMPLE_RATE;
	bits_per_sample = DEFAULT_BITS_PER_SAMPLE;
	audio_codec = DEFAULT_AUDIO_CODEC;

	hdmi_set_acr(sample_rate, dvi_mode);
	hdmi_spdif_init(audio_codec, bits_per_sample);
}

static void hdmi_dvi_mode_set(bool dvi_mode)
{
	u32 val;

	debug("%s %s\n",
		 __func__, dvi_mode ? "dvi monitor" : "hdmi monitor");

	nx_hdmi_write_mask(0, HDMI_LINK_MODE_SEL, dvi_mode ? HDMI_MODE_DVI_EN :
			HDMI_MODE_HDMI_EN, HDMI_MODE_MASK);

	if (dvi_mode)
		val = HDMI_VID_PREAMBLE_DIS | HDMI_GUARD_BAND_DIS;
	else
		val = HDMI_VID_PREAMBLE_EN | HDMI_GUARD_BAND_EN;

	nx_hdmi_set_reg(0, HDMI_LINK_HDMI_CON_2, val);
}

void hdmi_enable(int input, int preset, struct dp_sync_info *sync,
			int enable, struct dp_ctrl_info *ctrl)
{
	if (enable) {
		u8 vic = 0;

		hdmi_get_vsync(preset, sync, ctrl, &vic);
		hdmi_prepare(sync);
		hdmi_infoframe_set(sync, &vic);

		hdmi_audio_init(false);
		hdmi_audio_enable(true);
		hdmi_dvi_mode_set(false);

		nx_hdmi_set_reg(0, HDMI_LINK_HDMI_CON_0,
				(nx_hdmi_get_reg(0, HDMI_LINK_HDMI_CON_0) |
				 0x1));
		hdmi_vsync(sync);
	} else {
		hdmi_phy_enable(preset, 0);
	}
}

static int hdmi_setup(int input, int preset,
		      struct dp_sync_info *sync, struct dp_ctrl_info *ctrl)
{
	u32 HDMI_SEL = 0;
	int ret;

	switch (input) {
	case DP_DEVICE_DP0:
		HDMI_SEL = primary_mlc;
		break;
	case DP_DEVICE_DP1:
		HDMI_SEL = secondary_mlc;
		break;
	case DP_DEVICE_RESCONV:
		HDMI_SEL = resolution_conv;
		break;
	default:
		printf("HDMI: not support source device %d\n", input);
		return -EINVAL;
	}

	/**
	 * [SEQ 5] set up the HDMI PHY to specific video clock.
	 */
	ret = hdmi_phy_enable(preset, 1);
	if (0 > ret)
		return ret;

	/**
	 * [SEQ 6] I2S (or SPDIFTX) configuration for the source audio data
	 * this is done in another user app  - ex> Android Audio HAL
	 */

	/**
	 * [SEQ 7] Wait for ECID ready
	 */

	/**
	 * [SEQ 8] release the resets of HDMI.i_VIDEO_nRST and HDMI.i_SPDIF_nRST
	 * and HDMI.i_TMDS_nRST
	 */
	hdmi_reset();

	/**
	 * [SEQ 9] Wait for HDMI PHY ready (wait until 0xC0200020.[0], 1)
	 */
	if (0 == hdmi_wait_phy_ready()) {
		printf("%s: failed to wait for hdmiphy ready\n", __func__);
		hdmi_phy_enable(preset, 0);
		return -EIO;
	}
	/* set mux */
	nx_disp_top_set_hdmimux(1, HDMI_SEL);

	/**
	 * [SEC 10] Set the DPC CLKGEN's Source Clock to HDMI_CLK &
	 * Set Sync Parameter
	 */
	hdmi_clock();
	/* set hdmi link clk to clkgen  vs default is hdmi phy clk */

	/**
	 * [SEQ 11] Set up the HDMI Converter parameters
	 */
	hdmi_get_vsync(preset, sync, ctrl, NULL);
	hdmi_prepare(sync);

	return 0;
}

void nx_hdmi_display(int module,
		     struct dp_sync_info *sync, struct dp_ctrl_info *ctrl,
		     struct dp_plane_top *top, struct dp_plane_info *planes,
		     struct dp_hdmi_dev *dev)
{
	struct dp_plane_info *plane = planes;
	int input = module == 0 ? DP_DEVICE_DP0 : DP_DEVICE_DP1;
	int count = top->plane_num;
	int preset = dev->preset;
	int i = 0;
#ifndef QUICKBOOT
	printf("HDMI: display.%d\n", module);
#endif

	switch (preset) {
	case 0:
		top->screen_width = 1280;
		top->screen_height = 720;
		sync->h_active_len = 1280;
		sync->v_active_len = 720;
		break;
	case 1:
		top->screen_width = 1920;
		top->screen_height = 1080;
		sync->h_active_len = 1920;
		sync->v_active_len = 1080;
		break;
	case 2:
		top->screen_width = 1920;
		top->screen_height = 720;
		sync->h_active_len = 1920;
		sync->v_active_len = 720;
		break;
	default:
		printf("hdmi not support preset %d\n", preset);
		return;
	}
#ifndef QUICKBOOT
	printf("HDMI: display.%d, preset %d (%4d * %4d)\n",
	       module, preset, top->screen_width, top->screen_height);
#endif

	dp_control_init(module);
	dp_plane_init(module);

	hdmi_init();
	hdmi_setup(input, preset, sync, ctrl);

	dp_plane_screen_setup(module, top);
	for (i = 0; count > i; i++, plane++) {
		if (!plane->enable)
			continue;
		dp_plane_layer_setup(module, plane);
		dp_plane_layer_enable(module, plane, 1);
	}
	dp_plane_screen_enable(module, 1);

	dp_control_setup(module, sync, ctrl);
	dp_control_enable(module, 1);

	hdmi_enable(input, preset, sync, 1, ctrl);
}
