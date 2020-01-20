/*
 * Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is/mipi-csi functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/io.h>

#include "fimc-is-config.h"
#include "fimc-is-regs.h"
#include "fimc-is-hw.h"
#include "fimc-is-device-csi.h"
#include "fimc-is-device-sensor.h"
#include "fimc-is-device-ischain.h"
#ifndef ENABLE_IS_CORE
#include "fimc-is-hw-csi.h"
#endif

extern int debug_csi;

static u32 get_hsync_settle(struct fimc_is_sensor_cfg *cfg,
	const u32 cfgs, u32 width, u32 height, u32 framerate)
{
	u32 settle;
	u32 max_settle;
	u32 proximity_framerate, proximity_settle;
	u32 i;

	settle = 0;
	max_settle = 0;
	proximity_framerate = 0;
	proximity_settle = 0;

	for (i = 0; i < cfgs; i++) {
		if ((cfg[i].width == width) &&
			(cfg[i].height == height) &&
			(cfg[i].framerate == framerate)) {
			settle = cfg[i].settle;
			break;
		}

		if ((cfg[i].width == width) &&
			(cfg[i].height == height) &&
			(cfg[i].framerate > proximity_framerate)) {
			proximity_settle = cfg[i].settle;
			proximity_framerate = cfg[i].framerate;
		}

		if (cfg[i].settle > max_settle)
			max_settle = cfg[i].settle;
	}

	if (!settle) {
		if (proximity_settle) {
			settle = proximity_settle;
		} else {
			/*
			 * return a max settle time value in above table
			 * as a default depending on the channel
			 */
			settle = max_settle;

			warn("could not find proper settle time: %dx%d@%dfps",
				width, height, framerate);
		}
	}

	return settle;
}

static u32 get_vci_channel(struct fimc_is_vci *vci,
	const u32 vcis, u32 pixelformat)
{
	u32 i;
	u32 index = vcis;

	FIMC_BUG(!vci);

	for (i = 0; i < vcis; i++) {
		if (vci[i].pixelformat == pixelformat) {
			index = i;
			break;
		}
	}

	if (index == vcis) {
		err("invalid vc setting(foramt : %d)", pixelformat);
		BUG();
	}

	return index;
}

#ifdef ENABLE_CSIISR
static irqreturn_t fimc_is_isr_csi(int irq, void *data)
{
	u32 status;
	struct fimc_is_device_csi *csi;

	csi = data;

#ifndef ENABLE_IS_CORE
	/* Below function get and clear interrupts */
	status = csi_hw_g_irq_src(csi->base_reg, NULL, true);

	if (status & S5PCSIS_INTSRC_FRAME_END) {
		u32 vsync_cnt = 0;

		/* Expecting vvalid is 1 in case of NO_ERROR */
		if (likely(atomic_read(&csi->vvalid) == 1)) {
			atomic_set(&csi->vvalid, 0);
		} else if (atomic_read(&csi->vvalid) > 1) {
			/* How many times occured FRAME START without END */
			err("CSI%d : Frame start without Frame end %d times\n",
					csi->instance, atomic_read(&csi->vvalid));
		} else {
			/* When vvalid is less than or equal ZERO */
			pr_debug("CSI%d : Frame end without Frame start\n", csi->instance);
			atomic_dec(&csi->vvalid);
		}

		atomic_set(&csi->vblank_count, atomic_read(&csi->vsync_count));

		vsync_cnt = atomic_read(&csi->fcount);
		v4l2_subdev_notify(*csi->subdev, CSI_NOTIFY_VBLANK, &vsync_cnt);
	}

	if (status & S5PCSIS_INTSRC_FRAME_START) {
		u32 vsync_cnt = 0;

		/* Expecting vvalid is 0 in case of NO_ERROR */
		if (likely(atomic_read(&csi->vvalid) == 0)) {
			atomic_set(&csi->vvalid, 1);
		} else if (atomic_read(&csi->vvalid) < 0) {
			/* How many times occured FRAME END without START */
			err("CSI%d : Frame end without Frame start %ld times\n",
					csi->instance, abs(atomic_read(&csi->vvalid)));
		} else {
			/* When vvalid is bigger than ZERO */
			pr_debug("CSI%d : Frame start without Frame end\n", csi->instance);
			atomic_inc(&csi->vvalid);
		}

		atomic_inc(&csi->fcount);

		vsync_cnt = atomic_read(&csi->fcount);
		v4l2_subdev_notify(*csi->subdev, CSI_NOTIFY_VSYNC, &vsync_cnt);
	}

	if (unlikely(status & S5PCSIS_INTSRC_ERRORS))
		goto err_status;

	return IRQ_HANDLED;

err_status:
	/* TODO: Belows are Error interrupt handling. */
	if (status & S5PCSIS_INTSRC_ERR_ID) {
		err("CSI%d : ERR_ID, irq%d(%#X)\n",csi->instance, irq, status);
	}

	if (status & S5PCSIS_INTSRC_ERR_CRC) {
		err("CSI%d : ERR_CRC, irq%d(%#X)\n",csi->instance, irq, status);
	}

	if (status & S5PCSIS_INTSRC_ERR_ECC) {
		err("CSI%d : ERR_ECC, irq%d(%#X)\n",csi->instance, irq, status);
	}

	if (status & S5PCSIS_INTSRC_ERR_OVER) {
		err("CSI%d : ERR_OVER, irq%d(%#X)\n",csi->instance, irq, status);
	}

	if (status & S5PCSIS_INTSRC_ERR_LOST_FE) {
		err("CSI%d : ERR_LOST_FE, irq%d(%#X)\n",csi->instance, irq, status);
	}

	if (status & S5PCSIS_INTSRC_ERR_LOST_FS) {
		err("CSI%d : ERR_LOST_FS, irq%d(%#X)\n",csi->instance, irq, status);
	}

	if (status & S5PCSIS_INTSRC_ERR_SOT_HS) {
		err("CSI%d : ERR_SOT_HS, irq%d(%#X)\n",csi->instance, irq, status);
	}

	return IRQ_HANDLED;
#else
	status = csi_hw_g_irq_src(csi->base_reg, NULL, true);
	info("CSI%d : irq%d(%X)\n",csi->instance, irq, status);

	return IRQ_HANDLED;
#endif

}
#endif

