/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Samsung EXYNOS8 SoC series DPP driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/exynos_iovmm.h>
#include <linux/videodev2_exynos_media.h>
#include <linux/console.h>

#include "dpp.h"
#include "decon.h"

int dpp_log_level = 6;

#if defined(DMA_BIST)
u32 pattern_data[] = {
	0xffffffff,
	0xffffffff,
	0xffffffff,
	0xffffffff,
	0x000000ff,
	0x000000ff,
	0x000000ff,
	0x000000ff,
};
#endif

struct dpp_device *dpp_drvdata[MAX_DPP_CNT];

static int dpp_runtime_suspend(struct device *dev);
static int dpp_runtime_resume(struct device *dev);

static bool checked;

static void dma_dump_regs(struct dpp_device *dpp)
{
	dpp_info("\n=== DPU_DMA%d SFR DUMP ===\n", dpp->id);
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
			dpp->res.dma_regs, 0x6C, false);
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
			dpp->res.dma_regs + 0x100, 0x8, false);

	dpp_info("=== DPU_DMA%d SHADOW SFR DUMP ===\n", dpp->id);
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
			dpp->res.dma_regs + 0x800, 0x74, false);
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
			dpp->res.dma_regs + 0x900, 0x8, false);
}

static void dpp_dump_regs(struct dpp_device *dpp)
{
	dpp_info("=== DPP%d SFR DUMP ===\n", dpp->id);

	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
			dpp->res.regs, 0x4C, false);
	if (dpp->id == IDMA_VGF0 || dpp->id == IDMA_VGF1) {
		print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
				dpp->res.regs + 0x5B0, 0x10, false);
	}
	if (dpp->id == IDMA_VGF1) {
		print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
			dpp->res.regs + 0x600, 0x1E0, false);
	}
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
			dpp->res.regs + 0xA54, 0x4, false);
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
			dpp->res.regs + 0xB00, 0x4C, false);
	if (dpp->id == IDMA_VGF0 || dpp->id == IDMA_VGF1) {
		print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
				dpp->res.regs + 0xBB0, 0x10, false);
	}
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 32, 4,
			dpp->res.regs + 0xD00, 0xC, false);
}

static void __dpp_dump_ch_data(int id, enum dpp_reg_area reg_area,
		u32 sel[], u32 cnt)
{
	unsigned char linebuf[128] = {0, };
	int i, ret;
	int len = 0;
	u32 data;

	for (i = 0; i < cnt; i++) {
		if (!(i % 4) && i != 0) {
			linebuf[len] = '\0';
			len = 0;
			dpp_info("%s\n", linebuf);
		}

		if (reg_area == REG_AREA_DPP) {
			dpp_write(id, 0xC04, sel[i]);
			data = dpp_read(id, 0xC10);
		} else if (reg_area == REG_AREA_DMA) {
			dma_write(id, IDMA_DEBUG_CONTROL,
					IDMA_DEBUG_CONTROL_SEL(sel[i]) |
					IDMA_DEBUG_CONTROL_EN);
			data = dma_read(id, IDMA_DEBUG_DATA);
		} else { /* REG_AREA_DMA_COM */
			dma_com_write(0, DPU_DMA_DEBUG_CONTROL,
					DPU_DMA_DEBUG_CONTROL_SEL(sel[i]) |
					DPU_DMA_DEBUG_CONTROL_EN);
			data = dma_com_read(0, DPU_DMA_DEBUG_DATA);
		}

		ret = snprintf(linebuf + len, sizeof(linebuf) - len,
				"[0x%08x: %08x] ", sel[i], data);
		if (ret >= sizeof(linebuf) - len) {
			dpp_err("overflow: %d %ld %d\n",
					ret, sizeof(linebuf), len);
			return;
		}
		len += ret;
	}
	dpp_info("%s\n", linebuf);
}

static void dma_com_dump_debug_regs(int id)
{
	u32 sel[12] = {0x0000, 0x0100, 0x0200, 0x0204, 0x0205, 0x0300, 0x4000,
		0x4001, 0x4005, 0x8000, 0x8001, 0x8005};

	if (checked)
		return;

	dpp_info("-< DMA COMMON DEBUG SFR >-\n");
	__dpp_dump_ch_data(id, REG_AREA_DMA_COM, sel, 12);

	checked = true;
}

