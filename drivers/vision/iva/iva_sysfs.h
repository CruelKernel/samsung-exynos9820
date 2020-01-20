/*
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#ifndef _IVA_SYSFS_H_
#define _IVA_SYSFS_H_

#include "iva_ctrl.h"

extern int32_t iva_sysfs_init(struct device *dev);
extern void iva_sysfs_deinit(struct device *dev);

#endif /* _IVA_CTRL_H_ */
