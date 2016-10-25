/*
 * Xylon logiCVC IP core parameters
 *
 * Copyright (C) 2016 Xylon d.o.o.
 * Author: Davor Joja <davor.joja@logicbricks.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __LOGICVC_H__
#define __LOGICVC_H__

/*
 * All logiCVC registers are only 32-bit accessible.
 * All logiCVC registers are aligned to 8 byte boundary.
 */
#define LOGICVC_REG_STRIDE		8
#define LOGICVC_HSYNC_FRONT_PORCH_ROFF	(0  * LOGICVC_REG_STRIDE)
#define LOGICVC_HSYNC_ROFF		(1  * LOGICVC_REG_STRIDE)
#define LOGICVC_HSYNC_BACK_PORCH_ROFF	(2  * LOGICVC_REG_STRIDE)
#define LOGICVC_HRES_ROFF		(3  * LOGICVC_REG_STRIDE)
#define LOGICVC_VSYNC_FRONT_PORCH_ROFF	(4  * LOGICVC_REG_STRIDE)
#define LOGICVC_VSYNC_ROFF		(5  * LOGICVC_REG_STRIDE)
#define LOGICVC_VSYNC_BACK_PORCH_ROFF	(6  * LOGICVC_REG_STRIDE)
#define LOGICVC_VRES_ROFF		(7  * LOGICVC_REG_STRIDE)
#define LOGICVC_CTRL_ROFF		(8  * LOGICVC_REG_STRIDE)
#define LOGICVC_DTYPE_ROFF		(9  * LOGICVC_REG_STRIDE)
#define LOGICVC_BACKGROUND_COLOR_ROFF	(10 * LOGICVC_REG_STRIDE)
#define LOGICVC_CLUT_SELECT_ROFF	(12 * LOGICVC_REG_STRIDE)
#define LOGICVC_INT_STAT_ROFF		(13 * LOGICVC_REG_STRIDE)
#define LOGICVC_INT_MASK_ROFF		(14 * LOGICVC_REG_STRIDE)
#define LOGICVC_POWER_CTRL_ROFF		(15 * LOGICVC_REG_STRIDE)
#define LOGICVC_IP_VERSION_ROFF		(31 * LOGICVC_REG_STRIDE)
/* Version 3.x */
#define LOGICVC_VBUFF_SELECT_ROFF	(11 * LOGICVC_REG_STRIDE)

/*
 * logiCVC layer registers offsets (common for each layer)
 * Last possible logiCVC layer (No.4) implements only "Layer memory address"
 * and "Layer control" registers.
 */
/* Version 3.x */
#define LOGICVC_LAYER_HOFF_ROFF		(0 * LOGICVC_REG_STRIDE)
#define LOGICVC_LAYER_VOFF_ROFF		(1 * LOGICVC_REG_STRIDE)
/* Version 4.x */
#define LOGICVC_LAYER_ADDR_ROFF		(0 * LOGICVC_REG_STRIDE)
/* Common */
#define LOGICVC_LAYER_HPOS_ROFF		(2 * LOGICVC_REG_STRIDE)
#define LOGICVC_LAYER_VPOS_ROFF		(3 * LOGICVC_REG_STRIDE)
#define LOGICVC_LAYER_HSIZE_ROFF	(4 * LOGICVC_REG_STRIDE)
#define LOGICVC_LAYER_VSIZE_ROFF	(5 * LOGICVC_REG_STRIDE)
#define LOGICVC_LAYER_ALPHA_ROFF	(6 * LOGICVC_REG_STRIDE)
#define LOGICVC_LAYER_CTRL_ROFF		(7 * LOGICVC_REG_STRIDE)
#define LOGICVC_LAYER_TRANSP_COLOR_ROFF	(8 * LOGICVC_REG_STRIDE)