static void dma_dump_debug_regs(int id)
{
	u32 sel_g[11] = {
		0x0000, 0x0001, 0x0002, 0x0004, 0x000A, 0x000B, 0x0400, 0x0401,
		0x0402, 0x0405, 0x0406
	};
	u32 sel_v[39] = {
		0x1000, 0x1001, 0x1002, 0x1004, 0x100A, 0x100B, 0x1400, 0x1401,
		0x1402, 0x1405, 0x1406, 0x2000, 0x2001, 0x2002, 0x2004, 0x200A,
		0x200B, 0x2400, 0x2401, 0x2402, 0x2405, 0x2406, 0x3000, 0x3001,
		0x3002, 0x3004, 0x300A, 0x300B, 0x3400, 0x3401, 0x3402, 0x3405,
		0x3406, 0x4002, 0x4003, 0x4004, 0x4005, 0x4006, 0x4007
	};
	u32 sel_f[12] = {
		0x5100, 0x5101, 0x5104, 0x5105, 0x5200, 0x5202, 0x5204, 0x5205,
		0x5300, 0x5302, 0x5303, 0x5306
	};
	u32 sel_r[22] = {
		0x6100, 0x6101, 0x6102, 0x6103, 0x6104, 0x6105, 0x6200, 0x6201,
		0x6202, 0x6203, 0x6204, 0x6205, 0x6300, 0x6301, 0x6302, 0x6306,
		0x6307, 0x6400, 0x6401, 0x6402, 0x6406, 0x6407
	};
	u32 sel_com[4] = {
		0x7000, 0x7001, 0x7002, 0x7003
	};

	dpp_info("-< DPU_DMA%d DEBUG SFR >-\n", id);
	switch (id) {
	case IDMA_G0:
	case IDMA_G1:
		__dpp_dump_ch_data(id, REG_AREA_DMA, sel_g, 11);
		__dpp_dump_ch_data(id, REG_AREA_DMA, sel_com, 4);
		break;
	case IDMA_VG0:
	case IDMA_VG1:
		__dpp_dump_ch_data(id, REG_AREA_DMA, sel_g, 11);
		__dpp_dump_ch_data(id, REG_AREA_DMA, sel_v, 39);
		__dpp_dump_ch_data(id, REG_AREA_DMA, sel_com, 4);
		break;
	case IDMA_VGF0:
		__dpp_dump_ch_data(id, REG_AREA_DMA, sel_g, 11);
		__dpp_dump_ch_data(id, REG_AREA_DMA, sel_v, 39);
		__dpp_dump_ch_data(id, REG_AREA_DMA, sel_f, 12);
		__dpp_dump_ch_data(id, REG_AREA_DMA, sel_com, 4);
		break;
	case IDMA_VGF1:
		__dpp_dump_ch_data(id, REG_AREA_DMA, sel_g, 11);
		__dpp_dump_ch_data(id, REG_AREA_DMA, sel_v, 39);
		__dpp_dump_ch_data(id, REG_AREA_DMA, sel_f, 12);
		__dpp_dump_ch_data(id, REG_AREA_DMA, sel_r, 22);
		__dpp_dump_ch_data(id, REG_AREA_DMA, sel_com, 4);
		break;
	default:
		dpp_err("DPP%d is wrong ID\n", id);
		return;
	}
}

static void dpp_dump_debug_regs(int id)
{
	u32 sel_g[3] = {0x0000, 0x0100, 0x0101};
	u32 sel_vg[19] = {0x0000, 0x0100, 0x0101, 0x0200, 0x0201, 0x0202,
		0x0203, 0x0204, 0x0205, 0x0206, 0x0207, 0x0208, 0x0300, 0x0301,
		0x0302, 0x0303, 0x0304, 0x0400, 0x0401};
	u32 sel_vgf[37] = {0x0000, 0x0100, 0x0101, 0x0200, 0x0201, 0x0210,
		0x0211, 0x0220, 0x0221, 0x0230, 0x0231, 0x0240, 0x0241, 0x0250,
		0x0251, 0x0300, 0x0301, 0x0302, 0x0303, 0x0304, 0x0305, 0x0306,
		0x0307, 0x0308, 0x0400, 0x0401, 0x0402, 0x0403, 0x0404, 0x0500,
		0x0501, 0x0502, 0x0503, 0x0504, 0x0505, 0x0600, 0x0601};
	u32 cnt;
	u32 *sel = NULL;

	switch (id) {
	case IDMA_G0:
	case IDMA_G1:
		sel = sel_g;
		cnt = 3;
		break;
	case IDMA_VG0:
	case IDMA_VG1:
		sel = sel_vg;
		cnt = 19;
		break;
	case IDMA_VGF0:
	case IDMA_VGF1:
		sel = sel_vgf;
		cnt = 37;
		break;
	default:
		dpp_err("DPP%d is wrong ID\n", id);
		return;
	}

	dpp_write(id, 0x0C00, 0x1);
	dpp_info("-< DPP%d DEBUG SFR >-\n", id);
	__dpp_dump_ch_data(id, REG_AREA_DPP, sel, cnt);
}

void dpp_dump(struct dpp_device *dpp)
{
	int acquired = console_trylock();

	dma_com_dump_debug_regs(dpp->id);

	dma_dump_regs(dpp);
	dma_dump_debug_regs(dpp->id);

	dpp_dump_regs(dpp);
	dpp_dump_debug_regs(dpp->id);

	if (acquired)
		console_unlock();
}

void dpp_op_timer_handler(unsigned long arg)
{
	struct dpp_device *dpp = (struct dpp_device *)arg;

	dpp_dump(dpp);

	if (dpp->config->compression)
		dpp_info("Compression Source is %s of DPP[%d]\n",
			dpp->config->dpp_parm.comp_src == DPP_COMP_SRC_G2D ?
			"G2D" : "GPU", dpp->id);

	dpp_info("DPP[%d] irq hasn't been occured", dpp->id);
}

