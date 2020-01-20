/*
 * Samsung EXYNOS CAMERA PostProcessing GDC driver
 *
 * Copyright (C) 2016 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "camerapp-gdc.h"
#include "camerapp-hw-api-gdc.h"
#include <exynos-fimc-is-sensor.h>

void camerapp_gdc_grid_setting(struct gdc_dev *gdc)
{
	int i, j;
	struct gdc_ctx *ctx = gdc->current_ctx;

	if (ctx->crop_param.use_calculated_grid) {
		for (i = 0; i < GRID_Y_SIZE; i++) {
			for (j = 0; j < GRID_X_SIZE; j++) {
				ctx->grid_param.dx[i][j] = ctx->crop_param.calculated_grid_x[i][j];
				ctx->grid_param.dy[i][j] = ctx->crop_param.calculated_grid_y[i][j];
			}
		}
		gdc_dbg("use calculated grid table\n");
	} else {
		for (i = 0; i < GRID_Y_SIZE; i++) {
			for (j = 0; j < GRID_X_SIZE; j++) {
				ctx->grid_param.dx[i][j] = 0;
				ctx->grid_param.dy[i][j] = 0;
			}
		}
	}

	ctx->grid_param.is_valid = true;

	return;
}

