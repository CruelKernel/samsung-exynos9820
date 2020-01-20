/*
 * Samsung Exynos5 SoC series Sensor driver
 *
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/of.h>

#include <media/v4l2-subdev.h>

#include "fimc-is-config.h"
#include "fimc-is-pafstat.h"
#include "fimc-is-hw-pafstat.h"
#include "fimc-is-interface-library.h"

static struct fimc_is_pafstat pafstat_device[MAX_NUM_OF_PAFSTAT];
atomic_t	g_pafstat_rsccount;

static void prepare_pafstat_sfr_dump(struct fimc_is_pafstat *pafstat)
{
	int reg_size = 0;

	if (IS_ERR_OR_NULL(pafstat->regs_b) ||
		(pafstat->regs_b_start == 0) ||
		(pafstat->regs_b_end == 0)) {
		warn("[PAFSTAT:%d]reg iomem is invalid", pafstat->id);
		return;
	}

	/* alloc sfr dump memory */
	reg_size = (pafstat->regs_end - pafstat->regs_start + 1);
	pafstat->sfr_dump = kzalloc(reg_size, GFP_KERNEL);
	if (IS_ERR_OR_NULL(pafstat->sfr_dump))
		err("[PAFSTAT:%d]sfr dump memory alloc fail", pafstat->id);
	else
		info("[PAFSTAT:%d]sfr dump memory (V/P/S):(%p/%p/0x%X)[0x%llX~0x%llX]\n", pafstat->id,
				pafstat->sfr_dump, (void *)virt_to_phys(pafstat->sfr_dump),
				reg_size, pafstat->regs_start, pafstat->regs_end);

	if (IS_ERR_OR_NULL(pafstat->regs_b) ||
		(pafstat->regs_b_start == 0) ||
		(pafstat->regs_b_end == 0))
		return;

	/* alloc sfr B dump memory */
	reg_size = (pafstat->regs_b_end - pafstat->regs_b_start + 1);
	pafstat->sfr_b_dump = kzalloc(reg_size, GFP_KERNEL);
	if (IS_ERR_OR_NULL(pafstat->sfr_b_dump))
		err("[PAFSTAT:%d]sfr B dump memory alloc fail", pafstat->id);
	else
		info("[PAFSTAT:%d]sfr B dump memory (V/P/S):(%p/%p/0x%X)[0x%llX~0x%llX]\n", pafstat->id,
				pafstat->sfr_b_dump, (void *)virt_to_phys(pafstat->sfr_b_dump),
				reg_size, pafstat->regs_b_start, pafstat->regs_b_end);
}

void pafstat_sfr_dump(struct fimc_is_pafstat *pafstat)
{
	int reg_size = 0;

	reg_size = (pafstat->regs_end - pafstat->regs_start + 1);
	memcpy(pafstat->sfr_dump, pafstat->regs, reg_size);
	info("[PAFSTAT:%d]##### SFR DUMP(V/P/S):(%p/%p/0x%X)[0x%llX~0x%llX]\n", pafstat->id,
			pafstat->sfr_dump, (void *)virt_to_phys(pafstat->sfr_dump),
			reg_size, pafstat->regs_start, pafstat->regs_end);
#ifdef ENABLE_PANIC_SFR_PRINT
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4,
			pafstat->regs, reg_size, false);
#endif

	/* dump reg B */
	reg_size = (pafstat->regs_b_end - pafstat->regs_b_start + 1);
	memcpy(pafstat->sfr_b_dump, pafstat->regs_b, reg_size);

	info("[PAFSTAT:%d]##### SFR B DUMP(V/P/S):(%p/%p/0x%X)[0x%llX~0x%llX]\n", pafstat->id,
			pafstat->sfr_b_dump, (void *)virt_to_phys(pafstat->sfr_b_dump),
			reg_size, pafstat->regs_b_start, pafstat->regs_b_end);
#ifdef ENABLE_PANIC_SFR_PRINT
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_OFFSET, 32, 4,
			pafstat->regs_b, reg_size, false);
#endif
}

