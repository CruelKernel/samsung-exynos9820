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

struct dpp_device *dpp_drvdata[MAX_DPP_CNT];

static u32 default_fmt[DEFAULT_FMT_CNT] = {
	DECON_PIXEL_FORMAT_ARGB_8888, DECON_PIXEL_FORMAT_ABGR_8888,
	DECON_PIXEL_FORMAT_RGBA_8888, DECON_PIXEL_FORMAT_BGRA_8888,
	DECON_PIXEL_FORMAT_XRGB_8888, DECON_PIXEL_FORMAT_XBGR_8888,
	DECON_PIXEL_FORMAT_RGBX_8888, DECON_PIXEL_FORMAT_BGRX_8888,
	DECON_PIXEL_FORMAT_RGB_565, DECON_PIXEL_FORMAT_BGR_565
};

void dpp_dump(struct dpp_device *dpp)
{
	int acquired = console_trylock();

	__dpp_dump(dpp->id, dpp->res.regs, dpp->res.dma_regs, dpp->attr);

	if (acquired)
		console_unlock();
}

void dpp_op_timer_handler(unsigned long arg)
{
	struct dpp_device *dpp = (struct dpp_device *)arg;

	dpp_dump(dpp);

	if (dpp->dpp_config->config.compression)
		dpp_info("Compression Source is %s of DPP[%d]\n",
			dpp->dpp_config->config.dpp_parm.comp_src == DPP_COMP_SRC_G2D ?
			"G2D" : "GPU", dpp->id);

	dpp_info("DPP[%d] irq hasn't been occured", dpp->id);
}

static int dpp_wb_wait_for_framedone(struct dpp_device *dpp)
{
	int ret;
	int done_cnt;

	if (!test_bit(DPP_ATTR_ODMA, &dpp->attr)) {
		dpp_err("waiting for dpp's framedone is only for writeback\n");
		return -EINVAL;
	}

	if (dpp->state == DPP_STATE_OFF) {
		dpp_err("dpp%d power is off state(%d)\n", dpp->id, dpp->state);
		return -EPERM;
	}

	done_cnt = dpp->d.done_count;
	/* TODO: dma framedone should be wait */
	ret = wait_event_interruptible_timeout(dpp->framedone_wq,
			(done_cnt != dpp->d.done_count), msecs_to_jiffies(17));
	if (ret == 0) {
		dpp_err("timeout of dpp%d framedone\n", dpp->id);
		return -ETIMEDOUT;
	}

	return 0;
}

static void dpp_get_params(struct dpp_device *dpp, struct dpp_params_info *p)
{
	u64 src_w, src_h, dst_w, dst_h;

	struct decon_win_config *config = &dpp->dpp_config->config;
	struct dpp_restriction *res = &dpp->restriction;

	p->rcv_num = dpp->dpp_config->rcv_num;
#ifdef CONFIG_EXYNOS_MCD_HDR
	p->wcg_mode = dpp->dpp_config->wcg_mode;
#endif
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
	p->max_luminance = config->dpp_parm.max_luminance;
	p->min_luminance = config->dpp_parm.min_luminance;
	p->is_4p = false;
	p->y_2b_strd = 0;
	p->c_2b_strd = 0;

	if (p->format == DECON_PIXEL_FORMAT_NV12N)
		p->addr[1] = NV12N_CBCR_BASE(p->addr[0], p->src.f_w, p->src.f_h);

	if (p->format == DECON_PIXEL_FORMAT_NV12_P010)
		p->addr[1] = P010_CBCR_BASE(p->addr[0], p->src.f_w, p->src.f_h);

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

	if (p->format == DECON_PIXEL_FORMAT_NV16M_S10B || p->format == DECON_PIXEL_FORMAT_NV61M_S10B) {
		p->addr[2] = p->addr[0] + NV16M_Y_SIZE(p->src.f_w, p->src.f_h);
		p->addr[3] = p->addr[1] + NV16M_CBCR_SIZE(p->src.f_w, p->src.f_h);
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
		(p->block.w < res->blk_w.min) || (p->block.h < res->blk_h.min))
		p->is_block = false;
	else
		p->is_block = true;
}

