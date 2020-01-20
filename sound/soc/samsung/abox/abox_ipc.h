/* sound/soc/samsung/abox/abox_ipc.h
 *
 * ALSA SoC Audio Layer - Samsung Abox Inter-Processor Communication driver
 *
 * Copyright (c) 2017 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_ABOX_IPC_H
#define __SND_SOC_ABOX_IPC_H

/**
 * Send IPC
 * @param[in]	dev	device which invokes this API
 * @param[in]	ipc	ipc
 * @param[in]	size	size of ipc
 * @return	error code or 0
 */
extern int abox_ipc_send(struct device *dev, const ABOX_IPC_MSG *ipc,
		size_t size, const void *bundle, size_t bundle_size);

/**
 * Retry last IPC
 */
extern void abox_ipc_retry(void);

/**
 * Flush sent IPCs
 * @param[in]	dev	device which invokes this API
 * @return	error code or 0
 */
extern int abox_ipc_flush(struct device *dev);

/**
 * Register ipc handler
 * @param[in]	dev	device which invokes this API
 * @param[in]	ipc_id	ipc id
 * @param[in]	handler	ipc handler
 * @param[in]	data	private data
 * @return	error code or 0
 */
extern int abox_ipc_register_handler(struct device *dev, int ipc_id,
		abox_irq_handler_t handler, void *data);

/**
 * Unregister ipc handler
 * @param[in]	dev	device which invokes this API
 * @param[in]	ipc_id	ipc id
 * @param[in]	handler	ipc handler
 * @return	error code or 0
 */
extern int abox_ipc_unregister_handler(struct device *dev, int ipc_id,
		abox_irq_handler_t handler);

/**
 * Initialize IPC module
 * @param[in]	dev	abox device
 * @param[in]	tx	memory address for ipc to abox
 * @param[in]	tx_size	size of memory for ipc to abox
 * @param[in]	rx	memory address for ipc from abox
 * @param[in]	rx_size	size of memory for ipc from abox
 * @return	error code or 0
 */
extern int abox_ipc_init(struct device *dev, void *tx, size_t tx_size,
		void *rx, size_t rx_size);

#endif /* __SND_SOC_ABOX_IPC_H */
