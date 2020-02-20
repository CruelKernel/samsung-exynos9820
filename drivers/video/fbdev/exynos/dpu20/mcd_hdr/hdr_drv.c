/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Samsung EXYNOS9SoC series HDR driver
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
#include <linux/platform_device.h>

#include "mcd_cm.h"
#include "hdr_drv.h"
#include "mcd_cm_def.h"
#include "hdr_reg.h"

#include "../decon.h"



/*log level for hdr default : 6*/
int hdr_log_level = 7;

static unsigned int __eq_to_gammut(unsigned int eq_mode)
{
	int ret = INDEX_GAMUT_UNSPECIFIED;

	switch (eq_mode & 0x3f) {
		case CSC_BT_601:
		case CSC_BT_709:
			ret = INDEX_GAMUT_BT709;
			break;
		case CSC_BT_2020:
			ret = INDEX_GAMUT_BT2020;
			break;
		case CSC_DCI_P3:
			ret = INDEX_GAMUT_DCI_P3;
			break;
		case CSC_STANDARD_UNSPECIFIED:
			ret = INDEX_GAMUT_UNSPECIFIED;
			break;
		case CSC_BT_601_625:
		case CSC_BT_601_625_UNADJUSTED:
			ret = INDEX_GAMUT_BT601_625;
			break;
		case CSC_BT_601_525:
		case CSC_BT_601_525_UNADJUSTED:
			ret = INDEX_GAMUT_BT601_525;
			break;
		case CSC_BT_2020_CONSTANT_LUMINANCE:
			ret = INDEX_GAMUT_BT2020;
			break;

		case CSC_BT_470M:
			ret = INDEX_GAMUT_BT470M;
			break;

		case CSC_FILM:
			ret = INDEX_GAMUT_FILM;
			break;

		case CSC_ADOBE_RGB:
			ret = INDEX_GAMUT_ADOBE_RGB;
			break;

		default:
			hdr_err("HDR:ERR:%s:undefined eq_mode : %d\n", __func__, eq_mode);
			break;
	}

	return ret;
}

static unsigned int __hdr_to_gamma(unsigned int hdr_std)
{
	int ret = INDEX_GAMMA_UNSPECIFIED;

	switch(hdr_std) {
		case DPP_TRANSFER_UNSPECIFIED:
			ret = INDEX_GAMMA_UNSPECIFIED;
			break;
		case DPP_HDR_ST2084:
			ret = INDEX_GAMMA_ST2084;
			break;
		case DPP_HDR_HLG:
			ret = INDEX_GAMMA_HLG;
			break;
		case DPP_TRANSFER_LINEAR:
			ret = INDEX_GAMMA_LINEAR;
			break;
		case DPP_TRANSFER_SRGB:
			ret = INDEX_GAMMA_SRGB;
			break;
		case DPP_TRANSFER_SMPTE_170M:
#ifdef ASSUME_SMPTE170M_IS_SRGB
			ret = INDEX_GAMMA_SRGB;
#else
			ret = INDEX_GAMMA_SMPTE_170M;
#endif
			break;
		case DPP_TRANSFER_GAMMA2_2:
			ret = INDEX_GAMMA_GAMMA2_2;
			break;
		case DPP_TRANSFER_GAMMA2_6:
			ret = INDEX_GAMMA_GAMMA2_6;
			break;
		case DPP_TRANSFER_GAMMA2_8:
			ret = INDEX_GAMMA_GAMMA2_8;
			break;

		default:
			hdr_err("HDR:ERR:%s:undefined hdr_std : %d\n", __func__, hdr_std);
			break;
	}
	return ret;
}

static unsigned int __colorspace_to_gamut(unsigned int color_mode)
{
	int gamut = INDEX_GAMUT_UNSPECIFIED;

	switch(color_mode) {

		case HAL_COLOR_MODE_NATIVE:
			gamut = INDEX_GAMUT_UNSPECIFIED;
			break;
		case HAL_COLOR_MODE_STANDARD_BT601_625:
			gamut = INDEX_GAMUT_BT601_625;
			break;
		case HAL_COLOR_MODE_STANDARD_BT601_625_UNADJUSTED:
			gamut = INDEX_GAMUT_BT601_625;
			break;
		case HAL_COLOR_MODE_STANDARD_BT601_525:
			gamut = INDEX_GAMUT_BT601_525;
			break;
		case HAL_COLOR_MODE_STANDARD_BT601_525_UNADJUSTED:
			gamut = INDEX_GAMUT_BT601_525;
			break;
		case HAL_COLOR_MODE_STANDARD_BT709:
			gamut = INDEX_GAMUT_BT709;
			break;
		case HAL_COLOR_MODE_DCI_P3:
			gamut = INDEX_GAMUT_DCI_P3;
			break;
		case HAL_COLOR_MODE_SRGB:
			gamut = INDEX_GAMUT_BT709;
			break;
		case HAL_COLOR_MODE_ADOBE_RGB:
			gamut = INDEX_GAMUT_ADOBE_RGB;
			break;
		case HAL_COLOR_MODE_DISPLAY_P3:
			gamut = INDEX_GAMUT_DCI_P3;
			break;
	}

	return gamut;
}

