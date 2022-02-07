/*
 * Copyright (C) 2016  Nexell Co., Ltd.
 * Author: junghyun, kim <jhkim@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _S5PXX18_DP_HDMI_H_
#define _S5PXX18_DP_HDMI_H_

enum color_range {
	AVI_FULL_RANGE = 0,
	AVI_LIMITED_RANGE
};

enum hdmi_vformat {
	HDMI_VIDEO_FORMAT_2D = 0x0,
	HDMI_VIDEO_FORMAT_3D = 0x2
};

enum hdmi_3d_type {
	HDMI_3D_TYPE_FP = 0x0, /** Frame Packing */
	HDMI_3D_TYPE_TB = 0x6, /** Top-and-Bottom */
	HDMI_3D_TYPE_SB_HALF = 0x8 /** Side-by-Side Half */
};

struct hdmi_format {
	enum hdmi_vformat vformat;
	enum hdmi_3d_type type_3d;
};

/* VENDOR header */
#define HDMI_VSI_VERSION		0x01
#define HDMI_VSI_LENGTH			0x05

/* AVI header */
#define HDMI_AVI_VERSION		0x02
#define HDMI_AVI_LENGTH			0x0d
#define AVI_UNDERSCAN			(2 << 0)
#define AVI_ACTIVE_FORMAT_VALID	(1 << 4)
#define AVI_SAME_AS_PIC_ASPECT_RATIO 0x8
#define AVI_ITU709				(2 << 6)
#define AVI_LIMITED_RANGE		(1 << 2)
#define AVI_FULL_RANGE			(2 << 2)

/* AUI header info */
#define HDMI_AUI_VERSION		0x01
#define HDMI_AUI_LENGTH			0x0a

enum HDMI_3D_EXT_DATA {
	/*
	 * refer to Table H-3 3D_Ext_Data - Additional video format
	 * information for Side-by-side(half) 3D structure
	 */

	/** Horizontal sub-sampleing */
	HDMI_H_SUB_SAMPLE = 0x1
};

enum HDMI_OUTPUT_FMT {
	HDMI_OUTPUT_RGB888 = 0x0,
	HDMI_OUTPUT_YUV444 = 0x2
};

enum HDMI_AUDIO_CODEC {
	HDMI_AUDIO_PCM,
	HDMI_AUDIO_AC3,
	HDMI_AUDIO_MP3
};

#endif