static int dpp_check_size(struct dpp_device *dpp, struct dpp_img_format *vi)
{
	struct decon_win_config *config = &dpp->dpp_config->config;
	struct decon_frame *src = &config->src;
	struct decon_frame *dst = &config->dst;
	struct dpp_size_constraints vc;

	dpp_constraints_params(&vc, vi, &dpp->restriction);

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

	cnt = dpu_get_plane_cnt(p->format, DPP_HDR_OFF);

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
	if (!test_bit(DPP_ATTR_ROT, &dpp->attr) && (p->rot > DPP_ROT_180)) {
		dpp_err("Not support rotation(%d) in DPP%d - VGRF only!\n",
				p->rot, dpp->id);
		return -EINVAL;
	}

#ifndef CONFIG_EXYNOS_MCD_HDR
	if (!test_bit(DPP_ATTR_HDR, &dpp->attr) && (p->hdr > DPP_HDR_OFF)) {
		dpp_err("Not support hdr in DPP%d - VGRF only!\n",
				dpp->id);
		return -EINVAL;
	}

	if ((p->hdr < DPP_HDR_OFF) || (p->hdr > DPP_TRANSFER_GAMMA2_8)) {
		dpp_err("Unsupported HDR standard in DPP%d, HDR std(%d)\n",
				dpp->id, p->hdr);
		return -EINVAL;
	}
#endif
	if (!test_bit(DPP_ATTR_CSC, &dpp->attr) &&
			(p->format >= DECON_PIXEL_FORMAT_NV16)) {
		dpp_err("Not support YUV format(%d) in DPP%d - VG & VGF only!\n",
			p->format, dpp->id);
		return -EINVAL;
	}

	if (!test_bit(DPP_ATTR_AFBC, &dpp->attr) && p->is_comp) {
		dpp_err("Not support AFBC decoding in DPP%d - VGF only!\n",
				dpp->id);
		return -EINVAL;
	}

	if (!test_bit(DPP_ATTR_SCALE, &dpp->attr) && p->is_scale) {
		dpp_err("Not support SCALING in DPP%d - VGF only!\n", dpp->id);
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

	ret = dpp_check_size(dpp, &vi);
	if (ret)
		return -EINVAL;

	return 0;
}

static int dpp_afbc_enabled(struct dpp_device *dpp, int *afbc_enabled)
{
	int ret = 0;

	if (test_bit(DPP_ATTR_AFBC, &dpp->attr))
		*afbc_enabled = 1;
	else
		*afbc_enabled = 0;

	return ret;
}

#ifdef CONFIG_EXYNOS_MCD_HDR
void dpp_reg_sel_hdr(u32 id, enum hdr_path path);


#define HAL_DATASPACE_V0_SRGB 		142671872 // ((STANDARD_BT709 | TRANSFER_SRGB) | RANGE_FULL)
#define HAL_DATASPACE_DCI_P3_LINEAR 139067392 // ((STANDARD_DCI_P3 | TRANSFER_LINEAR) | RANGE_FULL)


int dpp_mcd_config_hdr(struct dpp_device *dpp, struct dpp_params_info *params)
{
	u32 ret = 0;
	struct dpp_hdr10_info *hdr_info;
	struct hdr10_config config;
	struct dpp_config *dpp_cfg = dpp->dpp_config;
	unsigned int ioctl_cmd = CONFIG_HDR10;

	memset(&config, 0, sizeof(struct hdr10_config ));

	hdr_info = &dpp_cfg->hdr_info;
	config.eq_mode = params->eq_mode;
	config.hdr_mode = params->hdr;
	config.color_mode = params->wcg_mode;
	config.src_max_luminance = params->max_luminance;
	config.dst_max_luminance = dpp_cfg->hdr_info.dst_max_luminance;

	if (hdr_info->type & VIDEO_INFO_TYPE_HDR_DYNAMIC) {
		if (!(dpp->attr & (1 << DPP_ATTR_C_HDR10_PLUS))) {
			dpp_info("DPP:%s:INFO:DPP_ID: %d, HDR10+ -> HDR10\n",
				__func__, dpp->id);
			goto exit_config;
		}

		ioctl_cmd = CONFIG_HDR10P;

#ifdef HDR_DEBUG
		dpp_info("######### Support Dynamic Meta LUT Info #########\n");
		print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			&hdr_info->lut, sizeof(unsigned int) * 42, false);
#endif
		config.lut = hdr_info->lut;
	}

exit_config:
	ret = v4l2_subdev_call(dpp->mcd_sd,
			core , ioctl ,ioctl_cmd, &config);
		if (ret)
			dpp_err("DPP:ERR:%s:faild to hdr10p\n", __func__);

	return ret;
}


