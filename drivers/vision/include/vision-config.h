/*
 * Samsung Exynos SoC series VPU driver
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef VISION_CONFIG_H_
#define VISION_CONFIG_H_

#define VISION_MAX_BUFFER		16
#define VISION_MAX_PLANE		3
#define VISION_MAP_KVADDR

/*#define probe_info(fmt, ...)		pr_info("[V]" fmt, ##__VA_ARGS__)
#define probe_warn(fmt, args...)	pr_warning("[V][WRN]" fmt, ##args)
#define probe_err(fmt, args...)	pr_err("[V][ERR]%s:%d:" fmt, __func__, __LINE__, ##args)*/

#ifdef DEBUG_LOG_MEMORY
#define vision_err_target(fmt, ...)	printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#define vision_warn_target(fmt, ...)	printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#define vision_info_target(fmt, ...)	printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#define vision_dbg_target(fmt, ...)	printk(KERN_DEBUG fmt, ##__VA_ARGS__)
#else
#define vision_err_target(fmt, ...)	pr_err(fmt, ##__VA_ARGS__)
#define vision_warn_target(fmt, ...)	pr_warn(fmt, ##__VA_ARGS__)
#define vision_info_target(fmt, ...)	pr_info(fmt, ##__VA_ARGS__)
#define vision_dbg_target(fmt, ...)	pr_info(fmt, ##__VA_ARGS__)
#endif

#define vision_err(fmt, args...) \
	vision_err_target("[V][ERR]%s:%d:" fmt, __func__, __LINE__, ##args)

#define vision_info(fmt, args...) \
	vision_info_target("[V]" fmt, ##args)

#endif