int fimc_is_csi_open(struct v4l2_subdev *subdev,
	struct fimc_is_framemgr *framemgr)
{
	int ret = 0;
	u32 instance = 0;
	struct fimc_is_device_csi *csi;
	struct fimc_is_device_sensor *device;

	FIMC_BUG(!subdev);

	csi = v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("csi is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	device = v4l2_get_subdev_hostdata(subdev);
	if (!device) {
		err("device is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	csi->sensor_cfgs = 0;
	csi->sensor_cfg = NULL;
	memset(&csi->image, 0, sizeof(struct fimc_is_image));

	ret = request_irq(csi->irq,
			fimc_is_isr_csi,
			IRQF_SHARED,
			"mipi-csi",
			csi);
	if (ret) {
		err("request_irq(IRQ_MIPICSI %d) is fail(%d)", csi->irq, ret);
		goto p_err;
	}

#ifndef ENABLE_IS_CORE
	atomic_set(&csi->vblank_count, 0);
	atomic_set(&csi->vvalid, 0);
#endif

p_err:
	return ret;
}

int fimc_is_csi_close(struct v4l2_subdev *subdev)
{
	int ret = 0;
	struct fimc_is_device_csi *csi;

	FIMC_BUG(!subdev);

	csi = v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("csi is NULL");
		ret = -EINVAL;
		goto p_err;
	}

#ifdef ENABLE_CSIISR
	free_irq(csi->irq, csi);
#endif

p_err:
	return ret;
}

/* value : module enum */
static int csi_init(struct v4l2_subdev *subdev, u32 value)
{
	int ret = 0;
	struct fimc_is_device_csi *csi;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor *device;

	FIMC_BUG(!subdev);

	csi = v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("csi is NULL");
		ret = -EINVAL;
		goto p_err;
	}

	device = container_of(csi->subdev, struct fimc_is_device_sensor, subdev_csi);
	module = &device->module_enum[value];

	csi->sensor_cfgs = module->cfgs;
	csi->sensor_cfg = module->cfg;
	csi->vcis = module->vcis;
	csi->vci = module->vci;
	csi->image.framerate = SENSOR_DEFAULT_FRAMERATE; /* default frame rate */
	csi->mode = module->mode;
	csi->lanes = module->lanes;

p_err:
	return ret;
}

static int csi_s_power(struct v4l2_subdev *subdev,
	int on)
{
	int ret = 0;
	struct fimc_is_device_csi *csi;

	FIMC_BUG(!subdev);

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("csi is NULL");
		return -EINVAL;
	}