static void dpp_get_params(struct dpp_device *dpp, struct dpp_params_info *p)
{
	u64 src_w, src_h, dst_w, dst_h;
	struct decon_win_config *config = dpp->config;

	memcpy(&p->src, &config->src, sizeof(struct decon_frame));
	memcpy(&p->dst, &config->dst, sizeof(struct decon_frame));
	memcpy(&p->block, &config->block_area, sizeof(struct decon_win_rect));
	p->rot = config->dpp_parm.rot;
	p->is_comp = config->compression;
	p->format = config->format;
	p->addr[0] = config->dpp_parm.addr[0];
	p->addr[1] = config->dpp_parm.addr[1];
	p->addr[2] = 0;
	p->addr[3] = 0;
	p->eq_mode = config->dpp_parm.eq_mode;
	p->hdr = config->dpp_parm.hdr_std;
	p->is_4p = false;
	p->y_2b_strd = 0;
	p->c_2b_strd = 0;

	if (p->format == DECON_PIXEL_FORMAT_NV12N) {
		p->addr[1] = NV12N_CBCR_BASE(p->addr[0], p->src.f_w, p->src.f_h);
	}

	if (p->format == DECON_PIXEL_FORMAT_NV12M_S10B || p->format == DECON_PIXEL_FORMAT_NV21M_S10B) {
		p->addr[2] = p->addr[0] + NV12M_Y_SIZE(p->src.f_w, p->src.f_h);
		p->addr[3] = p->addr[1] + NV12M_CBCR_SIZE(p->src.f_w, p->src.f_h);
		p->is_4p = true;
		p->y_2b_strd = S10B_2B_STRIDE(p->src.f_w);
		p->c_2b_strd = S10B_2B_STRIDE(p->src.f_w);
	}

	if (p->format == DECON_PIXEL_FORMAT_NV12N_10B) {
		p->addr[1] = NV12N_10B_CBCR_BASE(p->addr[0], p->src.f_w, p->src.f_h);
		p->addr[2] = p->addr[0] + NV12N_10B_Y_8B_SIZE(p->src.f_w, p->src.f_h);
		p->addr[3] = p->addr[1] + NV12N_10B_CBCR_8B_SIZE(p->src.f_w, p->src.f_h);
		p->is_4p = true;
		p->y_2b_strd = S10B_2B_STRIDE(p->src.f_w);
		p->c_2b_strd = S10B_2B_STRIDE(p->src.f_w);
	}

	if (is_rotation(config)) {
		src_w = p->src.h;
		src_h = p->src.w;
	} else {
		src_w = p->src.w;
		src_h = p->src.h;
	}
	dst_w = p->dst.w;
	dst_h = p->dst.h;

	p->h_ratio = (src_w << 20) / dst_w;
	p->v_ratio = (src_h << 20) / dst_h;

	if ((p->h_ratio != (1 << 20)) || (p->v_ratio != (1 << 20)))
		p->is_scale = true;
	else
		p->is_scale = false;

	if ((config->dpp_parm.rot != DPP_ROT_NORMAL) || (p->is_scale) ||
		(p->format >= DECON_PIXEL_FORMAT_NV16) ||
		(p->block.w < BLK_WIDTH_MIN) || (p->block.h < BLK_HEIGHT_MIN))
		p->is_block = false;
	else
		p->is_block = true;
}

static int dpp_check_size(struct dpp_device *dpp, struct dpp_img_format *vi)
{
	struct decon_win_config *config = dpp->config;
	struct decon_frame *src = &config->src;
	struct decon_frame *dst = &config->dst;
	struct dpp_size_constraints vc;

	dpp_constraints_params(&vc, vi);

	if ((!check_align(src->x, src->y, vc.src_mul_x, vc.src_mul_y)) ||
	   (!check_align(src->f_w, src->f_h, vc.src_mul_w, vc.src_mul_h)) ||
	   (!check_align(src->w, src->h, vc.img_mul_w, vc.img_mul_h)) ||
	   (!check_align(dst->w, dst->h, vc.sca_mul_w, vc.sca_mul_h))) {
		dpp_err("Alignment error!\n");
		goto err;
	}

	if (src->w > vc.img_w_max || src->w < vc.img_w_min ||
		src->h > vc.img_h_max || src->h < vc.img_h_min) {
		dpp_err("Unsupported SRC size!\n");
		goto err;
	}

	if (dst->w > vc.sca_w_max || dst->w < vc.sca_w_min ||
		dst->h > vc.sca_h_max || dst->h < vc.sca_h_min) {
		dpp_err("Unsupported DST size!\n");
		goto err;
	}

	/* check boundary */
	if (src->x + src->w > vc.src_w_max || src->y + src->h > vc.src_h_max) {
		dpp_err("Unsupported src boundary size!\n");
		goto err;
	}

	if (src->x + src->w > src->f_w || src->y + src->h > src->f_h) {
		dpp_err("Unsupported src range!\n");
		goto err;
	}

	if (src->x < 0 || src->y < 0 ||
		dst->x < 0 || dst->y < 0) {
		dpp_err("Unsupported src/dst x,y position!\n");
		goto err;
	}

	return 0;
err:
	dpp_err("offset x : %d, offset y: %d\n", src->x, src->y);
	dpp_err("src_mul_x : %d, src_mul_y : %d\n", vc.src_mul_x, vc.src_mul_y);
	dpp_err("src f_w : %d, src f_h: %d\n", src->f_w, src->f_h);
	dpp_err("src_mul_w : %d, src_mul_h : %d\n", vc.src_mul_w, vc.src_mul_h);
	dpp_err("src w : %d, src h: %d\n", src->w, src->h);
	dpp_err("img_mul_w : %d, img_mul_h : %d\n", vc.img_mul_w, vc.img_mul_h);
	dpp_err("dst w : %d, dst h: %d\n", dst->w, dst->h);
	dpp_err("sca_mul_w : %d, sca_mul_h : %d\n", vc.sca_mul_w, vc.sca_mul_h);
	dpp_err("rotation : %d, color_format : %d\n",
				config->dpp_parm.rot, config->format);
	dpp_err("hdr : %d, color_format : %d\n",
				config->dpp_parm.hdr_std, config->format);
	return -EINVAL;
}

