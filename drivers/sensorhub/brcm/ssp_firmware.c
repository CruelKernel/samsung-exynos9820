/*
 *  Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */
#include "ssp.h"

#if defined(CONFIG_SENSORS_SSP_BEYOND)

#if ANDROID_VERSION < 100000	//p_os
#define SSP_FIRMWARE_REVISION_BCM	19071200
#else				//q_os
#define SSP_FIRMWARE_REVISION_BCM	19072400
#endif

#elif defined(CONFIG_SENSORS_SSP_DAVINCI)

#if ANDROID_VERSION < 100000	//p_os
#define SSP_FIRMWARE_REVISION_BCM	19080500
#else				//q_os
#define SSP_FIRMWARE_REVISION_BCM	19072400
#endif

#else
#define SSP_FIRMWARE_REVISION_BCM	00000000
#endif
#define SSP_FIRMWARE_REVISION_NEW_OLD_BCM	19030700

unsigned int get_module_rev(struct ssp_data *data)
{
        int patch_version = get_patch_version(data->ap_type, data->ap_rev);

        if(patch_version == bbd_new_old)
        	return SSP_FIRMWARE_REVISION_NEW_OLD_BCM;
    
	return SSP_FIRMWARE_REVISION_BCM;
}
