/*
 * Samsung Exynos5 SoC series FIMC-IS-FROM driver
 *
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_FROM_H
#define FIMC_IS_FROM_H

int fimc_is_spi_write(struct fimc_is_spi *spi, u32 addr, u8 *data, size_t size);
int fimc_is_spi_write_enable(struct fimc_is_spi *spi);
int fimc_is_spi_write_disable(struct fimc_is_spi *spi);
int fimc_is_spi_erase_sector(struct fimc_is_spi *spi, u32 addr);
int fimc_is_spi_erase_block(struct fimc_is_spi *spi, u32 addr);
int fimc_is_spi_read_status_bit(struct fimc_is_spi *spi, u8 *buf);
int fimc_is_spi_read_module_id(struct fimc_is_spi *spi, void *buf, u16 addr, size_t size);
#endif
