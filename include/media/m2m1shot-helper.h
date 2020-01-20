/*
 * include/media/m2m1shot-helper.h
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *
 * Contact: Cho KyongHo <pullip.cho@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef _M2M1SHOT_HELPER_H_
#define _M2M1SHOT_HELPER_H_
#include <linux/kernel.h>
#include <linux/dma-buf.h>

#include <media/m2m1shot.h>

int m2m1shot_map_dma_buf(struct device *dev,
			struct m2m1shot_buffer_plane_dma *plane,
			enum dma_data_direction dir);

void m2m1shot_unmap_dma_buf(struct device *dev,
			struct m2m1shot_buffer_plane_dma *plane,
			enum dma_data_direction dir);

int m2m1shot_dma_addr_map(struct device *dev,
			struct m2m1shot_buffer_dma *buf,
			int plane_idx, enum dma_data_direction dir);

void m2m1shot_dma_addr_unmap(struct device *dev,
			struct m2m1shot_buffer_dma *buf, int plane_idx);

void m2m1shot_sync_for_device(struct device *dev,
			      struct m2m1shot_buffer_plane_dma *plane,
			      enum dma_data_direction dir);

void m2m1shot_sync_for_cpu(struct device *dev,
			   struct m2m1shot_buffer_plane_dma *plane,
			   enum dma_data_direction dir);

static inline dma_addr_t m2m1shot_dma_address(
			struct m2m1shot_buffer_plane_dma *plane)
{
	return plane->dma_addr;
}

#endif /* _M2M1SHOT_HELPER_H_ */
