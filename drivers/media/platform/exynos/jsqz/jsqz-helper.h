/* drivers/media/platform/exynos/jsqz/jsqz-helper.h
 *
 * Internal header for Samsung JPEG Squeezer driver
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Jungik Seo <jungi.seo@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef JSQZ_HELPER_H_
#define JSQZ_HELPER_H_

#include <linux/dma-buf.h>
#include <linux/ion_exynos.h>
#include "jsqz-core.h"

/**
 * jsqz_set_dma_address - set DMA address
 */
static inline void jsqz_set_dma_address(
		struct jsqz_buffer_dma *buffer_dma,
		dma_addr_t dma_addr)
{
	buffer_dma->plane.dma_addr = dma_addr;
}

int jsqz_map_dma_attachment(struct device *dev,
		     struct jsqz_buffer_plane_dma *plane,
		     enum dma_data_direction dir);

void jsqz_unmap_dma_attachment(struct device *dev,
			struct jsqz_buffer_plane_dma *plane,
			enum dma_data_direction dir);

int jsqz_dma_addr_map(struct device *dev,
		      struct jsqz_buffer_dma *buf,
		      enum dma_data_direction dir);

void jsqz_dma_addr_unmap(struct device *dev,
			 struct jsqz_buffer_dma *buf);
/*
void jsqz_sync_for_device(struct device *dev,
			  struct jsqz_buffer_plane_dma *plane,
			  enum dma_data_direction dir);

void jsqz_sync_for_cpu(struct device *dev,
		       struct jsqz_buffer_plane_dma *plane,
		       enum dma_data_direction dir);
*/
#endif /* JSQZ_HELPER_H_ */