static int dpp_mcd_config_wcg(struct dpp_device *dpp, struct dpp_params_info *params)
{
	int ret = 0;
	struct wcg_config color_config;

#if 0
	dpp_info("dpp-%d : eq_mode : %d(%x), hdr_std : %d(%x)\n",
		dpp->id,
		params->eq_mode, params->eq_mode,
		params->hdr, params->hdr);
#endif

	memset(&color_config, 0, sizeof(struct wcg_config));

	color_config.color_mode = params->wcg_mode;
	color_config.eq_mode = params->eq_mode;
	color_config.hdr_mode = params->hdr;

	ret = v4l2_subdev_call(dpp->mcd_sd,
		core , ioctl ,CONFIG_WCG, &color_config);
	if (ret)
		dpp_err("DPP:ERR:%s:faild to config wcg\n", __func__);

	return ret;
}

static int dpp_mcd_stop(struct dpp_device *dpp)
{
	int ret = 0;
	int param = 0;

	ret = v4l2_subdev_call(dpp->mcd_sd,
		core, ioctl, STOP_MCD_IP, &param);

	return ret;
}


static int dpp_mcd_reset(struct dpp_device *dpp)
{
	int ret = 0;
	int param = 0;

	ret = v4l2_subdev_call(dpp->mcd_sd,
		core, ioctl, RESET_MCD_IP, &param);

	return ret;
}

#if 0
static int dpp_mcd_dump(struct dpp_device *dpp)
{
	int ret = 0;
	int param = 0;

	ret = v4l2_subdev_call(dpp->mcd_sd,
		core, ioctl, DUMP_MCD_IP, &param);

	return ret;
}
#endif
#endif


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
		dpp_reg_init(dpp->id, dpp->attr);

		enable_irq(dpp->res.dma_irq);
		if (test_bit(DPP_ATTR_DPP, &dpp->attr))
			enable_irq(dpp->res.irq);
	}

	/* set all parameters to dpp hw */
	dpp_reg_configure_params(dpp->id, &params, dpp->attr);

#ifdef CONFIG_EXYNOS_MCD_HDR
	dpp_mcd_reset(dpp);

	if (params.wcg_mode != HAL_COLOR_MODE_NATIVE) {
		ret = dpp_mcd_config_wcg(dpp, &params);
		if (ret)
			dpp_err("DPP:ERR:%s:faield to set mcd ip\n", __func__);
	}

	if (IS_HDR_FMT(params.hdr))
		dpp_mcd_config_hdr(dpp, &params);
#endif

	dpp->d.op_timer.expires = (jiffies + 1 * HZ);
	mod_timer(&dpp->d.op_timer, dpp->d.op_timer.expires);

	DPU_EVENT_LOG(DPU_EVT_DPP_WINCON, &dpp->sd, ktime_set(0, 0));

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
	if (test_bit(DPP_ATTR_DPP, &dpp->attr))
		disable_irq(dpp->res.irq);

	del_timer(&dpp->d.op_timer);
	dpp_reg_deinit(dpp->id, reset, dpp->attr);

	dpp_dbg("dpp%d is stopped\n", dpp->id);

	dpp->state = DPP_STATE_OFF;
