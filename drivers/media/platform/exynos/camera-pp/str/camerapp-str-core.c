/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Core file for Samsung EXYNOS ISPP GDC driver
 * (FIMC-IS PostProcessing Generic Distortion Correction driver)
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/exynos_iovmm.h>
#include <linux/ion_exynos.h>

#include "camerapp-str-hw.h"
#include "camerapp-str-core.h"

static int _str_clk_get(struct str_core *core)
{
	long ret;

	core->clk_mux = devm_clk_get(core->dev, "UMUX_CLKCMU_VRA2_STR");
	if (IS_ERR(core->clk_mux)) {
		ret = PTR_ERR(core->clk_mux);
		if (ret != -ENOENT) {
			dev_err(core->dev, "failed to get UMUX_CLKCMU_VRA2_STR. ret(%ld)\n", ret);
			return ret;
		}
		dev_info(core->dev, "UMUX_CLKCMU_VRA2_STR is NOT present.\n");
	}

	core->clk_gate = devm_clk_get(core->dev, "GATE_STR");
	if (IS_ERR(core->clk_gate)) {
		ret = PTR_ERR(core->clk_gate);
		if (ret != -ENOENT) {
			dev_err(core->dev, "failed to get GATE_STR. ret(%ld)\n", ret);
			return ret;
		}
		dev_info(core->dev, "GATE_STR is NOT present.\n");
	}

	return 0;
}

static void _str_clk_put(struct str_core *core)
{
	if (!IS_ERR_OR_NULL(core->clk_gate))
		devm_clk_put(core->dev, core->clk_gate);

	if (!IS_ERR_OR_NULL(core->clk_mux))
		devm_clk_put(core->dev, core->clk_mux);
}

static int _str_clk_prepare_enable(struct str_core *core)
{
	int ret = 0;

	if (!IS_ERR_OR_NULL(core->clk_mux)) {
		ret = clk_prepare_enable(core->clk_mux);
		if (ret) {
			dev_err(core->dev, "Failed to enable clk_mux. ret(%d)\n", ret);
			goto err_exit;
		}
	}

	if (!IS_ERR_OR_NULL(core->clk_gate)) {
		ret = clk_prepare_enable(core->clk_gate);
		if (ret) {
			dev_err(core->dev, "Failed to enable clk_gate. ret(%d)\n", ret);
			goto err_clk_gate;
		}
	}

	return 0;
err_clk_gate:
	if (!IS_ERR_OR_NULL(core->clk_mux))
		clk_disable_unprepare(core->clk_mux);
err_exit:
	return ret;
}

static void _str_clk_disable_unprepare(struct str_core *core)
{
	if (!IS_ERR_OR_NULL(core->clk_gate))
		clk_disable_unprepare(core->clk_gate);

	if (!IS_ERR_OR_NULL(core->clk_mux))
		clk_disable_unprepare(core->clk_mux);
}

static void _str_ion_init(struct device *dev, struct str_ion_ctx *ion_ctx)
{
	ion_ctx->dev = dev;
	ion_ctx->align = SZ_4K;
	ion_ctx->flags = ION_FLAG_CACHED;
	mutex_init(&ion_ctx->lock);
}

static void _str_ion_deinit(struct str_ion_ctx *ion_ctx)
{
	mutex_destroy(&ion_ctx->lock);
}

struct str_priv_buf *str_ion_alloc(struct str_ion_ctx *ion_ctx, size_t size)
{
	int ret = 0;
	const char *heapname = "ion_system_heap";
	struct str_priv_buf *buf;

	buf = vzalloc(sizeof(*buf));
	if (!buf)
		return ERR_PTR(-ENOMEM);

	buf->size = PAGE_ALIGN(size);
	buf->direction = DMA_BIDIRECTIONAL;
	buf->ion_ctx = ion_ctx;

	buf->dma_buf = ion_alloc_dmabuf(heapname, buf->size, ion_ctx->flags);
	if (IS_ERR(buf->dma_buf)) {
		ret = -ENOMEM;
		goto err_alloc;
	}

	buf->dma_attach= dma_buf_attach(buf->dma_buf, ion_ctx->dev);
	if (IS_ERR(buf->dma_attach)) {
		ret = PTR_ERR(buf->dma_attach);
		goto err_attach;
	}

	buf->sgt = dma_buf_map_attachment(buf->dma_attach, buf->direction);
	if (IS_ERR(buf->sgt)) {
		ret = PTR_ERR(buf->sgt);
		goto err_map_dmabuf;
	}