static irqreturn_t fimc_is_isr_pafstat(int irq, void *data)
{
	struct fimc_is_pafstat *pafstat;
	u32 irq_src, irq_mask, status;
	bool err_intr_flag = false;
	u32 ret;

	pafstat = data;
	if (pafstat == NULL)
		return IRQ_NONE;

	irq_src = pafstat_hw_g_irq_src(pafstat->regs);
	irq_mask = pafstat_hw_g_irq_mask(pafstat->regs);
	status = (~irq_mask) & irq_src;

	pafstat_hw_s_irq_src(pafstat->regs, status);

	if (status & (1 << PAFSTAT_INT_FRAME_START)) {
		u32 __iomem *base_reg = pafstat->regs;

		atomic_set(&pafstat->Vvalid, V_VALID);
#if 0 /* TODO */
		if (pafstat_g_current(base_reg) == PAFSTAT_SEL_SET_A)
			base_reg = pafstat->regs_b;
		else
			base_reg = pafstat->regs;
#endif

		pafstat_hw_s_img_size(base_reg, pafstat->in_width, pafstat->in_height);
		pafstat_hw_s_pd_size(base_reg, pafstat->pd_width, pafstat->pd_height);
		if (atomic_read(&pafstat->sfr_state)) { /* TODO */
			u32 reg_idx;

			for (reg_idx = 0; reg_idx < pafstat->regs_max; reg_idx++) {
				if (pafstat->regs_set[reg_idx].reg_addr)
					writel(pafstat->regs_set[reg_idx].reg_data,
							base_reg + pafstat->regs_set[reg_idx].reg_addr);
			}
		}

		ret = pafstat_hw_g_ready(base_reg);
		if (!ret)
			atomic_set(&pafstat->sfr_state, PAFSTAT_SFR_APPLIED);
		pafstat_hw_s_ready(base_reg, 1);

		atomic_inc(&pafstat->fs);
		dbg_isr("[%d][F:%d] F.S (0x%x)", pafstat, pafstat->id, atomic_read(&pafstat->fs), status);
		atomic_add(pafstat->fro_cnt, &pafstat->fs);
	}

	if (status & (1 << PAFSTAT_INT_TIMEOUT)) {
		err("[PAFSTAT:%d] TIMEOUT (0x%x)", pafstat->id, status);
	}

	if (status & (1 << PAFSTAT_INT_BAYER_FRAME_END)) {
		atomic_inc(&pafstat->fe_img);
		dbg_isr("[%d][F:%d] F.E, img (0x%x)", pafstat, pafstat->id, atomic_read(&pafstat->fe_img), status);
		atomic_add(pafstat->fro_cnt, &pafstat->fe_img);
	}

	if (status & (1 << PAFSTAT_INT_STAT_FRAME_END)) {
		atomic_inc(&pafstat->fe_stat);
		dbg_isr("[%d][F:%d] F.E, stat (0x%x)", pafstat, pafstat->id, atomic_read(&pafstat->fe_stat), status);
		atomic_add(pafstat->fro_cnt, &pafstat->fe_stat);
	}

	if (status & (1 << PAFSTAT_INT_TOTAL_FRAME_END)) {
		atomic_inc(&pafstat->fe);
		dbg_isr("[%d][F:%d] F.E (0x%x)", pafstat, pafstat->id, atomic_read(&pafstat->fe), status);
		atomic_add(pafstat->fro_cnt, &pafstat->fe);
		pafstat_hw_s_timeout_cnt_clear(pafstat->regs);
		atomic_set(&pafstat->Vvalid, V_BLANK);
		wake_up(&pafstat->wait_queue);
	}

	if (status & (1 << PAFSTAT_INT_FRAME_LINE)) {
		atomic_inc(&pafstat->cl);
		dbg_sensor(5, "[PAFSTAT:%d] LINE interrupt (0x%x)", pafstat->id, status);
		atomic_add(pafstat->fro_cnt, &pafstat->cl);
	}

	if (status & (1 << PAFSTAT_INT_FRAME_FAIL)) {
		err("[PAFSTAT:%d] DMA OVERFLOW (0x%x)", pafstat->id, status);
		err_intr_flag = true;
	}

	if (status & (1 << PAFSTAT_INT_LIC_BUFFER_FULL)) {
		err("[PAFSTAT:%d] LIC BUFFER FULL (0x%x)", pafstat->id, status);
		err_intr_flag = true;
		/* TODO: recovery */
#ifdef ENABLE_FULLCHAIN_OVERFLOW_RECOVERY
		fimc_is_hw_overflow_recovery();
#endif
	}
#if 0 /* TODO */
	if (err_intr_flag) {
		pafstat_sfr_dump(pafstat);
		pafstat_hw_com_s_output_mask(pafstat->regs_com, 1);
	}
#endif

	return IRQ_HANDLED;
}

