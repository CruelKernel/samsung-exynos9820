/* sound/soc/samsung/abox/abox_qos.h
 *
 * ALSA SoC - Samsung Abox QoS driver
 *
 * Copyright (c) 2018 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_ABOX_QOS_H
#define __SND_SOC_ABOX_QOS_H

#include <linux/pm_qos.h>

enum abox_qos_class {
	ABOX_QOS_AUD = PM_QOS_AUD_THROUGHPUT,
	ABOX_QOS_CL0 = PM_QOS_CLUSTER0_FREQ_MIN,
	ABOX_QOS_CL1 = PM_QOS_CLUSTER1_FREQ_MIN,
	ABOX_QOS_CL2 = PM_QOS_CLUSTER2_FREQ_MIN,
	ABOX_QOS_INT = PM_QOS_DEVICE_THROUGHPUT,
	ABOX_QOS_MIF = PM_QOS_BUS_THROUGHPUT,
};

struct abox_qos_req {
	unsigned int id;
	unsigned int val;
	char name[32];
	unsigned long long updated;
};

struct abox_qos {
	struct pm_qos_request pm_qos;
	enum abox_qos_class qos_class;
	unsigned int val;
	const char *name;
	struct abox_qos_req req_array[64];
};

/**
 * Get target value of the QoS
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	qos_class	refer to the enum abox_qos_class
 * @return	target value
 */
extern int abox_qos_get_target(struct device *dev,
		enum abox_qos_class qos_class);

/**
 * Get value of the specific request
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	qos_class	refer to the enum abox_qos_class
 * @param[in]	id		key which is used as unique handle
 * @return	value of the request
 */
unsigned int abox_qos_get_value(struct device *dev,
		enum abox_qos_class qos_class, unsigned int id);

/**
 * Check whether the specific request is active or not
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	qos_class	refer to the enum abox_qos_class
 * @param[in]	id		key which is used as unique handle
 * @return	1 if the request is active or 0
 */
extern int abox_qos_is_active(struct device *dev,
		enum abox_qos_class qos_class, unsigned int id);

/**
 * Complete scheduled QoS requests
 */
extern void abox_qos_complete(void);

/**
 * Request minimum lock
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	qos_class	refer to the enum abox_qos_class
 * @param[in]	id		key which is used as unique handle
 * @param[in]	val		QoS value
 * @param[in]	name		cookie for logging
 * @return	error code or 0
 */
extern int abox_qos_request(struct device *dev, enum abox_qos_class qos_class,
		unsigned int id, unsigned int val, const char *name);

/**
 * Request minimum lock on ABOX CPU clock (PM QoS AUD)
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	id		key which is used as unique handle
 * @param[in]	val		QoS value
 * @param[in]	name		cookie for logging
 * @return	error code or 0
 */
static inline int abox_qos_request_aud(struct device *dev, unsigned int id,
		unsigned int val, const char *name)
{
	dev_info(dev, "%s(%#x, %d)\n", __func__, id, val);
	return abox_qos_request(dev, ABOX_QOS_AUD, id, val, name);
}

/**
 * Request minimum lock on PM QoS CLUSTER0
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	id		key which is used as unique handle
 * @param[in]	val		QoS value
 * @param[in]	name		cookie for logging
 * @return	error code or 0
 */
static inline int abox_qos_request_cl0(struct device *dev, unsigned int id,
		unsigned int val, const char *name)
{
	dev_dbg(dev, "%s(%#x, %d)\n", __func__, id, val);
	return abox_qos_request(dev, ABOX_QOS_CL0, id, val, name);
}

/**
 * Request minimum lock on PM QoS CLUSTER1
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	id		key which is used as unique handle
 * @param[in]	val		QoS value
 * @param[in]	name		cookie for logging
 * @return	error code or 0
 */
static inline int abox_qos_request_cl1(struct device *dev, unsigned int id,
		unsigned int val, const char *name)
{
	dev_dbg(dev, "%s(%#x, %d)\n", __func__, id, val);
	return abox_qos_request(dev, ABOX_QOS_CL1, id, val, name);
}

/**
 * Request minimum lock on PM QoS CLUSTER2
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	id		key which is used as unique handle
 * @param[in]	val		QoS value
 * @param[in]	name		cookie for logging
 * @return	error code or 0
 */
static inline int abox_qos_request_cl2(struct device *dev, unsigned int id,
		unsigned int val, const char *name)
{
	dev_dbg(dev, "%s(%#x, %d)\n", __func__, id, val);
	return abox_qos_request(dev, ABOX_QOS_CL2, id, val, name);
}

/**
 * Request minimum lock on PM QoS INT
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	id		key which is used as unique handle
 * @param[in]	val		QoS value
 * @param[in]	name		cookie for logging
 * @return	error code or 0
 */
static inline int abox_qos_request_int(struct device *dev, unsigned int id,
		unsigned int val, const char *name)
{
	dev_info(dev, "%s(%#x, %d)\n", __func__, id, val);
	return abox_qos_request(dev, ABOX_QOS_INT, id, val, name);
}

/**
 * Request minimum lock on PM QoS MIF
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	id		key which is used as unique handle
 * @param[in]	val		QoS value
 * @param[in]	name		cookie for logging
 * @return	error code or 0
 */
static inline int abox_qos_request_mif(struct device *dev, unsigned int id,
		unsigned int val, const char *name)
{
	dev_info(dev, "%s(%#x, %d)\n", __func__, id, val);
	return abox_qos_request(dev, ABOX_QOS_MIF, id, val, name);
}

/**
 * Clear all request for the QoS class
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	qos_class	refer to the enum abox_qos_class
 */
extern void abox_qos_clear(struct device *dev, enum abox_qos_class qos_class);

/**
 * Print all active request for the QoS class
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	qos_class	refer to the enum abox_qos_class
 */
extern void abox_qos_print(struct device *dev, enum abox_qos_class qos_class);

/**
 * Initialize abox_qos module
 * @param[in]	adev		abox device
 */
extern void abox_qos_init(struct device *adev);

#ifdef CONFIG_SND_SOC_SAMSUNG_AUDIO
extern ssize_t abox_qos_read_file(struct file *file, char __user *user_buf,
				    size_t count, loff_t *ppos);
#endif

#endif /* __SND_SOC_ABOX_QOS_H */