static int dpp_check_scale_ratio(struct dpp_params_info *p)
{
	u32 sc_down_max_w, sc_down_max_h;
	u32 sc_up_min_w, sc_up_min_h;
	u32 sc_src_w, sc_src_h;

	sc_down_max_w = p->dst.w * 2;
	sc_down_max_h = p->dst.h * 2;
	sc_up_min_w = (p->dst.w + 7) / 8;
	sc_up_min_h = (p->dst.h + 7) / 8;
	if (p->rot > DPP_ROT_180) {
		sc_src_w = p->src.h;
		sc_src_h = p->src.w;
	} else {
		sc_src_w = p->src.w;
		sc_src_h = p->src.h;
	}

	if (sc_src_w > sc_down_max_w || sc_src_h > sc_down_max_h) {
		dpp_err("Not support under 1/2x scale-down!\n");
		goto err;
	}

	if (sc_src_w < sc_up_min_w || sc_src_h < sc_up_min_h) {
		dpp_err("Not support over 8x scale-up\n");
		goto err;
	}

	return 0;
err:
	dpp_err("src w(%d) h(%d), dst w(%d) h(%d), rotation(%d)\n",
			p->src.w, p->src.h, p->dst.w, p->dst.h, p->rot);
	return -EINVAL;
}

static int dpp_check_addr(struct dpp_device *dpp, struct dpp_params_info *p)
{
	int cnt = 0;

	cnt = dpu_get_plane_cnt(p->format, false);

	switch (cnt) {
	case 1:
		if (IS_ERR_OR_NULL((void *)p->addr[0])) {
			dpp_err("Address[0] is 0x0 DPP%d\n", dpp->id);
			return -EINVAL;
		}
		break;
	case 2:
	case 3:
		if (IS_ERR_OR_NULL((void *)p->addr[0])) {
			dpp_err("Address[0] is 0x0 DPP%d\n", dpp->id);
			return -EINVAL;
		}
		if (IS_ERR_OR_NULL((void *)p->addr[1])) {
			dpp_err("Address[1] is 0x0 DPP%d\n", dpp->id);
			return -EINVAL;
		}
		break;
	case 4:
		if (IS_ERR_OR_NULL((void *)p->addr[0])) {
			dpp_err("Address[0] is 0x0 DPP%d\n", dpp->id);
			return -EINVAL;
		}
		if (IS_ERR_OR_NULL((void *)p->addr[1])) {
			dpp_err("Address[1] is 0x0 DPP%d\n", dpp->id);
			return -EINVAL;
		}
		if (IS_ERR_OR_NULL((void *)p->addr[2])) {
			dpp_err("Address[2] is 0x0 DPP%d\n", dpp->id);
			return -EINVAL;
		}
		if (IS_ERR_OR_NULL((void *)p->addr[3])) {
			dpp_err("Address[3] is 0x0 DPP%d\n", dpp->id);
			return -EINVAL;
		}
		break;
	default:
		dpp_err("Unsupport plane cnt\n");
			return -EINVAL;
		break;
	}

	return 0;
}