/* logiCVC interrupt bits */
#define LOGICVC_INT_L0_UPDATED		(1 << 0)
#define LOGICVC_INT_L1_UPDATED		(1 << 1)
#define LOGICVC_INT_L2_UPDATED		(1 << 2)
#define LOGICVC_INT_L3_UPDATED		(1 << 3)
#define LOGICVC_INT_L4_UPDATED		(1 << 4)
#define LOGICVC_INT_V_SYNC		(1 << 5)
#define LOGICVC_INT_E_VIDEO_VALID	(1 << 6)
#define LOGICVC_INT_FIFO_UNDERRUN	(1 << 7)
#define LOGICVC_INT_L0_CLUT_SW		(1 << 8)
#define LOGICVC_INT_L1_CLUT_SW		(1 << 9)
#define LOGICVC_INT_L2_CLUT_SW		(1 << 10)
#define LOGICVC_INT_L3_CLUT_SW		(1 << 11)
#define LOGICVC_INT_L4_CLUT_SW		(1 << 12)

/* logiCVC layer base offsets */
#define LOGICVC_LAYER_OFFSET		0x80
#define LOGICVC_LAYER_BASE_OFFSET	0x100
#define LOGICVC_LAYER_BASE_END		0x338
#define LOGICVC_LAYER_0_OFFSET		(0 * LOGICVC_LAYER_OFFSET)
#define LOGICVC_LAYER_1_OFFSET		(1 * LOGICVC_LAYER_OFFSET)
#define LOGICVC_LAYER_2_OFFSET		(2 * LOGICVC_LAYER_OFFSET)
#define LOGICVC_LAYER_3_OFFSET		(3 * LOGICVC_LAYER_OFFSET)
#define LOGICVC_LAYER_4_OFFSET		(4 * LOGICVC_LAYER_OFFSET)

/* logiCVC layer CLUT base offsets */
#define LOGICVC_CLUT_OFFSET		0x800
#define LOGICVC_CLUT_BASE_OFFSET	0x1000
#define LOGICVC_CLUT_L0_CLUT_0_OFFSET	(0 * LOGICVC_CLUT_OFFSET)
#define LOGICVC_CLUT_L0_CLUT_1_OFFSET	(1 * LOGICVC_CLUT_OFFSET)
#define LOGICVC_CLUT_L1_CLUT_0_OFFSET	(2 * LOGICVC_CLUT_OFFSET)
#define LOGICVC_CLUT_L1_CLUT_1_OFFSET	(3 * LOGICVC_CLUT_OFFSET)
#define LOGICVC_CLUT_L2_CLUT_0_OFFSET	(4 * LOGICVC_CLUT_OFFSET)
#define LOGICVC_CLUT_L2_CLUT_1_OFFSET	(5 * LOGICVC_CLUT_OFFSET)
#define LOGICVC_CLUT_L3_CLUT_0_OFFSET	(6 * LOGICVC_CLUT_OFFSET)
#define LOGICVC_CLUT_L3_CLUT_1_OFFSET	(7 * LOGICVC_CLUT_OFFSET)
#define LOGICVC_CLUT_L4_CLUT_0_OFFSET	(8 * LOGICVC_CLUT_OFFSET)
#define LOGICVC_CLUT_L4_CLUT_1_OFFSET	(9 * LOGICVC_CLUT_OFFSET)
#define LOGICVC_CLUT_REGISTER_SIZE	8
#define LOGICVC_CLUT_0_INDEX_OFFSET	2
#define LOGICVC_CLUT_1_INDEX_OFFSET	1

/* logiCVC registers range */
#define LOGICVC_REGISTERS_RANGE		0x6000
#define LOGICVC_LAYER_REGISTERS_RANGE	0x80

/* logiCVC control register bits */
#define LOGICVC_CTRL_HSYNC			(1 << 0)
#define LOGICVC_CTRL_HSYNC_INVERT		(1 << 1)
#define LOGICVC_CTRL_VSYNC			(1 << 2)
#define LOGICVC_CTRL_VSYNC_INVERT		(1 << 3)
#define LOGICVC_CTRL_DATA_ENABLE		(1 << 4)
#define LOGICVC_CTRL_DATA_ENABLE_INVERT		(1 << 5)
#define LOGICVC_CTRL_PIXEL_DATA_INVERT		(1 << 7)
#define LOGICVC_CTRL_PIXEL_DATA_TRIGGER_INVERT	(1 << 8)
#define LOGICVC_CTRL_DISABLE_LAYER_UPDATE	(1 << 9)