static unsigned int __colorspace_to_gamma_wcg(unsigned int color_mode)
{
	int gamma = INDEX_GAMMA_UNSPECIFIED;
	
	switch(color_mode) {

		case HAL_COLOR_MODE_NATIVE:
			gamma = INDEX_GAMMA_UNSPECIFIED;
			break;
		case HAL_COLOR_MODE_STANDARD_BT601_625:
		case HAL_COLOR_MODE_STANDARD_BT601_625_UNADJUSTED:
		case HAL_COLOR_MODE_STANDARD_BT601_525:
		case HAL_COLOR_MODE_STANDARD_BT601_525_UNADJUSTED:
		case HAL_COLOR_MODE_STANDARD_BT709:
#ifdef ASSUME_SMPTE170M_IS_SRGB
			gamma = INDEX_GAMMA_SRGB;
#else
			gamma = INDEX_GAMMA_SMPTE_170M;
#endif
			break;
		case HAL_COLOR_MODE_DCI_P3:
			gamma = INDEX_GAMMA_GAMMA2_6;
			break;
		case HAL_COLOR_MODE_SRGB:
			gamma = INDEX_GAMMA_SRGB;
			break;
		case HAL_COLOR_MODE_ADOBE_RGB:
			gamma = INDEX_GAMMA_GAMMA2_2;
			break;
		case HAL_COLOR_MODE_DISPLAY_P3:
			gamma = INDEX_GAMMA_SRGB;
			break;
	}

	return gamma;
}

static unsigned int __colorspace_to_gamma_hdr(unsigned int color_mode)
{
	int gamma = INDEX_GAMMA_UNSPECIFIED;
	
	switch(color_mode) {

		case HAL_COLOR_MODE_NATIVE:
			gamma = INDEX_GAMMA_UNSPECIFIED;
			break;
		case HAL_COLOR_MODE_STANDARD_BT601_625:
		case HAL_COLOR_MODE_STANDARD_BT601_625_UNADJUSTED:
		case HAL_COLOR_MODE_STANDARD_BT601_525:
		case HAL_COLOR_MODE_STANDARD_BT601_525_UNADJUSTED:
		case HAL_COLOR_MODE_STANDARD_BT709:
#ifdef ASSUME_SMPTE170M_IS_SRGB
#ifdef ASSUME_SRGB_IS_GAMMA22_FOR_HDR
			gamma = INDEX_GAMMA_GAMMA2_2;
#else
			gamma = INDEX_GAMMA_SRGB;
#endif
#else
			gamma = INDEX_GAMMA_SMPTE_170M;
#endif
			break;
		case HAL_COLOR_MODE_DCI_P3:
			gamma = INDEX_GAMMA_GAMMA2_6;
			break;
		case HAL_COLOR_MODE_SRGB:
#ifdef ASSUME_SRGB_IS_GAMMA22_FOR_HDR
			gamma = INDEX_GAMMA_GAMMA2_2;
#else
			gamma = INDEX_GAMMA_SRGB;
#endif
			break;
		case HAL_COLOR_MODE_ADOBE_RGB:
			gamma = INDEX_GAMMA_GAMMA2_2;
			break;
		case HAL_COLOR_MODE_DISPLAY_P3:
#ifdef ASSUME_SRGB_IS_GAMMA22_FOR_HDR
			gamma = INDEX_GAMMA_GAMMA2_2;
#else
			gamma = INDEX_GAMMA_SRGB;
#endif
			break;
	}

	return gamma;
}

