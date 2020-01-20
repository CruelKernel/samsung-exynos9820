/*
 * Copyright (C) 2017 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __IVA_VDMA30_H__
#define __IVA_VDMA30_H__

#include "iva_ctrl.h"

/* vdma instance id */
enum vdma_id {
	vdma_id_0	= 0,	/* vdma instance 0 */
	vdma_id_max		/* vdma instance id max */
};

/* vdma conversion mode */
enum vdma_conversion_mode {
	vdma_conversion_disable	= 0,
	vdma_conversion_enable	= 1,
};

#define VDMA_TCM_BASE_ADDRESS       (0x40100000)
#define NUM_VDMA_CHANNELS	        (16)

enum vdma_ch {
        vdma_ch0_type_ro        = 0,
        vdma_ch1_type_ro,
        vdma_ch2_type_ro,
        vdma_ch3_type_ro,
        vdma_ch4_type_ro,
        vdma_ch5_type_ro_spl,
        vdma_ch6_type_ro_yuv,
        vdma_ch7_type_ro_yuv,
        vdma_ch8_type_wo,
        vdma_ch9_type_wo,
        vdma_ch10_type_wo,
        vdma_ch11_type_wo,
        vdma_ch12_type_wo,
        vdma_ch13_type_wo,
        vdma_ch14_type_wo_yuv,
        vdma_ch15_type_wo_yuv,
	vdma_ch_max	= NUM_VDMA_CHANNELS,

	/* just for easier code readibility */
	vdma_ch_invalid = 16,
};

#define INVALID_HALDLE_ID	((uint32_t)-1)

#define CH_MASK			(vdma_ch_max - 1)
#define HALDLE_TO_CH(hdl)	((enum vdma_ch)(hdl & CH_MASK))
#define CH_TO_HANDLE(ch)	(ch)

extern void iva_vdma_start_cmd(struct iva_dev_data *iva, uint32_t handle);
/* busy wait*/
extern void iva_vdma_wait_done(struct iva_dev_data *iva, uint32_t handle);

extern int32_t	iva_vdma_config_dram_to_mcu(struct iva_dev_data *iva,
			uint32_t handle, uint32_t io_va,
			uint32_t mcu_addr, uint32_t x_size, bool start);

extern int32_t	iva_vdma_enable(struct iva_dev_data *iva, bool enable);

extern void	iva_vdma_show_status(struct iva_dev_data *iva);

extern void     iva_vdma_path_dram_to_mcu(struct iva_dev_data *iva,
                        uint32_t handle, bool enable);


#endif /* __IVA_VDMA30_H__ */