/* logiCVC layer control register bits */
#define LOGICVC_LAYER_CTRL_ENABLE			(1 << 0)
#define LOGICVC_LAYER_CTRL_COLOR_TRANSPARENCY_DISABLE	(1 << 1)
#define LOGICVC_LAYER_CTRL_EXTERNAL_BUFFER_SWITCH	(1 << 2)
#define LOGICVC_LAYER_CTRL_INTERLACE		(1 << 3)
#define LOGICVC_LAYER_CTRL_PIXEL_FORMAT_ABGR	(1 << 4)

/* logiCVC register initial values */
#define LOGICVC_DTYPE_REG_INIT	0

/* logiCVC display power signals */
#define LOGICVC_EN_BLIGHT_MSK	(1 << 0)
#define LOGICVC_EN_VDD_MSK	(1 << 1)
#define LOGICVC_EN_VEE_MSK	(1 << 2)
#define LOGICVC_V_EN_MSK	(1 << 3)

/* logiCVC various definitions */
#define LOGICVC_MAJOR_REVISION_SHIFT	11
#define LOGICVC_MAJOR_REVISION_MASK	0x3F
#define LOGICVC_MINOR_REVISION_SHIFT	5
#define LOGICVC_MINOR_REVISION_MASK	0x3F
#define LOGICVC_PATCH_LEVEL_MASK	0x1F

#define LOGICVC_MAX_LAYER_BUFFERS	3
#define LOGICVC_MIN_XRES		64
#define LOGICVC_MIN_VRES		1
#define LOGICVC_CLUT_SIZE		256

#define LOGICVC_COEFF_Y_R		29900
#define LOGICVC_COEFF_Y_G		58700
#define LOGICVC_COEFF_Y_B		11400
#define LOGICVC_COEFF_Y			0
#define LOGICVC_COEFF_U			12800000
#define LOGICVC_COEFF_V			12800000
#define LOGICVC_COEFF_U_R		16868
#define LOGICVC_COEFF_U_G		33107
#define LOGICVC_COEFF_U_B		49970
#define LOGICVC_COEFF_V_R		49980
#define LOGICVC_COEFF_V_G		41850
#define LOGICVC_COEFF_V_B		8128
#define LOGICVC_COEFF_ITU656_Y		1600000
#define LOGICVC_COEFF_ITU656_U_R	17258
#define LOGICVC_COEFF_ITU656_U_G	33881
#define LOGICVC_COEFF_ITU656_U_B	51140
#define LOGICVC_COEFF_ITU656_V_R	51138
#define LOGICVC_COEFF_ITU656_V_G	42820
#define LOGICVC_COEFF_ITU656_V_B	8316
#define LOGICVC_YUV_NORM		100000
#define LOGICVC_COEFF_R_U		1402524
#define LOGICVC_COEFF_R			179000000
#define LOGICVC_COEFF_G_U		714403
#define LOGICVC_COEFF_G_V		344340
#define LOGICVC_COEFF_G			135000000
#define LOGICVC_COEFF_B_V		1773049
#define LOGICVC_COEFF_B			226000000
#define LOGICVC_RGB_NORM		1000000

enum xylonfb_layer_type {
	LOGICVC_LAYER_RGB,
	LOGICVC_LAYER_YUV,
	LOGICVC_LAYER_ALPHA
};

enum xylonfb_alpha_type {
	LOGICVC_ALPHA_LAYER,
	LOGICVC_ALPHA_PIXEL,
	LOGICVC_ALPHA_CLUT_16BPP,
	LOGICVC_ALPHA_CLUT_32BPP
};

#endif /* __LOGICVC_H__ */