int pafstat_set_num_buffers(struct v4l2_subdev *subdev, u32 num_buffers, u32 mipi_speed)
{
	struct fimc_is_pafstat *pafstat;

	pafstat = v4l2_get_subdevdata(subdev);
	if (!pafstat) {
		err("A subdev data of PAFSTAT is null");
		return -ENODEV;
	}

	pafstat->fro_cnt = (num_buffers > 8 ? 7 : num_buffers - 1);
	pafstat_hw_com_s_fro(pafstat->regs_com, pafstat->fro_cnt);
	pafstat_hw_s_4ppc(pafstat->regs, mipi_speed);

	info("[PAFSTAT:%d] fro_cnt(%d,%d)\n", pafstat->id, pafstat->fro_cnt, num_buffers);

	return 0;
}

int pafstat_hw_set_regs(struct v4l2_subdev *subdev,
		struct paf_setting_t *regs, u32 regs_cnt)
{
	struct fimc_is_pafstat *pafstat;

	pafstat = v4l2_get_subdevdata(subdev);
	if (!pafstat) {
		err("A subdev data of PAFSTAT is null");
		return -ENODEV;
	}

	memcpy(pafstat->regs_set, regs, regs_cnt * sizeof(struct paf_setting_t));
	pafstat->regs_max = regs_cnt;
	atomic_set(&pafstat->sfr_state, PAFSTAT_SFR_UNAPPLIED);

	return 0;
}

int pafstat_hw_get_ready(struct v4l2_subdev *subdev, u32 *ready)
{
	struct fimc_is_pafstat *pafstat;

	pafstat = v4l2_get_subdevdata(subdev);
	if (!pafstat) {
		err("A subdev data of PAFSTAT is null");
		return -ENODEV;
	}

	*ready = (u32)atomic_read(&pafstat->sfr_state);

	return 0;
}

int pafstat_register(struct fimc_is_module_enum *module, int pafstat_ch)
{
	int ret = 0;
	struct fimc_is_pafstat *pafstat;
	struct fimc_is_device_sensor_peri *sensor_peri = module->private_data;
	u32 version = 0;

	if (test_bit(FIMC_IS_SENSOR_PAFSTAT_AVAILABLE, &sensor_peri->peri_state)) {
		err("already registered");
		ret = -EINVAL;
		goto p_err;
	}

	if (pafstat_ch >= MAX_NUM_OF_PAFSTAT) {
		err("A pafstat channel is invalide");
		ret = -EINVAL;
		goto p_err;
	}

	pafstat = &pafstat_device[pafstat_ch];
	sensor_peri->pafstat = pafstat;
	sensor_peri->subdev_pafstat = pafstat->subdev;
	v4l2_set_subdev_hostdata(pafstat->subdev, module);

	atomic_set(&pafstat->sfr_state, PAFSTAT_SFR_UNAPPLIED);
	atomic_set(&pafstat->fs, 0);
	atomic_set(&pafstat->cl, 0);
	atomic_set(&pafstat->fe, 0);
	atomic_set(&pafstat->fe_img, 0);
	atomic_set(&pafstat->fe_stat, 0);
	atomic_set(&pafstat->Vvalid, V_BLANK);
	init_waitqueue_head(&pafstat->wait_queue);

	pafstat->regs_com = pafstat_device[0].regs;
	if (!atomic_read(&g_pafstat_rsccount)) {
		info("[PAFSTAT:%d] %s: hw_com_init()\n", pafstat->id, __func__);
		pafstat_hw_com_init(pafstat->regs_com);
	}
	atomic_inc(&g_pafstat_rsccount);
	info("[PAFSTAT:%d] %s: pafstat_hw_s_init(rsccount:%d)\n",
			pafstat->id, __func__, (u32)atomic_read(&g_pafstat_rsccount));

	set_bit(FIMC_IS_SENSOR_PAFSTAT_AVAILABLE, &sensor_peri->peri_state);
	version = pafstat_hw_com_g_version(pafstat->regs_com);
	info("[PAFSTAT:%d] %s: (HW_VER:0x%x)\n", pafstat->id, __func__, version);

	return ret;
p_err:
	return ret;
}

