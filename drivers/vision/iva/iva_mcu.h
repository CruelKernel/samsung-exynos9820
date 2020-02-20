/*
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#ifndef _IVA_MCU_H_
#define _IVA_MCU_H_

#include "iva_ctrl.h"

enum mcu_err_context {
	muc_err_from_normal,
	mcu_err_from_irq,
	mcu_err_from_abort,

	mcu_err_context_max
};

enum mcu_boot_hold {
	mcu_boothold		= (0x1 << 0),
};

enum edil_ctrl {
	edil_read_cache		= (0x1 << 0),
	edil_early_response	= (0x1 << 1),
};

enum init_tcm {
	itcm_en			= (0x1 << 0),
	dtcm_en			= (0x1 << 1),
};

#if defined(CONFIG_SOC_EXYNOS8895)
#define IVA_MCU_FILE_PATH "/system/vendor/firmware/iva10_rt-kangchen.bin"
#elif defined(CONFIG_SOC_EXYNOS9810)
#define IVA_MCU_FILE_PATH "/system/vendor/firmware/iva20_rt-lhotse.bin"
#elif defined(CONFIG_SOC_EXYNOS9820)
#define IVA_MCU_FILE_PATH "/system/vendor/firmware/iva30_rt-makalu.bin"
#endif

extern void	iva_mcu_print_flush_pending_mcu_log(struct iva_dev_data *iva);

extern void	iva_mcu_handle_error_k(struct iva_dev_data *iva,
				enum mcu_err_context from, uint32_t msg);

extern void	iva_mcu_show_status(struct iva_dev_data *iva);

extern int32_t	iva_mcu_boot_file(struct iva_dev_data *iva, const char *mcu_file,
			int32_t wait_ready);
extern int32_t	iva_mcu_exit(struct iva_dev_data *iva);

extern int32_t	iva_mcu_probe(struct iva_dev_data *iva);

extern void	iva_mcu_remove(struct iva_dev_data *iva);

extern void	iva_mcu_shutdown(struct iva_dev_data *iva);

#if defined(CONFIG_SOC_EXYNOS9820)
extern uint32_t iva_mcu_ctrl(struct iva_dev_data *iva,
			enum mcu_boot_hold ctrl, bool set);

extern uint32_t iva_mcu_init_tcm(struct iva_dev_data *iva,
			enum init_tcm ctrl, bool set);

extern void iva_mcu_prepare_mcu_reset(struct iva_dev_data *iva,
                        uint32_t init_vtor);

extern void iva_mcu_reset_mcu(struct iva_dev_data *iva);
#endif

#endif /* _IVA_MCU_H_ */
