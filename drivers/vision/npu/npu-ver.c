/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/sysfs.h>

#include "npu-device.h"
#include "npu-debug.h"
#include "npu-log.h"
#include "npu-interface.h"
#include "npu-ver-info.h"
#include "npu-ver.h"

#ifdef CONFIG_EXYNOS_NPU_PUBLISH_NPU_BUILD_VER
static ssize_t version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	/*
	 * sysfs read buffer size is PAGE_SIZE
	 * Ref: http://www.kernel.org/pub/linux/kernel/people/mochel/doc/papers/ols-2005/mochel.pdf
	 */
	ssize_t remain = PAGE_SIZE - 1;
	ssize_t ret = 0;

	npu_info("start.");

	/* Populate the buffer */
	ret = scnprintf(buf, remain,
		"Build info: %s\n"
		"NCP version: %u\n"
		"Mailbox verison: %d\n"
		"CMD version: %d\n"
		"User API version: %d\n"
		"Signature:\n%s\n",
		npu_build_info,
		NCP_VERSION, MAILBOX_VERSION, COMMAND_VERSION,
		USER_API_VERSION,
		npu_git_hash_str);

	return ret;
}

/*
 * sysfs attribute for version info
 *   name = dev_attr_version,
 *   show = version_show
 */
static DEVICE_ATTR_RO(version);

/* Exported functions */
int npu_ver_probe(struct npu_device *npu_device)
{
	int ret = 0;
	struct device *dev;

	BUG_ON(!npu_device);
	dev = npu_device->dev;
	BUG_ON(!dev);

	ret = sysfs_create_file(&dev->kobj, &dev_attr_version.attr);
	if (ret) {
		npu_err("sysfs_create_file error : ret = %d\n", ret);
		return ret;
	}

	return ret;
}
/* Exported functions */
int npu_ver_release(struct npu_device *npu_device)
{
	struct device *dev;

	BUG_ON(!npu_device);
	dev = npu_device->dev;
	BUG_ON(!dev);

	sysfs_remove_file(&dev->kobj, &dev_attr_version.attr);

	return 0;
}
#else	/* Not CONFIG_EXYNOS_NPU_PUBLISH_NPU_BUILD_VER */

int npu_ver_probe(struct npu_device *npu_device)
{
	/* No OP */
	return 0;
}

int npu_ver_release(struct npu_device *npu_device)
{
	/* No OP */
	return 0;
}
#endif