err:
	mutex_unlock(&dpp->lock);
	return ret;
}

static long dpp_subdev_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct dpp_device *dpp = v4l2_get_subdevdata(sd);
	bool reset = true;
	int ret = 0;
	int *afbc_enabled;

	switch (cmd) {
	case DPP_WIN_CONFIG:
		dpp->dpp_config = (struct dpp_config *)arg;
		ret = dpp_set_config(dpp);
		if (ret)
			dpp_err("failed to configure dpp%d\n", dpp->id);
		break;

	case DPP_STOP:
		if (&arg != NULL)
			reset = (bool)arg;
#ifdef CONFIG_EXYNOS_MCD_HDR
		ret = dpp_mcd_stop(dpp);
#endif
		ret = dpp_stop(dpp, reset);
		if (ret)
			dpp_err("failed to stop dpp%d\n", dpp->id);
		break;

	case DPP_DUMP:
		dpp_dump(dpp);
		break;

	case DPP_WB_WAIT_FOR_FRAMEDONE:
		ret = dpp_wb_wait_for_framedone(dpp);
		break;

	case DPP_AFBC_ATTR_ENABLED:
		if (!arg) {
			dpp_err("failed to get afbc enabled info\n");
			ret = -EINVAL;
			break;
		}
		afbc_enabled = (int *)arg;
		ret = dpp_afbc_enabled(dpp, afbc_enabled);
		break;

	case DPP_GET_PORT_NUM:
		if (!arg) {
			dpp_err("failed to get AXI port number\n");
			ret = -EINVAL;
			break;
		}
		*(int *)arg = dpp->port;
		break;

	case DPP_GET_RESTRICTION:
		if (!arg) {
			dpp_err("failed to get dpp restriction\n");
			ret = -EINVAL;
			break;
		}
		memcpy(&(((struct dpp_ch_restriction *)arg)->restriction),
				&dpp->restriction,
				sizeof(struct dpp_restriction));
		((struct dpp_ch_restriction *)arg)->id = dpp->id;
		((struct dpp_ch_restriction *)arg)->attr = dpp->attr;
		break;

	case DPP_GET_RECOVERY_CNT:
		if (arg)
			*((int *)arg) = dpp->d.recovery_cnt;
		break;

	default:
		break;
	}

	return ret;
}

static const struct v4l2_subdev_core_ops dpp_subdev_core_ops = {
	.ioctl = dpp_subdev_ioctl,
};

static struct v4l2_subdev_ops dpp_subdev_ops = {
	.core = &dpp_subdev_core_ops,
};


#ifdef CONFIG_EXYNOS_MCD_HDR
static int __mcd_match_dev(struct device *dev, void *data)
{
	struct mcd_hdr_device *hdr;
	struct dpp_device *dpp = (struct dpp_device *)data;

	hdr = (struct mcd_hdr_device *)dev_get_drvdata(dev);
	if (hdr == NULL) {
		dpp_err("DDP:ERR:%s:NULL HDR structure\n", __func__);
		return -EINVAL;
	}

	if (dpp->id == hdr->id) {
		dpp_info("Match ID : %d\n", hdr->id);
		dpp->mcd_sd = &hdr->sd;
	}

	return 0;
}

static int dpp_get_mcd_hdr_subdev(struct dpp_device *dpp, char *devname)
{
	int ret = 0;
	struct device *dev;
	struct device_driver *drv;

	drv = driver_find(devname, &platform_bus_type);
	if (IS_ERR_OR_NULL(drv)) {
		dpp_err("DPP:ERR:%s:failed to find driver\n", __func__);
		return -ENODEV;
	}

	dev = driver_find_device(drv, NULL, dpp, __mcd_match_dev);

	return ret;
}
#endif