	mutex_lock(&ion_ctx->lock);
	buf->iova = ion_iovmm_map(buf->dma_attach, 0, buf->size, buf->direction, 0);
	if (IS_ERR_VALUE(buf->iova)) {
		ret = (int) buf->iova;
		mutex_unlock(&ion_ctx->lock);
		goto err_ion_map_io;
	}
	mutex_unlock(&ion_ctx->lock);

	return buf;

err_ion_map_io:
	dma_buf_unmap_attachment(buf->dma_attach, buf->sgt, buf->direction);

err_map_dmabuf:
	dma_buf_detach(buf->dma_buf, buf->dma_attach);

err_attach:
	dma_buf_put(buf->dma_buf);

err_alloc:
	vfree(buf);

	pr_err("[STR:MEM][%s]Failed to allocate ION memory. size(%zu)\n", __func__, buf->size);
	return ERR_PTR(ret);
}

void str_ion_free(struct str_priv_buf *buf)
{
	struct str_ion_ctx *ion_ctx = buf->ion_ctx;

	mutex_lock(&ion_ctx->lock);
	if (buf->iova)
		ion_iovmm_unmap(buf->dma_attach, buf->iova);
	mutex_unlock(&ion_ctx->lock);

	dma_buf_unmap_attachment(buf->dma_attach, buf->sgt, buf->direction);
	dma_buf_detach(buf->dma_buf, buf->dma_attach);
	dma_buf_put(buf->dma_buf);

	vfree(buf);
}

static int __attribute__((unused)) _str_sysmmu_falut_handler(struct iommu_domain *domain,
		struct device *dev, unsigned long iova, int flags, void *token)
{
	return 0;
}

int str_power_clk_enable(struct str_core *core)
{
	int ret = 0;

	ret = pm_runtime_get_sync(core->dev);
	if (ret < 0) {
		dev_err(core->dev, "failed to pm_runtime_get_sync. ret(%d)\n", ret);
		goto err_exit;
	}

	ret = _str_clk_prepare_enable(core);
	if (ret) {
		dev_err(core->dev, "failed to str_clk_prepare_enable. ret(%d)\n", ret);
		goto err_clk_enable;
	}

	return 0;
err_clk_enable:
	pm_runtime_put_sync(core->dev);
err_exit:
	return ret;
}

int str_power_clk_disable(struct str_core *core)
{
	int ret = 0;

	_str_clk_disable_unprepare(core);

	ret = pm_runtime_put_sync(core->dev);
	if (ret < 0) {
		dev_err(core->dev, "failed to pm_runtime_put_sync. ret(%d)\n", ret);
	}

	return ret;
}