	ret = exynos5_csis_phy_enable(csi->instance, on);
	if (ret) {
		err("fail to csi%d power on", csi->instance);
		goto p_err;
	}

p_err:
	mdbgd_front("%s(%d, %d)\n", csi, __func__, on, ret);
	return ret;
}

static const struct v4l2_subdev_core_ops core_ops = {
	.init = csi_init,
	.s_power = csi_s_power
};

static int csi_stream_on(struct fimc_is_device_csi *csi)
{
	int ret = 0;
	u32 settle, index;
	u32 __iomem *base_reg;
	struct fimc_is_device_sensor *device;

	FIMC_BUG(!csi);
	FIMC_BUG(!csi->sensor_cfg);

	device = container_of(csi->subdev, struct fimc_is_device_sensor, subdev_csi);
	base_reg = csi->base_reg;

	if (!device->cfg) {
		merr("[CSI] cfg is NULL", csi);
		ret = -EINVAL;
		goto p_err;
	}
	csi->lanes = device->cfg->lanes;

	settle = get_hsync_settle(
		csi->sensor_cfg,
		csi->sensor_cfgs,
		csi->image.window.width,
		csi->image.window.height,
		csi->image.framerate);
	minfo("[CSI] settle(%dx%d@%d) = %d\n", csi,
		csi->image.window.width,
		csi->image.window.height,
		csi->image.framerate,
		settle);

	index = get_vci_channel(csi->vci, csi->vcis, csi->image.format.pixelformat);
	minfo("[CSI] vci(0x%X) = %d\n", csi, csi->image.format.pixelformat, index);

	csi_hw_reset(base_reg);
	csi_hw_s_settle(base_reg, settle);
	csi_hw_s_lane(base_reg, &csi->image, csi->lanes);
	csi_hw_s_control(base_reg, CSIS_CTRL_INTERLEAVE_MODE, csi->mode);

	if (csi->mode == CSI_MODE_CH0_ONLY) {
		csi_hw_s_config(base_reg,
			CSI_VIRTUAL_CH_0,
			&csi->vci[index].config[CSI_VIRTUAL_CH_0],
			csi->image.window.width,
			csi->image.window.height);
	} else {
		csi_hw_s_config(base_reg,
			CSI_VIRTUAL_CH_0,
			&csi->vci[index].config[CSI_VIRTUAL_CH_0],
			csi->image.window.width,
			csi->image.window.height);
		csi_hw_s_config(base_reg,
			CSI_VIRTUAL_CH_1,
			&csi->vci[index].config[CSI_VIRTUAL_CH_1],
			csi->image.window.width,
			csi->image.window.height);
		csi_hw_s_config(base_reg,
			CSI_VIRTUAL_CH_2,
			&csi->vci[index].config[CSI_VIRTUAL_CH_2],
			csi->image.window.width,
			csi->image.window.height);
	}

	csi_hw_s_irq_msk(base_reg, true);
	csi_hw_enable(base_reg);

	if (unlikely(debug_csi))
		csi_hw_dump(base_reg);

p_err:
	return ret;
}

static int csi_stream_off(struct fimc_is_device_csi *csi)
{
	int ret = 0;
	u32 __iomem *base_reg;

	FIMC_BUG(!csi);

	base_reg = csi->base_reg;

	csi_hw_s_irq_msk(base_reg, false);

	csi_hw_disable(base_reg);

	return ret;
}

static int csi_s_stream(struct v4l2_subdev *subdev, int enable)
{
	int ret = 0;
	struct fimc_is_device_csi *csi;

	FIMC_BUG(!subdev);

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("csi is NULL");
		return -EINVAL;
	}

	if (enable) {
		ret = csi_stream_on(csi);
		if (ret) {
			err("csi_stream_on is fail(%d)", ret);
			goto p_err;
		}
	} else {
		ret = csi_stream_off(csi);
		if (ret) {
			err("csi_stream_off is fail(%d)", ret);
			goto p_err;
		}
#ifndef ENABLE_IS_CORE
		atomic_set(&csi->vvalid, 0);
#endif
	}

p_err:
	return 0;
}

static int csi_s_param(struct v4l2_subdev *subdev, struct v4l2_streamparm *param)
{
	int ret = 0;
	struct fimc_is_device_csi *csi;
	struct v4l2_captureparm *cp;
	struct v4l2_fract *tpf;

	FIMC_BUG(!subdev);
	FIMC_BUG(!param);

	cp = &param->parm.capture;
	tpf = &cp->timeperframe;

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("csi is NULL");
		return -EINVAL;
	}

	csi->image.framerate = tpf->denominator / tpf->numerator;

	mdbgd_front("%s(%d, %d)\n", csi, __func__, csi->image.framerate, ret);
	return ret;
}