static void dpp_init_subdev(struct dpp_device *dpp)
{
	struct v4l2_subdev *sd = &dpp->sd;

	v4l2_subdev_init(sd, &dpp_subdev_ops);
	sd->owner = THIS_MODULE;
	sd->grp_id = dpp->id;
	snprintf(sd->name, sizeof(sd->name), "%s.%d", "dpp-sd", dpp->id);
	v4l2_set_subdevdata(sd, dpp);
}

static void dpp_parse_restriction(struct dpp_device *dpp, struct device_node *n)
{
	u32 range[3] = {0, };
	u32 align[2] = {0, };

	dpp_info("dpp restriction\n");
	of_property_read_u32_array(n, "src_f_w", range, 3);
	dpp->restriction.src_f_w.min = range[0];
	dpp->restriction.src_f_w.max = range[1];
	dpp->restriction.src_f_w.align = range[2];

	of_property_read_u32_array(n, "src_f_h", range, 3);
	dpp->restriction.src_f_h.min = range[0];
	dpp->restriction.src_f_h.max = range[1];
	dpp->restriction.src_f_h.align = range[2];

	of_property_read_u32_array(n, "src_w", range, 3);
	dpp->restriction.src_w.min = range[0];
	dpp->restriction.src_w.max = range[1];
	dpp->restriction.src_w.align = range[2];

	of_property_read_u32_array(n, "src_h", range, 3);
	dpp->restriction.src_h.min = range[0];
	dpp->restriction.src_h.max = range[1];
	dpp->restriction.src_h.align = range[2];

	of_property_read_u32_array(n, "src_xy_align", align, 2);
	dpp->restriction.src_x_align = align[0];
	dpp->restriction.src_y_align = align[1];

	of_property_read_u32_array(n, "dst_f_w", range, 3);
	dpp->restriction.dst_f_w.min = range[0];
	dpp->restriction.dst_f_w.max = range[1];
	dpp->restriction.dst_f_w.align = range[2];

	of_property_read_u32_array(n, "dst_f_h", range, 3);
	dpp->restriction.dst_f_h.min = range[0];
	dpp->restriction.dst_f_h.max = range[1];
	dpp->restriction.dst_f_h.align = range[2];

	of_property_read_u32_array(n, "dst_w", range, 3);
	dpp->restriction.dst_w.min = range[0];
	dpp->restriction.dst_w.max = range[1];
	dpp->restriction.dst_w.align = range[2];

	of_property_read_u32_array(n, "dst_h", range, 3);
	dpp->restriction.dst_h.min = range[0];
	dpp->restriction.dst_h.max = range[1];
	dpp->restriction.dst_h.align = range[2];

	of_property_read_u32_array(n, "dst_xy_align", align, 2);
	dpp->restriction.dst_x_align = align[0];
	dpp->restriction.dst_y_align = align[1];

	of_property_read_u32_array(n, "blk_w", range, 3);
	dpp->restriction.blk_w.min = range[0];
	dpp->restriction.blk_w.max = range[1];
	dpp->restriction.blk_w.align = range[2];

	of_property_read_u32_array(n, "blk_h", range, 3);
	dpp->restriction.blk_h.min = range[0];
	dpp->restriction.blk_h.max = range[1];
	dpp->restriction.blk_h.align = range[2];

	of_property_read_u32_array(n, "blk_xy_align", align, 2);
	dpp->restriction.blk_x_align = align[0];
	dpp->restriction.blk_y_align = align[1];

	if (of_property_read_u32(n, "src_h_rot_max",
				&dpp->restriction.src_h_rot_max))
		dpp->restriction.src_h_rot_max = dpp->restriction.src_h.max;
}