int str_shot(struct str_ctx *ctx)
{
	struct str_core *core;
	struct str_hw *hw;
	struct str_vb2_buffer *vb2_buf;
	struct str_priv_buf *priv_buf;
	struct str_frame_cfg *frame;
	struct str_metadata *meta;

	core = video_to_core(ctx->video);
	hw = &core->hw;
	vb2_buf = ctx->buf;
	frame = &ctx->frame_cfg;
	meta = frame->meta;

	if (meta->face_array.length <= 0) {
		pr_info("[STR][%s]No face. Skip.\n", __func__);
		vb2_buffer_done(&vb2_buf->vb2_buf, VB2_BUF_STATE_DONE);
		goto func_exit;
	}

	/* Set device virtual addresses for image planes */
	hw->image_y_addr.addr = vb2_buf->dva[0];
	hw->image_y_addr.size = vb2_buf->size[0];
	hw->image_c_addr.addr = vb2_buf->dva[1];
	hw->image_c_addr.size = vb2_buf->size[1];

	/* Set device virtual addresses for internal memory */
	priv_buf = ctx->buf_y;
	hw->roi_y_addr.addr = priv_buf->iova;
	hw->roi_y_addr.size = priv_buf->size;
	priv_buf = ctx->buf_c;
	hw->roi_c_addr.addr = priv_buf->iova;
	hw->roi_c_addr.size = priv_buf->size;

	/* Initialize the reference model parameters for STRA. */
	str_hw_init_ref_model(hw);

	/* Initialize the group parameters for STRB. */
	pr_info("[STR][%s]region_index(%d)\n", __func__, meta->region_index);
	str_hw_init_group(hw, meta->region_index);

	/* Set image size. */
	pr_info("[STR][%s]image size(%dx%d)\n", __func__, frame->width, frame->height);
	str_hw_set_size(hw, frame->width, frame->height);

	/* Set lens position & device orientation. */
	pr_info("[STR][%s]lens_pos(%d) device_ori(%d)\n", __func__, meta->lens_position, meta->device_orientation);
	str_hw_set_direction(hw, meta->lens_position, meta->device_orientation);

	/* Set preview flag */
	pr_info("[STR][%s]is_preview(%d)\n", __func__, meta->is_preview);
	str_hw_set_preview_flag(hw, meta->is_preview);

	/* Start H/W configuration for current image */
	str_hw_set_cmdq_mode(hw, true);

	/* PU/DMA configuration per face */
	for (int face_id = 0; face_id < meta->face_array.length; face_id++) {
		/* Set face region & ROI */
		pr_info("[STR][%s]id(%d) region(%d,%d %d,%d)\n", __func__, face_id,
				meta->face_array.faces[face_id].left, meta->face_array.faces[face_id].top,
				meta->face_array.faces[face_id].right, meta->face_array.faces[face_id].bottom);
		str_hw_set_face(hw,
				meta->face_array.faces[face_id].left, meta->face_array.faces[face_id].top,
				meta->face_array.faces[face_id].right, meta->face_array.faces[face_id].bottom,
				face_id);

		/* Update PU control parameters */
		str_hw_stra_update_param(hw);
		str_hw_strb_update_param(hw);
		str_hw_strc_update_param(hw);
		str_hw_strd_update_param(hw);

		/* STRA configuration */
		str_hw_stra_dxi_mux_cfg(hw);
		str_hw_stra_dma_cfg(hw);
		str_hw_stra_ctrl_cfg(hw);

		/* STRB configuration */
		str_hw_strb_dxi_mux_cfg(hw);
		str_hw_strb_dma_cfg(hw);
		str_hw_strb_ctrl_cfg(hw);

		/* STRB 2nd configuration */
		str_hw_strb_set_second_mode(hw);

		/* STRC configuration */
		str_hw_strc_dxi_mux_cfg(hw);
		str_hw_strc_dma_cfg(hw);
		str_hw_strc_ctrl_cfg(hw);

		/* STRD configuration */
		str_hw_strd_dxi_mux_cfg(hw);
		str_hw_strd_dma_cfg(hw);
		str_hw_strd_ctrl_cfg(hw);
	}

	/* Finish H/W configuration for current image */
	str_hw_set_cmdq_mode(hw, false);

	/* Enable Interrupt */
	str_hw_frame_done_irq_enable(hw);

	/* Start STR */
	pr_info("[STR][%s]start(%d)\n", __func__, meta->face_array.length);
	str_hw_str_start(hw, meta->face_array.length);

func_exit:
	return 0;
}

static irqreturn_t str_irq_handler(int irq, void *priv)
{
	struct str_core *core = priv;

	str_hw_update_irq_status(&core->hw);

	pr_err("[STR][%s]irq(%d): pu_tile_done(%x) pu_frame_done(%x) pu_err_done(%x)\n",
			__func__, irq,
			core->hw.status.pu_tile_done,
			core->hw.status.pu_frame_done,
			core->hw.status.pu_err_done);
	pr_err("[STR][%s]irq(%d): dma_tile_done(%x) dma_frame_done(%x) dma_err_done(%x)\n",
			__func__, irq,
			core->hw.status.dma_tile_done,
			core->hw.status.dma_frame_done,
			core->hw.status.dma_err_done);
	if (core->hw.status.dma_err_done) {
		pr_err("[STR][%s]RDMA00 err(%x)\n", __func__, core->hw.status.dma_err_status.rdma00);
		pr_err("[STR][%s]RDMA01 err(%x)\n", __func__, core->hw.status.dma_err_status.rdma01);
		pr_err("[STR][%s]RDMA02 err(%x)\n", __func__, core->hw.status.dma_err_status.rdma02);
		pr_err("[STR][%s]WDMA00 err(%x)\n", __func__, core->hw.status.dma_err_status.wdma00);
		pr_err("[STR][%s]WDMA01 err(%x)\n", __func__, core->hw.status.dma_err_status.wdma01);
		pr_err("[STR][%s]WDMA02 err(%x)\n", __func__, core->hw.status.dma_err_status.wdma02);
		pr_err("[STR][%s]WDMA03 err(%x)\n", __func__, core->hw.status.dma_err_status.wdma03);
	}
	pr_err("[STR][%s]irq(%d): dxi_tile_done(%x) dxi_frame_done(%x)\n",
			__func__, irq,
			core->hw.status.dxi_tile_done,
			core->hw.status.dxi_frame_done);

	return IRQ_HANDLED;
}