int pafstat_unregister(struct fimc_is_module_enum *module)
{
	int ret = 0;
	struct fimc_is_device_sensor_peri *sensor_peri = module->private_data;
	struct fimc_is_pafstat *pafstat;
	long timetowait;

	if (!test_bit(FIMC_IS_SENSOR_PAFSTAT_AVAILABLE, &sensor_peri->peri_state)) {
		err("already unregistered");
		ret = -EINVAL;
		goto p_err;
	}

	pafstat = v4l2_get_subdevdata(sensor_peri->subdev_pafstat);
	if (!pafstat) {
		err("A subdev data of PAFSTAT is null");
		ret = -ENODEV;
		goto p_err;
	}

	sensor_peri->pafstat = NULL;
	sensor_peri->subdev_pafstat = NULL;
	pafstat_hw_s_enable(pafstat->regs, 0);

	timetowait = wait_event_timeout(pafstat->wait_queue,
		!atomic_read(&pafstat->Vvalid),
		FIMC_IS_HW_STOP_TIMEOUT);

	if (!timetowait) {
		err("[PAFSTAT:%d]wait FRAME_END timeout (%ld), fro_cnt(%d)",
			pafstat->id, timetowait, pafstat->fro_cnt);
		ret = -ETIME;
	}

	clear_bit(FIMC_IS_SENSOR_PAFSTAT_AVAILABLE, &sensor_peri->peri_state);
	atomic_dec(&g_pafstat_rsccount);

	info("[PAFSTAT:%d] %s(ret:%d)\n", pafstat->id, __func__, ret);
	return ret;
p_err:
	return ret;
}

int pafstat_init(struct v4l2_subdev *subdev, u32 val)
{
	return 0;
}

static int pafstat_s_stream(struct v4l2_subdev *subdev, int pd_mode)
{
	int irq_state = 0;
	int enable = 0;
	struct fimc_is_pafstat *pafstat;
	struct fimc_is_device_sensor_peri *sensor_peri;
	struct fimc_is_module_enum *module;
	cis_shared_data *cis_data = NULL;
	u32 lic_mode;
	enum pafstat_input_path input = PAFSTAT_INPUT_OTF;

	module = (struct fimc_is_module_enum *)v4l2_get_subdev_hostdata(subdev);
	WARN_ON(!module);

	sensor_peri = module->private_data;
	WARN_ON(!sensor_peri);

	cis_data = sensor_peri->cis.cis_data;
	WARN_ON(!cis_data);

	pafstat = v4l2_get_subdevdata(subdev);
	if (!pafstat) {
		err("A subdev data of PAFSTAT is null");
		return -ENODEV;
	}

	enable = pafstat_hw_s_sensor_mode(pafstat->regs, pd_mode);
	pafstat_hw_s_irq_mask(pafstat->regs, PAFSTAT_INT_MASK);

	cis_data->is_data.paf_stat_enable = enable;
	irq_state = pafstat_hw_g_irq_src(pafstat->regs);

	lic_mode = (pafstat->fro_cnt == 0 ? LIC_MODE_INTERLEAVING : LIC_MODE_SINGLE_BUFFER);
	pafstat_hw_com_s_lic_mode(pafstat->regs_com, pafstat->id, lic_mode, input);
	pafstat_hw_com_s_output_mask(pafstat->regs_com, 0);
	pafstat_hw_s_input_path(pafstat->regs, input);
	pafstat_hw_s_img_size(pafstat->regs, pafstat->in_width, pafstat->in_height);

	pafstat_hw_s_ready(pafstat->regs, 1);
	pafstat_hw_s_enable(pafstat->regs, 1);

	info("[PAFSTAT:%d] PD_MODE:%d, HW_ENABLE:%d, IRQ:0x%x, IRQ_MASK:0x%x, LIC_MODE(%s)\n",
		pafstat->id, pd_mode, enable, irq_state, PAFSTAT_INT_MASK,
		lic_mode == LIC_MODE_INTERLEAVING ? "INTERLEAVING": "SINGLE_BUFFER");

	return 0;
}