static int csi_s_format(struct v4l2_subdev *subdev,
	struct v4l2_subdev_pad_config *cfg,
	struct v4l2_subdev_format *fmt)
{
	int ret = 0;
	struct fimc_is_device_csi *csi;

	FIMC_BUG(!subdev);
	FIMC_BUG(!fmt);

	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(subdev);
	if (!csi) {
		err("csi is NULL");
		return -EINVAL;
	}

	csi->image.window.offs_h = 0;
	csi->image.window.offs_v = 0;
	csi->image.window.width = fmt->format.width;
	csi->image.window.height = fmt->format.height;
	csi->image.window.o_width = fmt->format.width;
	csi->image.window.o_height = fmt->format.height;
	csi->image.format.pixelformat = fmt->format.code;
	csi->image.format.field = fmt->format.field;

	mdbgd_front("%s(%dx%d, %X)\n", csi, __func__, fmt->format.width, fmt->format.height, fmt->format.code);
	return ret;
}

static int csi_s_buffer(struct v4l2_subdev *subdev, void *buf, unsigned int *size)
{
	return 0;
}

static const struct v4l2_subdev_video_ops video_ops = {
	.s_stream = csi_s_stream,
	.s_parm = csi_s_param,
	.s_rx_buffer = csi_s_buffer
};

static const struct v4l2_subdev_pad_ops pad_ops = {
	.set_fmt = csi_s_format
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
	.video = &video_ops,
	.pad = &pad_ops
};

int fimc_is_csi_probe(void *parent, u32 instance)
{
	int ret = 0;
	struct v4l2_subdev *subdev_csi;
	struct fimc_is_device_csi *csi;
	struct fimc_is_device_sensor *device = parent;
	struct resource *mem_res;
	struct platform_device *pdev;

	FIMC_BUG(!device);

	subdev_csi = kzalloc(sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!subdev_csi) {
		merr("subdev_csi is NULL", device);
		ret = -ENOMEM;
		goto p_err;
	}
	device->subdev_csi = subdev_csi;

	csi = kzalloc(sizeof(struct fimc_is_device_csi), GFP_KERNEL);
	if (!csi) {
		merr("csi is NULL", device);
		ret = -ENOMEM;
		goto p_err_free1;
	}

	/* Get SFR base register */
	pdev = device->pdev;
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem_res) {
		probe_err("Failed to get io memory region(%p)", mem_res);
		ret = -EBUSY;
		goto err_get_resource;
	}

	csi->regs_start = mem_res->start;
	csi->regs_end = mem_res->end;
	csi->base_reg =  devm_ioremap_nocache(&pdev->dev, mem_res->start, resource_size(mem_res));
	if (!csi->base_reg) {
		probe_err("Failed to remap io region(%p)", csi->base_reg);
		ret = -ENOMEM;
		goto err_get_resource;
	}

	/* Get IRQ number */
	csi->irq = platform_get_irq(pdev, 0);
	if (csi->irq < 0) {
		probe_err("Failed to get csi_irq(%d)", csi->irq);
		ret = -EBUSY;
		goto err_get_irq;
	}

	/* pointer to me from device sensor */
	csi->subdev = &device->subdev_csi;

	csi->instance = instance;

	v4l2_subdev_init(subdev_csi, &subdev_ops);
	v4l2_set_subdevdata(subdev_csi, csi);
	v4l2_set_subdev_hostdata(subdev_csi, device);
	snprintf(subdev_csi->name, V4L2_SUBDEV_NAME_SIZE, "csi-subdev.%d", instance);
	ret = v4l2_device_register_subdev(&device->v4l2_dev, subdev_csi);
	if (ret) {
		merr("v4l2_device_register_subdev is fail(%d)", device, ret);
		goto err_reg_v4l2_subdev;
	}

	info("[%d][FRT:D] %s(%d)\n", instance, __func__, ret);
	return 0;

err_reg_v4l2_subdev:
err_get_irq:
	iounmap(csi->base_reg);

err_get_resource:
	kfree(csi);

p_err_free1:
	kfree(subdev_csi);
	device->subdev_csi = NULL;

p_err:
	err("[%d][FRT:D] %s(%d)\n", instance, __func__, ret);
	return ret;
}
