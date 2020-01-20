/*
 * Copyright (C) 2017 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __IVA_RT_TABLE_H__
#define __IVA_RT_TABLE_H__

#include "iva_ctrl.h"

extern void	iva_rt_print_iva_entries(struct iva_dev_data *iva,
			void *iva_tbl_ptr, int8_t *num_deps_list);

extern void	iva_ram_dump_print_iva_bin_log(struct iva_dev_data *iva,
			void *bin_dbg_ptr);

#endif /* __IVA_RT_TABLE_H__ */