static void dpp_print_restriction(struct dpp_device *dpp)
{
	struct dpp_restriction *res = &dpp->restriction;

	dpp_info("src_f_w[%d %d %d] src_f_h[%d %d %d]\n",
			res->src_f_w.min, res->src_f_w.max, res->src_f_w.align,
			res->src_f_h.min, res->src_f_h.max, res->src_f_h.align);
	dpp_info("src_w[%d %d %d] src_h[%d %d %d] src_x_y_align[%d %d]\n",
			res->src_w.min, res->src_w.max, res->src_w.align,
			res->src_h.min, res->src_h.max, res->src_h.align,
			res->src_x_align, res->src_y_align);

	dpp_info("dst_f_w[%d %d %d] dst_f_h[%d %d %d]\n",
			res->dst_f_w.min, res->dst_f_w.max, res->dst_f_w.align,
			res->dst_f_h.min, res->dst_f_h.max, res->dst_f_h.align);
	dpp_info("dst_w[%d %d %d] dst_h[%d %d %d] dst_x_y_align[%d %d]\n",
			res->dst_w.min, res->dst_w.max, res->dst_w.align,
			res->dst_h.min, res->dst_h.max, res->dst_h.align,
			res->dst_x_align, res->dst_y_align);

	dpp_info("blk_w[%d %d %d] blk_h[%d %d %d] blk_x_y_align[%d %d]\n",
			res->blk_w.min, res->blk_w.max, res->blk_w.align,
			res->blk_h.min, res->blk_h.max, res->blk_h.align,
			res->blk_x_align, res->blk_y_align);

	dpp_info("src_h_rot_max[%d]\n", res->src_h_rot_max);
}

static void dpp_parse_dt(struct dpp_device *dpp, struct device *dev)
{
	struct device_node *node = dev->of_node;
	struct dpp_device *dpp0 = get_dpp_drvdata(0);
	struct dpp_restriction *res = &dpp->restriction;
	int i;
	char format_list[128] = {0, };
	int len = 0, ret;

	dpp->id = of_alias_get_id(dev->of_node, "dpp");
	dpp_info("dpp(%d) probe start..\n", dpp->id);
	of_property_read_u32(node, "attr", (u32 *)&dpp->attr);
	dpp_info("attributes = 0x%lx\n", dpp->attr);
	of_property_read_u32(node, "port", (u32 *)&dpp->port);
	dpp_info("AXI port = %d\n", dpp->port);

	if (dpp->id == 0) {
		dpp_parse_restriction(dpp, node);
		dpp_print_restriction(dpp);
	} else {
		memcpy(&dpp->restriction, &dpp0->restriction,
				sizeof(struct dpp_restriction));
		dpp_print_restriction(dpp);
	}

	of_property_read_u32(node, "scale_down", (u32 *)&res->scale_down);
	of_property_read_u32(node, "scale_up", (u32 *)&res->scale_up);
	dpp_info("max scale up(%dx), down(1/%dx) ratio\n", res->scale_up,
			res->scale_down);

	memcpy(res->format, default_fmt, sizeof(u32) * DEFAULT_FMT_CNT);
	of_property_read_u32(node, "fmt_cnt", (u32 *)&res->format_cnt);
	of_property_read_u32_array(node, "fmt", &res->format[DEFAULT_FMT_CNT],
			res->format_cnt);
	res->format_cnt += DEFAULT_FMT_CNT;
	dpp_info("supported format count = %d\n", dpp->restriction.format_cnt);

	for (i = 0; i < dpp->restriction.format_cnt; ++i) {
		ret = snprintf(format_list + len, sizeof(format_list) - len,
				"%d ", dpp->restriction.format[i]);
		len += ret;
	}
	format_list[len] = '\0';
	dpp_info("supported format list : %s\n", format_list);

	dpp->dev = dev;
}