static int dpp_check_format(struct dpp_device *dpp, struct dpp_params_info *p)
{
	if ((dpp->id != IDMA_VGF1) && (p->rot > DPP_ROT_180)) {
		dpp_err("Not support rotation in DPP%d - VGRF only!\n",
				p->rot);
		return -EINVAL;
	}

	if ((dpp->id != IDMA_VGF1) && (p->hdr > DPP_HDR_OFF)) {
		dpp_err("Not support hdr in DPP%d - VGRF only!\n",
				dpp->id);
		return -EINVAL;
	}

	if ((p->hdr < DPP_HDR_OFF) || (p->hdr > DPP_HDR_HLG)) {
		dpp_err("Unsupported HDR standard in DPP%d, HDR std(%d)\n",
				dpp->id, p->hdr);
		return -EINVAL;
	}

	if ((dpp->id == IDMA_G0 || dpp->id == IDMA_G1) &&
			(p->format >= DECON_PIXEL_FORMAT_NV16)) {
		dpp_err("Not support YUV format(%d) in DPP%d - VG & VGF only!\n",
			p->format, dpp->id);
		return -EINVAL;
	}

	if (dpp->id != IDMA_VGF0 && dpp->id != IDMA_VGF1) {
		if (p->is_comp) {
			dpp_err("Not support AFBC decoding in DPP%d - VGF only!\n",
				dpp->id);
			return -EINVAL;
		}

		if (p->is_scale) {
			dpp_err("Not support SCALING in DPP%d - VGF only!\n", dpp->id);
			return -EINVAL;
		}
	}

	switch (p->format) {
	case DECON_PIXEL_FORMAT_ARGB_8888:
	case DECON_PIXEL_FORMAT_ABGR_8888:
	case DECON_PIXEL_FORMAT_RGBA_8888:
	case DECON_PIXEL_FORMAT_BGRA_8888:
	case DECON_PIXEL_FORMAT_XRGB_8888:
	case DECON_PIXEL_FORMAT_XBGR_8888:
	case DECON_PIXEL_FORMAT_RGBX_8888:
	case DECON_PIXEL_FORMAT_BGRX_8888:
	case DECON_PIXEL_FORMAT_RGB_565:
	case DECON_PIXEL_FORMAT_NV12:
	case DECON_PIXEL_FORMAT_NV12M:
	case DECON_PIXEL_FORMAT_NV21:
	case DECON_PIXEL_FORMAT_NV21M:
	case DECON_PIXEL_FORMAT_NV12N:
	case DECON_PIXEL_FORMAT_NV12N_10B:

	case DECON_PIXEL_FORMAT_ARGB_2101010:
	case DECON_PIXEL_FORMAT_ABGR_2101010:
	case DECON_PIXEL_FORMAT_RGBA_1010102:
	case DECON_PIXEL_FORMAT_BGRA_1010102:

	case DECON_PIXEL_FORMAT_NV12M_P010:
	case DECON_PIXEL_FORMAT_NV21M_P010:
	case DECON_PIXEL_FORMAT_NV12M_S10B:
	case DECON_PIXEL_FORMAT_NV21M_S10B:
		break;
	default:
		dpp_err("Unsupported Format\n");
		return -EINVAL;
	}

	return 0;
}

/*
 * TODO: h/w limitation will be changed in KC
 * This function must be modified for KC after releasing DPP constraints
 */
static int dpp_check_limitation(struct dpp_device *dpp, struct dpp_params_info *p)
{
	int ret;
	struct dpp_img_format vi;

	ret = dpp_check_scale_ratio(p);
	if (ret) {
		dpp_err("failed to set dpp%d scale information\n", dpp->id);
		return -EINVAL;
	}

	dpp_select_format(dpp, &vi, p);

	ret = dpp_check_format(dpp, p);
	if (ret)
		return -EINVAL;

	ret = dpp_check_addr(dpp, p);
	if (ret)
		return -EINVAL;

	if (p->is_comp && p->rot) {
		dpp_err("Not support [AFBC+ROTATION] at the same time in DPP%d\n",
			dpp->id);
		return -EINVAL;
	}

	if (p->is_comp && p->is_block) {
		dpp_err("Not support [AFBC+BLOCK] at the same time in DPP%d\n",
			dpp->id);
		return -EINVAL;
	}

	if (p->is_comp && vi.yuv420) {
		dpp_err("Not support AFBC decoding for YUV format in DPP%d\n",
			dpp->id);
		return -EINVAL;
	}

	if (p->is_block && p->is_scale) {
		dpp_err("Not support [BLOCK+SCALE] at the same time in DPP%d\n",
			dpp->id);
		return -EINVAL;
	}

	if (p->is_block && vi.yuv420) {
		dpp_err("Not support BLOCK Mode for YUV format in DPP%d\n",
			dpp->id);
		return -EINVAL;
	}

	/* FIXME */
	if (p->is_block && p->rot) {
		dpp_err("Not support [BLOCK+ROTATION] at the same time in DPP%d\n",
			dpp->id);
		return -EINVAL;
	}

	/* HDR channel limitation */
	if ((p->hdr != DPP_HDR_OFF) && p->is_comp) {
		dpp_err("Not support [HDR+AFBC] at the same time in DPP%d\n",
			dpp->id);
		return -EINVAL;
	}

	/* HDR channel limitation */
	if ((p->hdr != DPP_HDR_OFF) && p->rot) {
		dpp_err("Not support [HDR+ROTATION] at the same time in DPP%d\n",
			dpp->id);
		return -EINVAL;
	}

	ret = dpp_check_size(dpp, &vi);
	if (ret)
		return -EINVAL;

	return 0;
}

