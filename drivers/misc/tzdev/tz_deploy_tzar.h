/*
 * Copyright (C) 2012-2018 Samsung Electronics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __TZ_DEPLOY_TZAR_H__
#define __TZ_DEPLOY_TZAR_H__


#ifdef CONFIG_TZDEV_DEPLOY_TZAR
int tzdev_deploy_tzar(void);
#else
int tzdev_deploy_tzar(void)
{
	return 0;
}
#endif

#endif /*__TZ_DEPLOY_TZAR_H__ */