static int mcd_config_wcg(struct mcd_hdr_device *hdr, struct wcg_config *config)
{
	int ret = 0;
	struct mcd_cm_params_info params;

	memset(&params, 0, sizeof (struct mcd_cm_params_info));

	params.src_gamma = __hdr_to_gamma(config->hdr_mode);
	params.src_gamut = __eq_to_gammut(config->eq_mode);

	params.dst_gamma = __colorspace_to_gamma_wcg(config->color_mode);
	params.dst_gamut = __colorspace_to_gamut(config->color_mode);
    
	mcd_cm_reg_set_params(hdr, &params);

	return ret;
}


static int mcd_config_hdr(struct mcd_hdr_device *hdr, struct hdr10_config *config)
{
	struct mcd_cm_params_info params;

	memset(&params, 0, sizeof (struct mcd_cm_params_info));

	params.needDither = 1;

	params.src_gamma = __hdr_to_gamma(config->hdr_mode);
	params.src_gamut = __eq_to_gammut(config->eq_mode);

	params.dst_gamma = __colorspace_to_gamma_hdr(config->color_mode);
	params.dst_gamut = __colorspace_to_gamut(config->color_mode);

	params.src_max_luminance = config->src_max_luminance;
	params.dst_max_luminance = config->dst_max_luminance;

	if (config->lut != NULL)
		params.hdr10p_lut = config->lut;

#ifdef HDR_DEBUG
	hdr_info("** MCD Dynamic Meta Data **\n");
	
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			params.hdr10p_lut, sizeof(unsigned int) * 42, false);
#endif

	mcd_cm_reg_set_params(hdr, &params);

#ifdef HDR_DEBUG
	hdr_info("** MCD Dynamic Meta Data After **\n");
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
		hdr->regs + DPP_MCD_CM_DM_ADDR(0), sizeof(unsigned int) * 41, false);
#endif

	return 0;
}

static int mcd_reg_reset(struct mcd_hdr_device *hdr)
{
	int ret = 0;
	
	ret = mcd_cm_reg_reset(hdr);

	return ret;
}

static int mcd_reg_dump(struct mcd_hdr_device *hdr)
{
	int ret = 0;

	ret = mcd_cm_reg_dump(hdr);

	return ret;
}


static long mcd_hdr_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct wcg_config *color_config;
	struct hdr10_config *hdr10_data;
	struct mcd_hdr_device *hdr = v4l2_get_subdevdata(sd);

	switch(cmd) {

	case CONFIG_WCG:
		if (!IS_SUPPORT_WCG(hdr->attr)) {
			hdr_err("HDR:ERR:%s:hdr id : %d does not support wcg\n", __func__, hdr->id);
			return -EINVAL;
		}
		color_config = (struct wcg_config *)arg;

		ret = mcd_config_wcg(hdr, color_config);

		break;

	case CONFIG_HDR10:
		if (!IS_SUPPORT_HDR10(hdr->attr)) {
			hdr_err("HDR:ERR:%s:hdr id : %d does not support hdr10\n", __func__, hdr->id);
			return -EINVAL;
		}
		hdr10_data = (struct hdr10_config *)arg;
		ret = mcd_config_hdr(hdr, hdr10_data);
		break;

	case CONFIG_HDR10P:
		if (!IS_SUPPORT_HDR10P(hdr->attr)) {
			hdr_err("HDR:ERR:%s:hdr id : %d does not support hdr10 plus\n", __func__, hdr->id);
			return -EINVAL;
		}
		hdr10_data = (struct hdr10_config *)arg;
		ret = mcd_config_hdr(hdr, hdr10_data);
		break;

	case RESET_MCD_IP:
	case STOP_MCD_IP:
		ret = mcd_reg_reset(hdr);
		break;

	case DUMP_MCD_IP:
		ret = mcd_reg_dump(hdr);
		break;

	case GET_ATTR:
		hdr_dbg("HDR:DBG:%s:%d:GET_ATTR: %x\n", __func__, hdr->id, hdr->attr);
		if (arg)
			*((int *)arg) = hdr->attr;
		break;
	default :
		break;
	}

	return ret;
}

static int hdr_s_stream(struct v4l2_subdev *sd, int enable)
{
	int ret = 0;

	hdr_dbg("HDR:DBG:%s\n", __func__);

	return ret;
}

static const struct v4l2_subdev_core_ops mcd_hdr_v4l2_core_ops = {
	.ioctl = mcd_hdr_ioctl,
};

static const struct v4l2_subdev_video_ops mcd_hdr_v4l2_video_ops = {
	.s_stream = hdr_s_stream,
};