static int pafstat_s_param(struct v4l2_subdev *subdev, struct v4l2_streamparm *param)
{
	return 0;
}

static int pafstat_s_format(struct v4l2_subdev *subdev,
	struct v4l2_subdev_pad_config *cfg,
	struct v4l2_subdev_format *fmt)
{
	int ret = 0;
	size_t width, height;
	struct fimc_is_pafstat *pafstat;
	struct fimc_is_module_enum *module;
	struct fimc_is_device_sensor *sensor = NULL;

	pafstat = v4l2_get_subdevdata(subdev);
	if (!pafstat) {
		err("A subdev data of PAFSTAT is null");
		ret = -ENODEV;
		goto p_err;
	}

	width = fmt->format.width;
	height = fmt->format.height;
	pafstat->in_width = width;
	pafstat->in_height = height;
	pafstat_hw_s_img_size(pafstat->regs, pafstat->in_width, pafstat->in_height);

	module = (struct fimc_is_module_enum *)v4l2_get_subdev_hostdata(subdev);
	if (!module) {
		err("[PAFSTAT:%d] A host data of PAFSTAT is null", pafstat->id);
		ret = -ENODEV;
		goto p_err;
	}

	sensor = (struct fimc_is_device_sensor *)v4l2_get_subdev_hostdata(module->subdev);
	if (!sensor) {
		err("device_sensor is null");
		ret = -ENODEV;
		goto p_err;
	}

	if (sensor->cfg) {
		pafstat->pd_width = sensor->cfg->input[CSI_VIRTUAL_CH_1].width;
		pafstat->pd_height = sensor->cfg->input[CSI_VIRTUAL_CH_1].height;
		pafstat_hw_s_pd_size(pafstat->regs, pafstat->pd_width, pafstat->pd_height);
	}

	info("[PAFSTAT:%d] %s: image_size(%lux%lu) pd_size(%lux%lu) ret(%d)\n", pafstat->id, __func__,
		width, height, pafstat->pd_width, pafstat->pd_height, ret);

p_err:
	return ret;
}

int fimc_is_pafstat_reset_recovery(struct v4l2_subdev *subdev, u32 reset_mode, int pd_mode)
{
	int ret = 0;
	struct fimc_is_pafstat *pafstat;

	pafstat = v4l2_get_subdevdata(subdev);
	if (!pafstat) {
		err("A subdev data of PAFSTAT is null");
		return -EINVAL;
	}

	if (reset_mode == 0) {	/* reset */
		pafstat_hw_com_s_output_mask(pafstat->regs_com, 1);
		pafstat_hw_sw_reset(pafstat->regs);
	} else {
		pafstat_s_stream(subdev, pd_mode);
		pafstat_hw_com_s_output_mask(pafstat->regs_com, 0);
	}

	return ret;
}

static const struct v4l2_subdev_core_ops core_ops = {
	.init = pafstat_init
};

static const struct v4l2_subdev_video_ops video_ops = {
	.s_stream = pafstat_s_stream,
	.s_parm = pafstat_s_param,
};

static const struct v4l2_subdev_pad_ops pad_ops = {
	.set_fmt = pafstat_s_format
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
	.video = &video_ops,
	.pad = &pad_ops
};

struct fimc_is_pafstat_ops pafstat_ops = {
	.set_param = pafstat_hw_set_regs,
	.get_ready = pafstat_hw_get_ready,
	.set_num_buffers = pafstat_set_num_buffers,
};