static int dpp_set_config(struct dpp_device *dpp)
{
	struct dpp_params_info params;
	int ret = 0;

	mutex_lock(&dpp->lock);

	/* parameters from decon driver are translated for dpp driver */
	dpp_get_params(dpp, &params);

	/* all parameters must be passed dpp hw limitation */
	ret = dpp_check_limitation(dpp, &params);
	if (ret)
		goto err;

	if (dpp->state == DPP_STATE_OFF) {
		dpp_dbg("dpp%d is started\n", dpp->id);
		dpp_reg_init(dpp->id);

		enable_irq(dpp->res.dma_irq);
		enable_irq(dpp->res.irq);

		/* DMA_debug registers enable */
		/*
		dma_reg_set_debug(dpp->id);
		dma_reg_set_common_debug(dpp->id);
		*/
	}

	/* set all parameters to dpp hw */
	dpp_reg_configure_params(dpp->id, &params);

	dpp->d.op_timer.expires = (jiffies + 1 * HZ);
	mod_timer(&dpp->d.op_timer, dpp->d.op_timer.expires);

	DPU_EVENT_LOG(DPU_EVT_DPP_WINCON, &dpp->sd, ktime_set(0, 0));

	/* It's only for DPP BIST mode test */
#if defined(DMA_BIST)
	dma_reg_set_test_en(dpp->id, 1);
	dma_reg_set_test_pattern(0, 0, &pattern_data[0]);
	dma_reg_set_test_pattern(0, 1, &pattern_data[0]);
#endif

	dpp_dbg("dpp%d configuration\n", dpp->id);

	dpp->state = DPP_STATE_ON;
err:
	mutex_unlock(&dpp->lock);
	return ret;
}

static int dpp_stop(struct dpp_device *dpp, bool reset)
{
	int ret = 0;

	mutex_lock(&dpp->lock);

	if (dpp->state == DPP_STATE_OFF) {
		dpp_warn("dpp%d is already disabled\n", dpp->id);
		goto err;
	}

	DPU_EVENT_LOG(DPU_EVT_DPP_STOP, &dpp->sd, ktime_set(0, 0));

	disable_irq(dpp->res.dma_irq);
	disable_irq(dpp->res.irq);

	del_timer(&dpp->d.op_timer);
	dpp_reg_deinit(dpp->id, reset);

	dpp_dbg("dpp%d is stopped\n", dpp->id);

	dpp->state = DPP_STATE_OFF;
err:
	mutex_unlock(&dpp->lock);
	return ret;
}

static int dpp_s_stream(struct v4l2_subdev *sd, int enable)
{
	dpp_dbg("%s: subdev name(%s)\n", __func__, sd->name);
	return 0;
}

static long dpp_subdev_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct dpp_device *dpp = v4l2_get_subdevdata(sd);
	bool reset = (bool)arg;
	unsigned long val;
	int ret = 0;

	switch (cmd) {
	case DPP_WIN_CONFIG:
		dpp->config = (struct decon_win_config *)arg;
		ret = dpp_set_config(dpp);
		if (ret)
			dpp_err("failed to configure dpp%d\n", dpp->id);
		break;

	case DPP_STOP:
		ret = dpp_stop(dpp, reset);
		if (ret)
			dpp_err("failed to stop dpp%d\n", dpp->id);
		break;

	case DPP_DUMP:
		dpp_dump(dpp);
		break;

	case DPP_WAIT_IDLE:
		val = (unsigned long)arg;
		if (dpp->state == DPP_STATE_ON)
			dpp_reg_wait_idle_status(dpp->id, val);
		break;

	case DPP_SET_RECOVERY_NUM:
		val = (unsigned long)arg;
		dma_reg_set_recovery_num(dpp->id, (u32)val);
		break;

	default:
		break;
	}

	return ret;
}

static const struct v4l2_subdev_core_ops dpp_subdev_core_ops = {
	.ioctl = dpp_subdev_ioctl,
};

static const struct v4l2_subdev_video_ops dpp_subdev_video_ops = {
	.s_stream = dpp_s_stream,
};

static struct v4l2_subdev_ops dpp_subdev_ops = {
	.core = &dpp_subdev_core_ops,
	.video = &dpp_subdev_video_ops,
};

static void dpp_init_subdev(struct dpp_device *dpp)
{
	struct v4l2_subdev *sd = &dpp->sd;

	v4l2_subdev_init(sd, &dpp_subdev_ops);
	sd->owner = THIS_MODULE;
	sd->grp_id = dpp->id;
	snprintf(sd->name, sizeof(sd->name), "%s.%d", "dpp-sd", dpp->id);
	v4l2_set_subdevdata(sd, dpp);
}