static int str_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct str_core *core;
	struct resource *rsc;

	core = devm_kzalloc(&pdev->dev, sizeof(struct str_core), GFP_KERNEL);
	if (!core)
		return -ENOMEM;

	core->dev = &pdev->dev;

	/* Get SFR */
	rsc = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	core->hw.regs = devm_ioremap_nocache(&pdev->dev, rsc->start, resource_size(rsc));
	if (IS_ERR(core->hw.regs))
		return PTR_ERR(core->hw.regs);

	pr_info("[STR][%s]SFR:start(%x) end(%x) size(%x)\n", __func__,
			rsc->start, rsc->end, resource_size(rsc));

	/* Get IRQ */
	rsc = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!rsc) {
		dev_err(core->dev, "failed to get IRQ resource\n");
		ret = -ENOENT;
		goto err_exit;
	}

	ret = devm_request_irq(core->dev, rsc->start, str_irq_handler, 0, pdev->name, core);
	if (ret) {
		dev_err(core->dev, "failed to reguest IRQ. ret(%d)\n", ret);
		goto err_exit;
	}

	pr_info("[STR][%s]IRQ:start(%d)\n", __func__, rsc->start);

	/* Get CLK */
	ret = _str_clk_get(core);
	if (ret) {
		dev_err(core->dev, "failed to get CLK resource\n");
		goto err_exit;
	}

	platform_set_drvdata(pdev, core);

	/* RPM Enable */
	pm_runtime_enable(core->dev);

	/* Register video device */
	ret = str_video_probe(core);
	if (ret) {
		dev_err(core->dev, "Failed to probe str_video. ret(%d)\n", ret);
		goto err_exit;
	}

	/* MMU Activation */
	ret = iovmm_activate(core->dev);
	if (ret) {
		dev_err(core->dev, "failed to attach iommu. ret(%d)\n", ret);
		goto err_exit;
	}

	/* Init ION context */
	_str_ion_init(core->dev, &core->ion_ctx);

	/* Check H/W Activation & Get Version */
	ret = str_power_clk_enable(core);
	if (ret < 0) {
		dev_err(core->dev, "failed to str_power_clk_enable. ret(%d)\n", ret);
		goto err_clk;
	}

	str_hw_update_version(&core->hw);
	dev_info(core->dev, "reg_ver(%x), str_ver(%x)\n", core->hw.reg_ver, core->hw.str_ver);

	ret = str_power_clk_disable(core);
	if (ret < 0) {
		dev_err(core->dev, "failed to pm_runtime_put_sync. ret(%d)\n", ret);
		goto err_clk;
	}

	/* Register MMU fault handler */
	iovmm_set_fault_handler(core->dev, _str_sysmmu_falut_handler, core);

	dev_info(core->dev, "STR driver probe done. version(%08x)\n", core->hw.str_ver);

	return 0;
err_clk:
	iovmm_deactivate(core->dev);
err_exit:
	return ret;
}

static int str_remove(struct platform_device *pdev)
{
	struct str_core *core = platform_get_drvdata(pdev);

	_str_ion_deinit(&core->ion_ctx);

	iovmm_deactivate(core->dev);

	_str_clk_put(core);

	return 0;
}

static void str_shutdown(struct platform_device *pdev)
{
	struct str_core *core = platform_get_drvdata(pdev);

	_str_ion_deinit(&core->ion_ctx);

	iovmm_deactivate(core->dev);
}

static int str_suspend(struct device *dev)
{
	struct str_core *core = dev_get_drvdata(dev);

	dev_info(core->dev, "[STR][%s]\n", __func__);

	return 0;
}

static int str_resume(struct device *dev)
{
	struct str_core *core = dev_get_drvdata(dev);

	dev_info(core->dev, "[STR][%s]\n", __func__);

	return 0;
}

static int str_runtime_suspend(struct device *dev)
{
	struct str_core *core = dev_get_drvdata(dev);

	dev_info(core->dev, "[STR][%s]\n", __func__);

	return 0;
}

static int str_runtime_resume(struct device *dev)
{
	struct str_core *core = dev_get_drvdata(dev);

	dev_info(core->dev, "[STR][%s]\n", __func__);

	return 0;
}

static const struct dev_pm_ops str_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(str_suspend, str_resume)
	SET_RUNTIME_PM_OPS(str_runtime_suspend, str_runtime_resume, NULL)
};

static const struct of_device_id exynos_str_match[] = {
	{
		.compatible = "samsung,exynos5-camerapp-str",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_str_match);

static struct platform_driver str_driver = {
	.probe		= str_probe,
	.remove		= str_remove,
	.shutdown	= str_shutdown,
	.driver		= {
		.name		= MODULE_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= of_match_ptr(exynos_str_match),
		.pm		= &str_pm_ops,
	}
};

module_platform_driver(str_driver);

MODULE_AUTHOR("Samsung LSI CameraPP");
MODULE_DESCRIPTION("EXYNOS CameraPP STR driver");
MODULE_LICENSE("GPL");