static int __init pafstat_probe(struct platform_device *pdev)
{
	int ret = 0;
	int id = -1;
	struct resource *mem_res;
	struct fimc_is_pafstat *pafstat;
	struct device_node *dnode;
	struct device *dev;
	u32 reg_cnt = 0;
	char irq_name[16];

	WARN_ON(!fimc_is_dev);
	WARN_ON(!pdev);
	WARN_ON(!pdev->dev.of_node);

	dev = &pdev->dev;
	dnode = dev->of_node;

	ret = of_property_read_u32(dnode, "id", &pdev->id);
	if (ret) {
		dev_err(dev, "id read is fail(%d)\n", ret);
		goto err_get_id;
	}

	id = pdev->id;
	pafstat = &pafstat_device[id];

	/* Get SFR base register */
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem_res) {
		dev_err(dev, "Failed to get io memory region(%p)\n", mem_res);
		ret = -EBUSY;
		goto err_get_resource;
	}

	pafstat->regs_start = mem_res->start;
	pafstat->regs_end = mem_res->end;
	pafstat->regs = devm_ioremap_nocache(&pdev->dev, mem_res->start, resource_size(mem_res));
	if (!pafstat->regs) {
		dev_err(dev, "Failed to remap io region(%p)\n", pafstat->regs);
		ret = -ENOMEM;
		goto err_ioremap;
	}

	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!mem_res) {
		dev_err(dev, "Failed to get io memory region(%p)\n", mem_res);
		ret = -EBUSY;
		goto err_get_resource;
	}

	pafstat->regs_b_start = mem_res->start;
	pafstat->regs_b_end = mem_res->end;
	pafstat->regs_b = devm_ioremap_nocache(&pdev->dev, mem_res->start, resource_size(mem_res));
	if (!pafstat->regs_b) {
		dev_err(dev, "Failed to remap io region(%p)\n", pafstat->regs_b);
		ret = -ENOMEM;
		goto err_ioremap;
	}

	/* Get CORE IRQ SPI number */
	pafstat->irq = platform_get_irq(pdev, 0);
	if (pafstat->irq < 0) {
		dev_err(dev, "Failed to get pafstat_irq(%d)\n", pafstat->irq);
		ret = -EBUSY;
		goto err_get_irq;
	}

	snprintf(irq_name, sizeof(irq_name), "pafstat%d", id);
	ret = request_irq(pafstat->irq,
			fimc_is_isr_pafstat,
			FIMC_IS_HW_IRQ_FLAG | IRQF_SHARED,
			irq_name,
			pafstat);
	if (ret) {
		dev_err(dev, "request_irq(IRQ_PAFSTAT %d) is fail(%d)\n", pafstat->irq, ret);
		goto err_get_irq;
	}

	pafstat->id = id;
	platform_set_drvdata(pdev, pafstat);

	pafstat->subdev = devm_kzalloc(&pdev->dev, sizeof(struct v4l2_subdev), GFP_KERNEL);
	if (!pafstat->subdev) {
		ret = -ENOMEM;
		goto err_subdev_alloc;
	}

	snprintf(pafstat->name, FIMC_IS_STR_LEN, "PAFSTAT%d", pafstat->id);
	reg_cnt = pafstat_hw_g_reg_cnt();
	pafstat->regs_set = devm_kzalloc(&pdev->dev, reg_cnt * sizeof(struct paf_setting_t), GFP_KERNEL);
	if (!pafstat->regs_set) {
		probe_err("pafstat->reg_set NULL");
		ret = -ENOMEM;
		goto err_reg_cnt_alloc;
	}

	v4l2_subdev_init(pafstat->subdev, &subdev_ops);

	v4l2_set_subdevdata(pafstat->subdev, pafstat);
	snprintf(pafstat->subdev->name, V4L2_SUBDEV_NAME_SIZE, "pafstat-subdev.%d", pafstat->id);

	pafstat->pafstat_ops = &pafstat_ops;

	prepare_pafstat_sfr_dump(pafstat);
	atomic_set(&g_pafstat_rsccount, 0);

	probe_info("%s(%s)\n", __func__, dev_name(&pdev->dev));
	return ret;

err_reg_cnt_alloc:
err_subdev_alloc:
err_get_irq:
err_ioremap:
err_get_resource:
err_get_id:
	return ret;
}

static const struct of_device_id exynos_fimc_is_pafstat_match[] = {
	{
		.compatible = "samsung,exynos5-fimc-is-pafstat",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_fimc_is_pafstat_match);

static struct platform_driver pafstat_driver = {
	.driver = {
		.name   = "FIMC-IS-PAFSTAT",
		.owner  = THIS_MODULE,
		.of_match_table = exynos_fimc_is_pafstat_match,
	}
};
builtin_platform_driver_probe(pafstat_driver, pafstat_probe);