static irqreturn_t dpp_irq_handler(int irq, void *priv)
{
	struct dpp_device *dpp = priv;
	u32 dpp_irq = 0;
	u32 cfg_err = 0;

	spin_lock(&dpp->slock);
	if (dpp->state == DPP_STATE_OFF)
		goto irq_end;

	dpp_irq = dpp_reg_get_irq_status(dpp->id);
	/* CFG_ERR_STATE SFR is cleared when clearing pending bits */
	if (dpp_irq & DPP_CONFIG_ERROR) {
		cfg_err = dpp_read(dpp->id, DPP_CFG_ERR_STATE);
		dpp_reg_clear_irq(dpp->id, dpp_irq);

		dpp_err("dpp%d config error occur(0x%x)\n",
				dpp->id, dpp_irq);
		dpp_err("DPP_CFG_ERR_STATE = (0x%x)\n", cfg_err);

		/*
		 * Disabled because this can cause slow update
		 * if conditions happen very often
		 *	dpp_dump(dpp);
		*/
		goto irq_end;
	}

	dpp_reg_clear_irq(dpp->id, dpp_irq);

irq_end:
	del_timer(&dpp->d.op_timer);
	spin_unlock(&dpp->slock);
	return IRQ_HANDLED;
}

static irqreturn_t dma_irq_handler(int irq, void *priv)
{
	struct dpp_device *dpp = priv;
	u32 irqs = 0;
	u32 reg_id = 0;
	u32 cfg_err = 0;
	u32 irq_pend = 0;
	u32 val = 0;

	spin_lock(&dpp->dma_slock);
	if (dpp->state == DPP_STATE_OFF)
		goto irq_end;

	irqs = dma_reg_get_irq_status(dpp->id);
	/* CFG_ERR_STATE SFR is cleared when clearing pending bits */
	reg_id = IDMA_CFG_ERR_STATE;
	irq_pend = IDMA_CONFIG_ERROR;

	if (irqs & irq_pend)
		cfg_err = dma_read(dpp->id, reg_id);
	dma_reg_clear_irq(dpp->id, irqs);

	if (irqs & IDMA_RECOVERY_START_IRQ) {
		DPU_EVENT_LOG(DPU_EVT_DMA_RECOVERY, &dpp->sd,
				ktime_set(0, 0));
		val = (u32)dpp->config->dpp_parm.comp_src;
		dpp->d.recovery_cnt++;
		dpp_info("dma%d recovery start(0x%x).. [src=%s], cnt[%d %d]\n",
				dpp->id, irqs,
				val == DPP_COMP_SRC_G2D ? "G2D" : "GPU",
				get_dpp_drvdata(IDMA_VGF0)->d.recovery_cnt,
				get_dpp_drvdata(IDMA_VGF1)->d.recovery_cnt);
		goto irq_end;
	}

	if ((irqs & IDMA_AFBC_TIMEOUT_IRQ) ||
			(irqs & IDMA_READ_SLAVE_ERROR) ||
			(irqs & IDMA_STATUS_DEADLOCK_IRQ)) {
		dpp_err("dma%d error irq occur(0x%x)\n", dpp->id, irqs);
		dpp_dump(dpp);
		goto irq_end;
	}

	if (irqs & IDMA_CONFIG_ERROR) {
		val = IDMA_CFG_ERR_IMG_HEIGHT
			| IDMA_CFG_ERR_IMG_HEIGHT_ROTATION;
		if (cfg_err & val)
			dpp_err("dma%d config: img_height(0x%x)\n",
					dpp->id, irqs);
		else {
			dpp_err("dma%d config error occur(0x%x)\n",
					dpp->id, irqs);
			dpp_err("CFG_ERR_STATE = (0x%x)\n", cfg_err);
			/* TODO: add to read config error information */
			/*
			 * Disabled because this can cause slow update
			 * if conditions happen very often
			 *	dpp_dump(dpp);
			 */
		}
		goto irq_end;
	}

	if (irqs & IDMA_STATUS_FRAMEDONE_IRQ) {
		/*
		 * TODO: Normally, DMA framedone occurs before
		 * DPP framedone. But DMA framedone can occur in case
		 * of AFBC crop mode
		 */
		DPU_EVENT_LOG(DPU_EVT_DMA_FRAMEDONE, &dpp->sd, ktime_set(0, 0));
		goto irq_end;
	}

irq_end:
	spin_unlock(&dpp->dma_slock);
	return IRQ_HANDLED;
}

static int dpp_get_clocks(struct dpp_device *dpp)
{
	return 0;
}

static void dpp_parse_dt(struct dpp_device *dpp, struct device *dev)
{
	dpp->id = of_alias_get_id(dev->of_node, "dpp");
	dpp_info("dpp(%d) probe start..\n", dpp->id);

	dpp->dev = dev;
}