static irqreturn_t dpp_irq_handler(int irq, void *priv)
{
	struct dpp_device *dpp = priv;
	u32 dpp_irq = 0;

	spin_lock(&dpp->slock);
	if (dpp->state == DPP_STATE_OFF)
		goto irq_end;

	dpp_irq = dpp_reg_get_irq_and_clear(dpp->id);

irq_end:
	del_timer(&dpp->d.op_timer);
	spin_unlock(&dpp->slock);
	return IRQ_HANDLED;
}

static irqreturn_t dma_irq_handler(int irq, void *priv)
{
	struct dpp_device *dpp = priv;
	u32 irqs, val = 0;

	spin_lock(&dpp->dma_slock);
	if (dpp->state == DPP_STATE_OFF)
		goto irq_end;

	if (test_bit(DPP_ATTR_ODMA, &dpp->attr)) { /* ODMA case */
		irqs = odma_reg_get_irq_and_clear(dpp->id);

		if ((irqs & ODMA_WRITE_SLAVE_ERROR) ||
			       (irqs & ODMA_STATUS_DEADLOCK_IRQ)) {
			dpp_err("odma%d error irq occur(0x%x)\n", dpp->id, irqs);
			dpp_dump(dpp);
			goto irq_end;
		}
		if (irqs & ODMA_STATUS_FRAMEDONE_IRQ) {
			dpp->d.done_count++;
			wake_up_interruptible_all(&dpp->framedone_wq);
			DPU_EVENT_LOG(DPU_EVT_DPP_FRAMEDONE, &dpp->sd,
					ktime_set(0, 0));
			goto irq_end;
		}
	} else { /* IDMA case */
		irqs = idma_reg_get_irq_and_clear(dpp->id);

		if (irqs & IDMA_RECOVERY_START_IRQ) {
			DPU_EVENT_LOG(DPU_EVT_DMA_RECOVERY, &dpp->sd,
					ktime_set(0, 0));
			val = (u32)dpp->dpp_config->config.dpp_parm.comp_src;
			dpp->d.recovery_cnt++;
			dpp_info("dma%d recovery start(0x%x).. cnt(%d)\n",
					dpp->id, irqs, dpp->d.recovery_cnt);

#ifdef CONFIG_SEC_ABC
			if (!(dpp->d.recovery_cnt % 10))
				sec_abc_send_event("MODULE=display@ERROR=afbc_recovery");
#endif
			goto irq_end;
		}
		if ((irqs & IDMA_AFBC_TIMEOUT_IRQ) ||
				(irqs & IDMA_READ_SLAVE_ERROR) ||
				(irqs & IDMA_STATUS_DEADLOCK_IRQ)) {
			dpp_err("dma%d error irq occur(0x%x)\n", dpp->id, irqs);
			dpp_dump(dpp);
			goto irq_end;
		}
		/*
		 * TODO: Normally, DMA framedone occurs before DPP framedone.
		 * But DMA framedone can occur in case of AFBC crop mode
		 */
		if (irqs & IDMA_STATUS_FRAMEDONE_IRQ) {
			DPU_EVENT_LOG(DPU_EVT_DMA_FRAMEDONE, &dpp->sd,
					ktime_set(0, 0));
			goto irq_end;
		}
#if defined(CONFIG_SOC_EXYNOS9820)
		/* TODO: SoC dependency will be removed */
		if (irqs & IDMA_AFBC_CONFLICT_IRQ) {
			dpp_err("dma%d AFBC conflict irq occurs\n", dpp->id);
			goto irq_end;
		}
#endif
	}

irq_end:
	if (!test_bit(DPP_ATTR_DPP, &dpp->attr))
		del_timer(&dpp->d.op_timer);

	spin_unlock(&dpp->dma_slock);
	return IRQ_HANDLED;
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
	dpp_info("dma res: start(0x%x), end(0x%x)\n",
			(u32)res->start, (u32)res->end);

	dpp->res.dma_regs = devm_ioremap_resource(dpp->dev, res);
	if (!dpp->res.dma_regs) {
		dpp_err("failed to remap DPU_DMA SFR region\n");
		return -EINVAL;
	}

	/* DPP0 channel can only access common area of DPU_DMA */
	if (dpp->id == 0) {
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

	if (test_bit(DPP_ATTR_DPP, &dpp->attr)) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
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
	}

	return 0;
}

