/*
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __IVA_VDMA10_H__
#define __IVA_VDMA10_H__

#include "iva_ctrl.h"

/* vdma instance id */
enum vdma_id {
	vdma_id_0	= 0,	/* vdma instance 0 */
	vdma_id_1,		/* vdma instance 1 */
	vdma_id_max		/* vdma instance id max */
};


#define NUM_VDMA_CHANNELS	(4)
enum vdma_ch {
	vdma_ch_0	= 0,
	vdma_ch_1,
	vdma_ch_2,
	vdma_ch_3,
	vdma_ch_max,

	/* just for easier code readibility */
	vdma_ch_invalid = 8,
};

#define CH_MASK		(vdma_ch_max - 1)
#define ID_SHIFT	(2)

#define INVALID_HALDLE_ID	((uint32_t)-1)

#define HALDLE_TO_CH(hdl)	((enum vdma_ch)(hdl & CH_MASK))
#define HALDLE_TO_ID(hdl)	((enum vdma_id)(hdl >> ID_SHIFT))
#define ID_CH_TO_HANDLE(id, ch)	((id << ID_SHIFT) | ch)

extern void iva_vdma_start_cmd(struct iva_dev_data *iva, uint32_t handle);
/* busy wait*/
extern void iva_vdma_wait_done(struct iva_dev_data *iva, uint32_t handle);

extern int32_t	iva_vdma_config_dram_to_mcu(struct iva_dev_data *iva,
			uint32_t handle, uint32_t io_va,
			uint32_t mcu_addr, uint32_t x_size, bool start);

extern int32_t	iva_vdma_enable(struct iva_dev_data *iva,
			enum vdma_id id, bool enable);

extern void	iva_vdma_show_status(struct iva_dev_data *iva);

#endif /* __IVA_VDMA_H__ */