static int dpp_init_resources(struct dpp_device *dpp, struct platform_device *pdev)
{
	struct resource *res;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dpp_err("failed to get mem resource\n");
		return -ENOENT;
	}
	dpp_info("res: start(0x%x), end(0x%x)\n",
			(u32)res->start, (u32)res->end);

	dpp->res.regs = devm_ioremap_resource(dpp->dev, res);
	if (!dpp->res.regs) {
		dpp_err("failed to remap DPP SFR region\n");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		dpp_err("failed to get mem resource\n");
		return -ENOENT;
	}
	dpp_info("dma res: start(0x%x), end(0x%x)\n",
			(u32)res->start, (u32)res->end);

	dpp->res.dma_regs = devm_ioremap_resource(dpp->dev, res);
	if (!dpp->res.dma_regs) {
		dpp_err("failed to remap DPU_DMA SFR region\n");
		return -EINVAL;
	}

	/* IDMA_G0 channel can only access common area of DPU_DMA */
	if (dpp->id == IDMA_G0) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
		if (!res) {
			dpp_err("failed to get mem resource\n");
			return -ENOENT;
		}
		dpp_info("dma common res: start(0x%x), end(0x%x)\n",
				(u32)res->start, (u32)res->end);

		dpp->res.dma_com_regs = devm_ioremap_resource(dpp->dev, res);
		if (!dpp->res.dma_com_regs) {
			dpp_err("failed to remap DPU_DMA COMMON SFR region\n");
			return -EINVAL;
		}
	}

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dpp_err("failed to get dpu dma irq resource\n");
		return -ENOENT;
	}
	dpp_info("dma irq no = %lld\n", res->start);

	dpp->res.dma_irq = res->start;
	ret = devm_request_irq(dpp->dev, res->start, dma_irq_handler, 0,
			pdev->name, dpp);
	if (ret) {
		dpp_err("failed to install DPU DMA irq\n");
		return -EINVAL;
	}
	disable_irq(dpp->res.dma_irq);

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
	if (!res) {
		dpp_err("failed to get dpp irq resource\n");
		return -ENOENT;
	}
	dpp_info("dpp irq no = %lld\n", res->start);

	dpp->res.irq = res->start;
	ret = devm_request_irq(dpp->dev, res->start, dpp_irq_handler, 0,
			pdev->name, dpp);
	if (ret) {
		dpp_err("failed to install DPP irq\n");
		return -EINVAL;
	}
	disable_irq(dpp->res.irq);

	return 0;
}

static int dpp_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dpp_device *dpp;
	int ret = 0;

	dpp = devm_kzalloc(dev, sizeof(*dpp), GFP_KERNEL);
	if (!dpp) {
		dpp_err("failed to allocate dpp device.\n");
		ret = -ENOMEM;
		goto err;
	}
	dpp_parse_dt(dpp, dev);

	dpp_drvdata[dpp->id] = dpp;
	ret = dpp_get_clocks(dpp);
	if (ret)
		goto err_clk;

	spin_lock_init(&dpp->slock);
	spin_lock_init(&dpp->dma_slock);
	mutex_init(&dpp->lock);

	ret = dpp_init_resources(dpp, pdev);
	if (ret)
		goto err_clk;

	dpp_init_subdev(dpp);
	platform_set_drvdata(pdev, dpp);
	setup_timer(&dpp->d.op_timer, dpp_op_timer_handler, (unsigned long)dpp);

	pm_runtime_enable(dev);

	dpp->state = DPP_STATE_OFF;
	dpp_info("dpp%d is probed successfully\n", dpp->id);

	return 0;

err_clk:
	kfree(dpp);
err:
	return ret;
}

static int dpp_remove(struct platform_device *pdev)
{
	struct dpp_device *dpp = platform_get_drvdata(pdev);

	iovmm_deactivate(dpp->dev);

	dpp_info("%s driver unloaded\n", pdev->name);
	return 0;
}

static int dpp_runtime_suspend(struct device *dev)
{
	struct dpp_device *dpp = dev_get_drvdata(dev);

	dpp_dbg("%s(%d) +\n", __func__, dpp->id);
	dpp_dbg("%s -\n", __func__);

	return 0;
}

static int dpp_runtime_resume(struct device *dev)
{
	int ret = 0;
	struct dpp_device *dpp = dev_get_drvdata(dev);

	dpp_dbg("%s(%d) +\n", __func__, dpp->id);
	dpp_dbg("%s -\n", __func__);

	return ret;
}

static const struct of_device_id dpp_of_match[] = {
#if defined(CONFIG_SOC_EXYNOS9810)
	{ .compatible = "samsung,exynos9-dpp" },
#else
	{ .compatible = "samsung,exynos8-dpp" },
#endif
	{},
};
MODULE_DEVICE_TABLE(of, dpp_of_match);

static const struct dev_pm_ops dpp_pm_ops = {
	.runtime_suspend	= dpp_runtime_suspend,
	.runtime_resume		= dpp_runtime_resume,
};

static struct platform_driver dpp_driver __refdata = {
	.probe		= dpp_probe,
	.remove		= dpp_remove,
	.driver = {
		.name	= DPP_MODULE_NAME,
		.owner	= THIS_MODULE,
		.pm	= &dpp_pm_ops,
		.of_match_table = of_match_ptr(dpp_of_match),
		.suppress_bind_attrs = true,
	}
};

static int dpp_register(void)
{
	return platform_driver_register(&dpp_driver);
}

device_initcall_sync(dpp_register);

MODULE_AUTHOR("Jaehoe Yang <jaehoe.yang@samsung.com>");
MODULE_AUTHOR("Minho Kim <m8891.kim@samsung.com>");
MODULE_DESCRIPTION("Samsung EXYNOS DPP driver");
MODULE_LICENSE("GPL");
