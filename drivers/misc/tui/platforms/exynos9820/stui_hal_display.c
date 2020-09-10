/*
 * Samsung TUI HW Handler driver. Display functions.
 *
 * Copyright (c) 2015-2018 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "decon.h"
#include "stui_core.h"
#include "stui_hal.h"

#include <linux/dma-buf.h>
#include <linux/fb.h>
#include <linux/ion_exynos.h>
#include <linux/version.h>

static struct dma_buf *g_dma_buf;
static struct dma_buf_attachment *g_attachment;
static struct sg_table *g_sgt;

static struct fb_info *get_fb_info_for_tui(struct device *fb_dev);

/* Framebuffer device driver identification
 * RETURN: 0 - Wrong device driver
 *         1 - Suitable device driver
 */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 9, 0)
static int _is_dev_ok(struct device *fb_dev, void *p)
#else
static int _is_dev_ok(struct device *fb_dev, const void *p)
#endif
{
	struct fb_info *fb_info;

	fb_info = get_fb_info_for_tui(fb_dev);
	if (!fb_info)
		return 0;

	return 1;
}

/* Find suitable framebuffer device driver */
static struct device *get_fb_dev_for_tui(void)
{
	struct device *fb_dev;
	/* get the first framebuffer device */
	fb_dev = class_find_device(fb_class, NULL, NULL, _is_dev_ok);
	if (!fb_dev)
		pr_err("[STUI] class_find_device failed\n");

	return fb_dev;
}

/* Get framebuffer's internal data */
static struct fb_info *get_fb_info_for_tui(struct device *fb_dev)
{
	struct fb_info *fb_item;

	if (!fb_dev || !fb_dev->p) {
		pr_err("[STUI] framebuffer device has no private data\n");
		return NULL;
	}
	fb_item = (struct fb_info *)dev_get_drvdata(fb_dev);
	if (!fb_item)
		pr_err("[STUI] dev_get_drvdata failed\n");

	return fb_item;
}

static int fb_protection_for_tui(bool tui_en)
{
	struct device *fb_dev;
	struct fb_info *fb_info;
	int ret;

	pr_debug("[STUI] %s : state %d start\n", __func__, tui_en);

	fb_dev = get_fb_dev_for_tui();
	if (!fb_dev)
		return -1;

	fb_info = get_fb_info_for_tui(fb_dev);
	if (!fb_info)
		return -1;

	ret = decon_tui_protection(tui_en);

	pr_debug("[STUI] %s : state %d end\n", __func__, tui_en);

	return ret;
}

void stui_free_video_space(void)
{
	if (g_attachment && g_sgt) {
		dma_buf_unmap_attachment(g_attachment, g_sgt, DMA_BIDIRECTIONAL);
		g_sgt = NULL;
	}
	if (g_dma_buf && g_attachment) {
		dma_buf_detach(g_dma_buf, g_attachment);
		g_attachment = NULL;
	}
	if (g_dma_buf) {
		dma_buf_put(g_dma_buf);
		g_dma_buf = NULL;
	}
}

int stui_alloc_video_space(struct device *dev, struct tui_hw_buffer *buffer)
{
	dma_addr_t phys_addr = 0;
	size_t framebuf_size;
	size_t workbuf_size;
	struct decon_lcd *lcd_info = decon_drvdata[0]->lcd_info;

	framebuf_size = (lcd_info->xres * lcd_info->yres * (DEFAULT_BPP >> 3));
	framebuf_size = STUI_ALIGN_UP(framebuf_size, STUI_ALIGN_4kB_SZ);
	workbuf_size = (lcd_info->xres * lcd_info->yres * (2 * (DEFAULT_BPP >> 3) + 1));
	workbuf_size = STUI_ALIGN_UP(workbuf_size, STUI_ALIGN_4kB_SZ);

	g_dma_buf = ion_alloc_dmabuf("vstream_heap",
					framebuf_size + workbuf_size + STUI_ALIGN_4kB_SZ,
					ION_FLAG_PROTECTED);
	if (IS_ERR(g_dma_buf)) {
		pr_err("[STUI] fail to allocate dma buffer\n");
		goto err_alloc;
	}

	g_attachment = dma_buf_attach(g_dma_buf, dev);
	if (IS_ERR_OR_NULL(g_attachment)) {
		pr_err("[STUI] fail to dma buf attachment\n");
		goto err_attach;
	}

	g_sgt = dma_buf_map_attachment(g_attachment, DMA_BIDIRECTIONAL);
	if (IS_ERR_OR_NULL(g_sgt)) {
		pr_err("[STUI] fail to map attachment\n");
		goto err_attachment;
	}
	phys_addr = sg_phys(g_sgt->sgl);
	phys_addr = STUI_ALIGN_UP(phys_addr, STUI_ALIGN_4kB_SZ);

	buffer->width = lcd_info->xres;
	buffer->height = lcd_info->yres;
	buffer->fb_physical = (uint64_t)phys_addr;
	buffer->wb_physical = (uint64_t)((workbuf_size) ? (phys_addr + framebuf_size) : 0);
	buffer->fb_size = framebuf_size;
	buffer->wb_size = workbuf_size;

	return 0;

err_attachment:
	dma_buf_detach(g_dma_buf, g_attachment);
err_attach:
	dma_buf_put(g_dma_buf);
err_alloc:
	return -1;
}

int stui_get_resolution(struct tui_hw_buffer *buffer)
{
	struct decon_lcd *lcd_info = decon_drvdata[0]->lcd_info;

	buffer->width = lcd_info->xres;
	buffer->height = lcd_info->yres;

	return 0;
}

int stui_prepare_tui(void)
{
	pr_debug("[STUI] %s - start\n", __func__);
	return fb_protection_for_tui(true);
}

void stui_finish_tui(void)
{
	pr_debug("[STUI] %s - start\n", __func__);
	if (fb_protection_for_tui(false))
		pr_err("[STUI] failed to unprotect tui\n");
}
