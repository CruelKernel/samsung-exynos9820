/*
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __IVA_MEM_SYNC_H__
#define __IVA_MEM_SYNC_H__

#include "iva_mem.h"

extern void iva_ion_sync_sg_for_cpu(struct iva_dev_data *iva,
		struct iva_mem_map *iva_map_node);
extern void iva_ion_sync_sg_for_device(struct iva_dev_data *iva,
		struct iva_mem_map *iva_map_node, bool clean_only);
#endif /*__IVA_MEM_SYNC_H_*/
