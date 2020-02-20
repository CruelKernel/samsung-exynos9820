/*
 * Samsung Exynos SoC series PDP drvier
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/mutex.h>

#include <media/v4l2-subdev.h>

#include "fimc-is-config.h"
#include "fimc-is-pdp.h"
#include "fimc-is-hw-pdp.h"
#include "fimc-is-device-sensor-peri.h"

static struct fimc_is_pdp pdp_devices[MAX_NUM_OF_PDP];

static irqreturn_t fimc_is_isr_pdp(int irq, void *data)
{
	struct fimc_is_module_enum *module;
	struct v4l2_subdev *subdev_module;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_pdp *pdp;
	unsigned int state;

	pdp = (struct fimc_is_pdp *)data;

	state = pdp_hw_g_irq_state(pdp->base, true) & ~pdp_hw_g_irq_mask(pdp->base);
	dbg_pdp(1, "IRQ: 0x%x\n", pdp, state);

	if (!pdp_hw_is_occured(state, PE_PAF_STAT0))
		return IRQ_NONE;

	if (debug_pdp >= 2) {
		module = (struct fimc_is_module_enum *)v4l2_get_subdev_hostdata(pdp->subdev);
		if (!module) {
			err("failed to get module");
			return IRQ_NONE;
		}

		subdev_module = module->subdev;
		if (!subdev_module) {
			err("module's subdev was not probed");
			return IRQ_NONE;
		}

		sensor = (struct fimc_is_device_sensor *)v4l2_get_subdev_hostdata(subdev_module);

		dbg_pdp(2, "IRQ: 0x%x, sensor fcount: %d\n", pdp, state, sensor->fcount);
	}

	tasklet_schedule(&pdp->tasklet_stat0);

	return IRQ_HANDLED;
}

static void pdp_tasklet_stat0(unsigned long data)
{
	struct fimc_is_pdp *pdp;
	struct fimc_is_module_enum *module;
	struct v4l2_subdev *subdev_module;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_device_csi *csi;
	struct fimc_is_subdev *dma_subdev;
	struct fimc_is_framemgr *framemgr;
	struct fimc_is_frame *frame;
	unsigned int frameptr;
	int ch;

	pdp = (struct fimc_is_pdp *)data;
	if (!pdp) {
		err("failed to get PDP");
		return;
	}

	module = (struct fimc_is_module_enum *)v4l2_get_subdev_hostdata(pdp->subdev);
	if (!module) {
		err("failed to get module");
		return;
	}

	subdev_module = module->subdev;
	if (!subdev_module) {
		err("module's subdev was not probed");
		return;
	}

	sensor = (struct fimc_is_device_sensor *)v4l2_get_subdev_hostdata(subdev_module);
	csi = (struct fimc_is_device_csi *)v4l2_get_subdevdata(sensor->subdev_csi);

	for (ch = CSI_VIRTUAL_CH_1; ch < CSI_VIRTUAL_CH_MAX; ch++) {
		if (sensor->cfg->output[ch].type == VC_PRIVATE) {
			dma_subdev = csi->dma_subdev[ch];
			if (!dma_subdev ||
				!test_bit(FIMC_IS_SUBDEV_START, &dma_subdev->state))
				continue;

			framemgr = GET_SUBDEV_FRAMEMGR(dma_subdev);
			if (!framemgr) {
				err("failed to get framemgr");
				continue;
			}

			framemgr_e_barrier(framemgr, FMGR_IDX_29);

			frameptr = atomic_read(&pdp->frameptr_stat0) % framemgr->num_frames;
			frame = &framemgr->frames[frameptr];
			frame->fcount = sensor->fcount;

			pdp_hw_g_stat0(pdp->base, (void *)frame->kvaddr_buffer[0],
				dma_subdev->output.width * dma_subdev->output.height);

			atomic_inc(&pdp->frameptr_stat0);

			framemgr_x_barrier(framemgr, FMGR_IDX_29);
		}
	}

	if (pdp->wq_stat0)
		queue_work(pdp->wq_stat0, &pdp->work_stat0);
	else
		schedule_work(&pdp->work_stat0);

	dbg_pdp(3, "%s, sensor fcount: %d\n", pdp, __func__, sensor->fcount);
}

static void pdp_worker_stat0(struct work_struct *work)
{
	struct fimc_is_pdp *pdp;
	struct fimc_is_module_enum *module;
	struct v4l2_subdev *subdev_module;
	struct fimc_is_device_sensor *sensor;
	struct paf_action *pa, *temp;
	unsigned long flag;

	pdp = container_of(work, struct fimc_is_pdp, work_stat0);
	module = (struct fimc_is_module_enum *)v4l2_get_subdev_hostdata(pdp->subdev);
	if (!module) {
		err("failed to get module");
		return;
	}

	subdev_module = module->subdev;
	if (!subdev_module) {
		err("module's subdev was not probed");
		return;
	}

	sensor = (struct fimc_is_device_sensor *)v4l2_get_subdev_hostdata(subdev_module);

	spin_lock_irqsave(&pdp->slock_paf_action, flag);
	list_for_each_entry_safe(pa, temp, &pdp->list_of_paf_action, list) {
		switch (pa->type) {
		case VC_STAT_TYPE_PDP_1_0_PDAF_STAT0:
		case VC_STAT_TYPE_PDP_1_1_PDAF_STAT0:
#ifdef ENABLE_FPSIMD_FOR_USER
			fpsimd_get();
			pa->notifier(pa->type, *(unsigned int *)&sensor->fcount, pa->data);
			fpsimd_put();
#else
			pa->notifier(pa->type, *(unsigned int *)&sensor->fcount, pa->data);
#endif
			break;
		default:
			break;
		}
	}
	spin_unlock_irqrestore(&pdp->slock_paf_action, flag);

	dbg_pdp(3, "%s, sensor fcount: %d\n", pdp, __func__, sensor->fcount);
}

int pdp_set_param(struct v4l2_subdev *subdev, struct paf_setting_t *regs, u32 regs_size)
{
	int i;
	struct fimc_is_pdp *pdp;

	pdp = (struct fimc_is_pdp *)v4l2_get_subdevdata(subdev);
	if (!pdp) {
		err("failed to get PDP");
		return -ENODEV;
	}

	dbg_pdp(1, "PDP(%p) SFR setting\n", pdp, pdp->base);
	for (i = 0; i < regs_size; i++) {
		dbg_pdp(1, "[%d] ofs: 0x%x, val: 0x%x\n", pdp,
				i, regs[i].reg_addr, regs[i].reg_data);
		writel(regs[i].reg_data, pdp->base + regs[i].reg_addr);
	}

	return 0;
}

int pdp_get_ready(struct v4l2_subdev *subdev, u32 *ready)
{
	*ready = 1;

	return 0;
}

int pdp_register_notifier(struct v4l2_subdev *subdev, enum itf_vc_stat_type type,
		paf_notifier_t notifier, void *data)
{
	struct fimc_is_pdp *pdp;
	struct paf_action *pa;
	unsigned long flag;

	pdp = (struct fimc_is_pdp *)v4l2_get_subdevdata(subdev);
	if (!pdp) {
		err("failed to get PDP");
		return -ENODEV;
	}

	switch (type) {
	case VC_STAT_TYPE_PDP_1_0_PDAF_STAT0:
	case VC_STAT_TYPE_PDP_1_1_PDAF_STAT0:
	case VC_STAT_TYPE_PDP_1_0_PDAF_STAT1:
	case VC_STAT_TYPE_PDP_1_1_PDAF_STAT1:
		pa = kzalloc(sizeof(struct paf_action), GFP_ATOMIC);
		if (!pa) {
			err_lib("failed to allocate a PAF action");
			return -ENOMEM;
		}

		pa->type = type;
		pa->notifier = notifier;
		pa->data = data;

		spin_lock_irqsave(&pdp->slock_paf_action, flag);
		list_add(&pa->list, &pdp->list_of_paf_action);
		spin_unlock_irqrestore(&pdp->slock_paf_action, flag);

		break;
	default:
		return -EINVAL;
	}

	dbg_pdp(2, "%s, type: %d, notifier: %p\n", pdp, __func__, type, notifier);

	return 0;
}

int pdp_unregister_notifier(struct v4l2_subdev *subdev, enum itf_vc_stat_type type,
		paf_notifier_t notifier)
{
	struct fimc_is_pdp *pdp;
	struct paf_action *pa, *temp;
	unsigned long flag;

	pdp = (struct fimc_is_pdp *)v4l2_get_subdevdata(subdev);
	if (!pdp) {
		err("failed to get PDP");
		return -ENODEV;
	}

	switch (type) {
	case VC_STAT_TYPE_PDP_1_0_PDAF_STAT0:
	case VC_STAT_TYPE_PDP_1_1_PDAF_STAT0:
	case VC_STAT_TYPE_PDP_1_0_PDAF_STAT1:
	case VC_STAT_TYPE_PDP_1_1_PDAF_STAT1:
		spin_lock_irqsave(&pdp->slock_paf_action, flag);
		list_for_each_entry_safe(pa, temp,
				&pdp->list_of_paf_action, list) {
			if ((pa->notifier == notifier)
					&& (pa->type == type)) {
				list_del(&pa->list);
				kfree(pa);
			}
		}
		spin_unlock_irqrestore(&pdp->slock_paf_action, flag);

		break;
	default:
		return -EINVAL;
	}

	dbg_pdp(2, "%s, type: %d, notifier: %p\n", pdp, __func__, type, notifier);

	return 0;
}

void pdp_notify(struct v4l2_subdev *subdev, unsigned int type, void *data)
{
	struct fimc_is_pdp *pdp;
	struct paf_action *pa, *temp;
	unsigned long flag;

	pdp = (struct fimc_is_pdp *)v4l2_get_subdevdata(subdev);
	if (!pdp) {
		err("failed to get PDP");
		return;
	}

	switch (type) {
	case CSIS_NOTIFY_DMA_END_VC_MIPISTAT:
		spin_lock_irqsave(&pdp->slock_paf_action, flag);
		list_for_each_entry_safe(pa, temp, &pdp->list_of_paf_action, list) {
			switch (pa->type) {
			case VC_STAT_TYPE_PDP_1_0_PDAF_STAT1:
			case VC_STAT_TYPE_PDP_1_1_PDAF_STAT1:
#ifdef ENABLE_FPSIMD_FOR_USER
				fpsimd_get();
				pa->notifier(pa->type, *(unsigned int *)data, pa->data);
				fpsimd_put();
#else
				pa->notifier(pa->type, *(unsigned int *)data, pa->data);
#endif
				break;
			default:
				break;
			}
		}
		spin_unlock_irqrestore(&pdp->slock_paf_action, flag);

	default:
		break;
	}

	dbg_pdp(2, "%s, sensor fcount: %d\n", pdp, __func__, *(unsigned int *)data);
}

int pdp_register(struct fimc_is_module_enum *module, int pdp_ch)
{
	struct fimc_is_device_sensor_peri *sensor_peri = module->private_data;
	struct v4l2_subdev *subdev_module;
	struct fimc_is_device_sensor *sensor;
	struct fimc_is_pdp *pdp;

	if (!sensor_peri) {
		err("could not refer to sensor's peri");
		return -ENODEV;
	}

	if (test_bit(FIMC_IS_SENSOR_PDP_AVAILABLE, &sensor_peri->peri_state)) {
		err("already registered");
		return -EINVAL;
	}

	if (pdp_ch >= MAX_NUM_OF_PDP) {
		err("requested channel: %d is invalid", pdp_ch);
		return -EINVAL;
	}

	subdev_module = module->subdev;
	if (!subdev_module) {
		err("module's subdev was not probed");
		return -ENODEV;
	}

	sensor = (struct fimc_is_device_sensor *)v4l2_get_subdev_hostdata(subdev_module);
	if (IS_ENABLED(CONFIG_SOC_EXYNOS9820) &&
			!IS_ENABLED(CONFIG_SOC_EXYNOS9820_EVT0)) {
		if ((sensor->cfg->pd_mode != PD_MOD3) &&
				(sensor->cfg->pd_mode != PD_MSPD_TAIL))
			return 0;
	} else {
		if (sensor->cfg->pd_mode != PD_MOD3)
			return 0;
	}

	pdp = &pdp_devices[pdp_ch];

	sensor_peri->pdp = pdp;
	sensor_peri->subdev_pdp = pdp->subdev;
	v4l2_set_subdev_hostdata(pdp->subdev, module);

	spin_lock_init(&pdp->slock_paf_action);
	INIT_LIST_HEAD(&pdp->list_of_paf_action);

	set_bit(FIMC_IS_SENSOR_PDP_AVAILABLE, &sensor_peri->peri_state);

	info("[PDP:%d] is registered\n", pdp->id);

	return 0;
}

int pdp_unregister(struct fimc_is_module_enum *module)
{
	struct fimc_is_device_sensor_peri *sensor_peri = module->private_data;
	struct fimc_is_pdp *pdp;
	struct paf_action *pa, *temp;
	unsigned long flag;

	if (!sensor_peri) {
		err("could not refer to sensor's peri");
		return -ENODEV;
	}

	if (!test_bit(FIMC_IS_SENSOR_PDP_AVAILABLE, &sensor_peri->peri_state)) {
		err("already unregistered");
		return -EINVAL;
	}

	pdp = (struct fimc_is_pdp *)v4l2_get_subdevdata(sensor_peri->subdev_pdp);
	if (!pdp) {
		err("failed to get PDP");
		return -ENODEV;
	}

	if (!list_empty(&pdp->list_of_paf_action)) {
		err("flush remaining notifiers...");
		spin_lock_irqsave(&pdp->slock_paf_action, flag);
		list_for_each_entry_safe(pa, temp,
				&pdp->list_of_paf_action, list) {
			list_del(&pa->list);
			kfree(pa);
		}
		spin_unlock_irqrestore(&pdp->slock_paf_action, flag);
	}

	sensor_peri->pdp = NULL;
	sensor_peri->subdev_pdp = NULL;
	v4l2_set_subdev_hostdata(pdp->subdev, NULL);

	clear_bit(FIMC_IS_SENSOR_PDP_AVAILABLE, &sensor_peri->peri_state);

	info("[PDP:%d] is unregistered\n", pdp->id);

	return 0;
}

int pdp_init(struct v4l2_subdev *subdev, u32 val)
{
	return 0;
}

static int pdp_s_stream(struct v4l2_subdev *subdev, int pd_mode)
{
	bool enable;
	unsigned int sensor_type;
	struct fimc_is_pdp *pdp;

	pdp = (struct fimc_is_pdp *)v4l2_get_subdevdata(subdev);
	if (!pdp) {
		err("failed to get PDP");
		return -ENODEV;
	}

	enable = pdp_hw_to_sensor_type(pd_mode, &sensor_type);
	if (enable) {
		tasklet_init(&pdp->tasklet_stat0, pdp_tasklet_stat0, (unsigned long)pdp);
		atomic_set(&pdp->frameptr_stat0, 0);
		INIT_WORK(&pdp->work_stat0, pdp_worker_stat0);

		if (debug_pdp >= 5) {
			info("[PDP:%d] is configured as default values\n", pdp->id);
			pdp_hw_s_config_default(pdp->base);
		}

		pdp_hw_s_sensor_type(pdp->base, sensor_type);
		pdp_hw_s_core(pdp->base, true);

		if (debug_pdp >= 2)
			pdp_hw_dump(pdp->base);
	} else {
		pdp_hw_s_core(pdp->base, false);

		tasklet_kill(&pdp->tasklet_stat0);
		if (flush_work(&pdp->work_stat0))
			info("flush pdp wq for stat0\n");
	}

	info("[PDP:%d] %s as PD mode: %d, IRQ: 0x%x\n",
		pdp->id, enable ? "enabled" : "disabled", pd_mode,
		pdp_hw_g_irq_state(pdp->base, false) & ~pdp_hw_g_irq_mask(pdp->base));

	return 0;
}

static int pdp_s_param(struct v4l2_subdev *subdev, struct v4l2_streamparm *param)
{
	return 0;
}

static int pdp_s_format(struct v4l2_subdev *subdev,
	struct v4l2_subdev_pad_config *cfg,
	struct v4l2_subdev_format *fmt)
{
	int ret = 0;
	size_t width, height;
	struct fimc_is_pdp *pdp;

	pdp = (struct fimc_is_pdp *)v4l2_get_subdevdata(subdev);
	if (!pdp) {
		err("A subdev data of PDP is null");
		return -ENODEV;
	}

	width = fmt->format.width;
	height = fmt->format.height;
	pdp->width = width;
	pdp->height = height;

	return ret;
}

static const struct v4l2_subdev_core_ops core_ops = {
	.init = pdp_init
};

static const struct v4l2_subdev_video_ops video_ops = {
	.s_stream = pdp_s_stream,
	.s_parm = pdp_s_param,
};

static const struct v4l2_subdev_pad_ops pad_ops = {
	.set_fmt = pdp_s_format
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
	.video = &video_ops,
	.pad = &pad_ops
};

struct fimc_is_pdp_ops pdp_ops = {
	.set_param = pdp_set_param,
	.get_ready = pdp_get_ready,
	.register_notifier = pdp_register_notifier,
	.unregister_notifier = pdp_unregister_notifier,
	.notify = pdp_notify,
};

static int __init pdp_probe(struct platform_device *pdev)
{
	int ret;
	int id;
	struct resource *res;
	struct fimc_is_pdp *pdp;
	struct device *dev = &pdev->dev;

	id = of_alias_get_id(dev->of_node, "pdp");
	if (id < 0 || id >= MAX_NUM_OF_PDP) {
		dev_err(dev, "invalid id (out-of-range)\n");
		return -EINVAL;
	}

	pdp = &pdp_devices[id];
	pdp->id = id;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "can't get memory resource\n");
		return -ENODEV;
	}

	if (!devm_request_mem_region(dev, res->start, resource_size(res),
				dev_name(dev))) {
		dev_err(dev, "can't request region for resource %pR\n", res);
		return -EBUSY;
	}

	pdp->regs_start = res->start;
	pdp->regs_end = res->end;

	pdp->base = devm_ioremap_nocache(dev, res->start, resource_size(res));
	if (!pdp->base) {
		dev_err(dev, "ioremap failed\n");
		ret = -ENOMEM;
		goto err_ioremap;
	}

	pdp->irq = platform_get_irq(pdev, 0);
	if (pdp->irq < 0) {
		dev_err(dev, "failed to get IRQ resource: %d\n", pdp->irq);
		ret = pdp->irq;
		goto err_get_irq;
	}

	ret = devm_request_irq(dev, pdp->irq, fimc_is_isr_pdp,
			IRQF_SHARED | FIMC_IS_HW_IRQ_FLAG,
			dev_name(dev), pdp);
	if (ret) {
		dev_err(dev, "failed to request IRQ(%d): %d\n", pdp->irq, ret);
		goto err_req_irq;
	}

	pdp->subdev = devm_kzalloc(&pdev->dev, sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!pdp->subdev) {
		dev_err(dev, "failed to alloc memory for pdp-subdev\n");
		ret = -ENOMEM;
		goto err_alloc_subdev;
	}

	v4l2_subdev_init(pdp->subdev, &subdev_ops);
	v4l2_set_subdevdata(pdp->subdev, pdp);
	snprintf(pdp->subdev->name, V4L2_SUBDEV_NAME_SIZE, "pdp-subdev.%d", pdp->id);

	pdp->pdp_ops = &pdp_ops;

	mutex_init(&pdp->control_lock);

	pdp->wq_stat0 = alloc_workqueue("pdp-stat0/[H/U]", WQ_HIGHPRI | WQ_UNBOUND, 0);
	if (!pdp->wq_stat0)
		probe_warn("failed to alloc PDP own workqueue, will be use global one");

	platform_set_drvdata(pdev, pdp);
	probe_info("%s device probe success\n", dev_name(dev));

	return 0;

err_alloc_subdev:
	devm_free_irq(dev, pdp->irq, pdp);
err_req_irq:
err_get_irq:
	devm_iounmap(dev, pdp->base);
err_ioremap:
	devm_release_mem_region(dev, res->start, resource_size(res));

	return ret;
}

static const struct of_device_id sensor_paf_pdp_match[] = {
	{
		.compatible = "samsung,sensor-paf-pdp",
	},
	{},
};
MODULE_DEVICE_TABLE(of, sensor_paf_pdp_match);

static struct platform_driver sensor_paf_pdp_platform_driver = {
	.driver = {
		.name   = "Sensor-PAF-PDP",
		.owner  = THIS_MODULE,
		.of_match_table = sensor_paf_pdp_match,
	}
};

static int __init sensor_paf_pdp_init(void)
{
	int ret;

	ret = platform_driver_probe(&sensor_paf_pdp_platform_driver, pdp_probe);
	if (ret)
		err("failed to probe %s driver: %d\n",
			sensor_paf_pdp_platform_driver.driver.name, ret);

	return ret;
}
late_initcall_sync(sensor_paf_pdp_init);