#ifdef CONFIG_EXYNOS_MCD_HDR
#if 0
static char *dpp_attr_string[DPP_ATTR_DPP + 1] = {
	"DPP_ATTR_AFBC",
	"DPP_ATTR_BLOCK",
	"DPP_ATTR_FLIP",
	"DPP_ATTR_ROT",
	"DPP_ATTR_CSC",
	"DPP_ATTR_SCALE",
	"DPP_ATTR_HDR",
	"DPP_ATTR_C_HDR",
	"DPP_ATTR_C_HDR10_PLUS",
	"DPP_ATTR_WCG",
	"None10",
	"None11",
	"None12",
	"None13",
	"None14",
	"None15",
	"DPP_ATTR_IDMA",
	"DPP_ATTR_ODMA",
	"DPP_ATTR_DPP",

};

static void print_dpp_restrict(unsigned long attr)
{
	int i = 0;

	for (i = 0; i <= DPP_ATTR_DPP; i++) {
		if ((attr & (1 << i)) && (dpp_attr_string[i] != NULL))
			dpp_info("DPP ATTR : %s\n", dpp_attr_string[i]);
	}
}
#endif
#endif

static int dpp_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dpp_device *dpp;
	int ret = 0;
#ifdef CONFIG_EXYNOS_MCD_HDR
	int attr = 0;
#endif

	dpp = devm_kzalloc(dev, sizeof(*dpp), GFP_KERNEL);
	if (!dpp) {
		dpp_err("failed to allocate dpp device.\n");
		ret = -ENOMEM;
		goto err;
	}
	dpp_parse_dt(dpp, dev);
	dpp_drvdata[dpp->id] = dpp;

	spin_lock_init(&dpp->slock);
	spin_lock_init(&dpp->dma_slock);
	mutex_init(&dpp->lock);
	init_waitqueue_head(&dpp->framedone_wq);

	ret = dpp_init_resources(dpp, pdev);
	if (ret)
		goto err_clk;

	dpp_init_subdev(dpp);

#ifdef CONFIG_EXYNOS_MCD_HDR
	dpp_get_mcd_hdr_subdev(dpp, MCD_HDR_MODULE_NAME);

	ret = v4l2_subdev_call(dpp->mcd_sd,
		core , ioctl ,GET_ATTR, &attr);
	if (ret)
		dpp_err("DPP:ERR:%s:faild to get attr wcg\n", __func__);

	if (IS_SUPPORT_HDR10(attr))
		dpp->attr |= (1 << DPP_ATTR_C_HDR);

	if (IS_SUPPORT_HDR10P(attr))
		dpp->attr |= (1 << DPP_ATTR_C_HDR10_PLUS);

	if (IS_SUPPORT_WCG(attr))
		dpp->attr |= (1 << DPP_ATTR_WCG);

	dpp_info("DPP:INFO:%s:%x attr : %x", __func__, dpp->id, dpp->attr);
#if 0
	print_dpp_restrict(dpp->attr);
#endif
#endif

	platform_set_drvdata(pdev, dpp);
	setup_timer(&dpp->d.op_timer, dpp_op_timer_handler, (unsigned long)dpp);

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
	dpp_info("%s driver unloaded\n", pdev->name);
	return 0;
}

static const struct of_device_id dpp_of_match[] = {
	{ .compatible = "samsung,exynos9-dpp" },
	{},
};
MODULE_DEVICE_TABLE(of, dpp_of_match);

static struct platform_driver dpp_driver __refdata = {
	.probe		= dpp_probe,
	.remove		= dpp_remove,
	.driver = {
		.name	= DPP_MODULE_NAME,
		.owner	= THIS_MODULE,
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