static struct v4l2_subdev_ops mcd_hdr_v4l2_ops = {
	.core = &mcd_hdr_v4l2_core_ops,
	.video = &mcd_hdr_v4l2_video_ops,
};

static void mcd_hdr_init_v4l2subdev(struct mcd_hdr_device *hdr)
{
	struct v4l2_subdev *sd = &hdr->sd;

	v4l2_subdev_init(sd, &mcd_hdr_v4l2_ops);
	sd->owner = THIS_MODULE;
	sd->grp_id = hdr->id;
	snprintf(sd->name, sizeof(sd->name), "%s.%d", "hdr-sd", hdr->id);

	v4l2_set_subdevdata(sd, hdr);
}

static int mcd_hdr_get_resource(struct mcd_hdr_device *hdr, struct platform_device *pdev)
{
	int ret = 0, attr;
	struct resource *res;
	struct device *dev = hdr->dev;

	if ((pdev == NULL) || (dev == NULL)) {
		hdr_err("HDR:ERR:%s: %s\n", __func__,
			pdev == NULL ? "pdev is NULL" : "dev is NULL");
		return -EINVAL;
	}

	hdr->id = of_alias_get_id(dev->of_node, "mcdhdr");
	hdr_info("HDR:INFO:%s:id:%d\n", __func__, hdr->id);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		hdr_err("HDR:ERR:%s:failed to get MEM resource\n", __func__);
		return -ENOENT;
	}

	hdr_info(
"HDR:INFO:%s:res 0x%x ~ 0x%x\n",
		__func__, (u32)(res->start), (u32)(res->end));

	hdr->regs = devm_ioremap_resource(hdr->dev, res);
	if (hdr->regs == NULL) {
		hdr_err("HDR:ERR:%s:failed to remap for resource\n", __func__);
		return -EINVAL;
	}

	of_property_read_u32(dev->of_node, "attr", &attr);
	hdr_info("HDR:INFO:%s:WCG : %s, HDR10 : %s, HDR10+ : %s\n",__func__,
		IS_SUPPORT_WCG(attr) ? "SUPPORT" : "N/A",
		IS_SUPPORT_HDR10(attr) ? "SUPPORT" : "N/A",
		IS_SUPPORT_HDR10P(attr) ? "SUPPORT" : "N/A");
	hdr->attr = attr;

	return ret;
}


static int mcd_hdr_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct mcd_hdr_device *hdr;
	struct device *dev = &pdev->dev;

	hdr_dbg("HDR:DBG:+%s\n", __func__);

	hdr = devm_kzalloc(dev, sizeof(struct mcd_hdr_device), GFP_KERNEL);
	if (hdr == NULL) {
		hdr_err("HDR:ERR:%s:faield to alloc mem for hdr device\n", __func__);
		ret = -ENOMEM;
		goto probe_err;
	}
	hdr->dev = dev;

	ret = mcd_hdr_get_resource(hdr, pdev);
	if (ret) {
		hdr_err("HDR:ERR:%s:failed to get resource\n", __func__);
		goto probe_err_free;
	}

	mcd_hdr_init_v4l2subdev(hdr);

	platform_set_drvdata(pdev, hdr);

	return 0;
probe_err_free:
	kfree(hdr);

probe_err:
	hdr_dbg("HDR:DBG:-%s\n", __func__);
	return ret;
}

static int mcd_hdr_remove(struct platform_device *pdev)
{
	int ret = 0;
	hdr_dbg("HDR:DBG:+%s\n", __func__);

	hdr_dbg("HDR:DBG:-%s\n", __func__);
	return ret;
}

static const struct of_device_id mcd_hdr_match_tbl[] = {
	{.compatible = "samsung,exynos9-mcdhdr" },
	{},
};

static struct platform_driver hdr_driver __refdata = {
	.probe = mcd_hdr_probe,
	.remove = mcd_hdr_remove,
	.driver = {
		.name = MCD_HDR_MODULE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(mcd_hdr_match_tbl),
		.suppress_bind_attrs = true,
	}
};

static int mcd_hdr_register(void)
{
	hdr_dbg("%s was called\n", __func__);

	return platform_driver_register(&hdr_driver);
}

fs_initcall_sync(mcd_hdr_register);
MODULE_AUTHOR("Minwoo Kim <minwoo7945.kim@samsung.com>");
MODULE_DESCRIPTION("Samsung Exynos9 MCD-HDR Driver");
MODULE_LICENSE("GPL");
