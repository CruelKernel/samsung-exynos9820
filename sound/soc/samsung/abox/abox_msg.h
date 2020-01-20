/* sound/soc/samsung/abox/abox_msg.h
 *
 * ALSA SoC Audio Layer - Samsung Abox Message Queue driver
 *
 * Copyright (c) 2017 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __SND_SOC_ABOX_MSG_H
#define __SND_SOC_ABOX_MSG_H

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/string.h>
#include <sound/samsung/abox_ipc.h>
#else
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "RTE_Components.h"
#include CMSIS_device_header

#include "cmsis_os2.h"
#include "rtx_lib.h"

#include "ipc_manager.h"

#ifndef __packed
#define	__packed	__attribute__((__packed__))
#endif

#define wmb()		__DMB()
#endif

#define ABOX_MSG_LEN_CMD	16

struct abox_msg_cfg {
	void *tx_addr;
	void *rx_addr;
	int32_t tx_size;
	int32_t rx_size;
	void (*tx_lock_f)(void);
	void (*tx_unlock_f)(void);
	void (*rx_lock_f)(void);
	void (*rx_unlock_f)(void);
	uint64_t (*get_time_f)(void);
	void (*print_log_f)(const char *fmt, ...);
	int32_t ret_ok;
	int32_t ret_err;
};

struct abox_msg_data {
	int32_t size;
	uint8_t data[];
} __packed;

/* debugging purpose only */
struct abox_msg_ipc {
	int32_t size;
	ABOX_IPC_MSG msg;
} __packed;

union abox_msg_addr {
	struct abox_msg_data *data;
	uint64_t addr64;
	uint32_t addr32;
	struct abox_msg_ipc *ipc;
} __packed;

struct abox_msg_cmd {
	int32_t id;
	int32_t cmd;
	int32_t arg[3];
	uint32_t data_idx;
	union abox_msg_addr send;
	union abox_msg_addr recv;
	uint64_t time_put;
	uint64_t time_get;
} __packed;

struct abox_msg_cmd_queue {
	int32_t idx_s;
	int32_t idx_e;
	struct abox_msg_cmd elem[ABOX_MSG_LEN_CMD];
} __packed;

struct abox_msg_data_queue {
	int32_t idx_s;
	int32_t idx_e;
	uint32_t len;
	int8_t elem[];
} __packed;

struct abox_msg_queue {
	struct abox_msg_cmd_queue q_cmd;
	struct abox_msg_data_queue q_data;
} __packed;

struct abox_msg_send_data {
	int size;
	const void *data;
};

/**
 * Send message
 * @param[in]	cmd	command
 * @param[in]	data	sending data
 * @param[in]	size	size of data
 * @return	0 or error code
 */
extern int32_t abox_msg_send(struct abox_msg_cmd *cmd,
		const struct abox_msg_send_data *data, int count);

/**
 * Flush sent messages
 * @return	0 or error code
 */
extern int32_t abox_msg_flush(void);

/**
 * Receive message
 * @param[in]	cmd	command
 * @param[in]	data	data buffer
 * @param[in]	size	size of data buffer
 * @return	size of data or error code
 */
extern int32_t abox_msg_recv(struct abox_msg_cmd *cmd,
		void *data, int32_t size);

/**
 * Initialize Message Queue
 * @param[in]	cfg	configuration
 * @return	0 or error code
 */
extern int abox_msg_init(const struct abox_msg_cfg *cfg);

#endif /* __SND_SOC_ABOX_MSG_H */
