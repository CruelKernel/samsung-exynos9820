/* sound/soc/samsung/abox/abox_msg.c
 *
 * ALSA SoC Audio Layer - Samsung Abox Message Queue driver
 *
 * Copyright (c) 2017 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "abox_msg.h"

#define TIMEOUT_NS 1000000
#define FLUSH_TIMEOUT_NS (TIMEOUT_NS * 20)
#define min(a, b) (a < b ? a : b)

static struct abox_msg_queue *tx;
static struct abox_msg_queue *rx;

static void (*tx_lock)(void);
static void (*tx_unlock)(void);
static void (*rx_lock)(void);
static void (*rx_unlock)(void);
static uint64_t (*get_time)(void);
static void (*print_log)(const char *fmt, ...);

static int32_t ret_ok;
static int32_t ret_err;

static int is_avail(struct abox_msg_data_queue *q_data, int32_t size)
{
	volatile int32_t *idx_s = &q_data->idx_s;
	int blank;

	if (*idx_s > q_data->idx_e) {
		blank = q_data->idx_s - q_data->idx_e;
	} else {
		blank = q_data->len - q_data->idx_e;
		if (blank < size) {
			q_data->idx_e = 0;
			blank = q_data->idx_s - q_data->idx_e;
		}
	}

	return blank >= size;
}

int32_t abox_msg_send(struct abox_msg_cmd *cmd,
		const struct abox_msg_send_data *data, int count)
{
	struct abox_msg_data *p_data;
	struct abox_msg_cmd_queue *q_cmd = &tx->q_cmd;
	struct abox_msg_data_queue *q_data = &tx->q_data;
	volatile int32_t *idx_s = &q_cmd->idx_s;
	uint32_t cmd_len, data_len, data_size;
	int32_t ret = ret_ok;
	int i;
	int busy_log;

	data_len = q_data->len;
	data_size = offsetof(struct abox_msg_data, data);
	for (i = 0; i < count; i++)
		data_size += data[i].size;

	cmd_len = sizeof(q_cmd->elem) / sizeof(q_cmd->elem[0]);
	cmd->time_put = get_time();
	cmd->time_get = 0;

	tx_lock();

	/* queue data */
	busy_log = 0;
	while (!is_avail(q_data, data_size)) {
		if (!busy_log) {
			print_log("%s: busy\n", __func__);
			busy_log = 1;
		}

		if (get_time() - cmd->time_put > TIMEOUT_NS) {
			print_log("%s: timeout: s=%d, e=%d, size=%u\n",
					__func__, q_data->idx_s, q_data->idx_e,
					data_size);
			ret = ret_err;
			goto unlock;
		}
	}
	p_data = (struct abox_msg_data *)&q_data->elem[q_data->idx_e];
	q_data->idx_e = (q_data->idx_e + data_size) % data_len;
	p_data->size = 0;
	for (i = 0; i < count; i++) {
		memcpy(p_data->data + p_data->size, data[i].data, data[i].size);
		p_data->size += data[i].size;
	}

	/* queue cmd */
	cmd->data_idx = (int8_t *)p_data - q_data->elem;
	cmd->send.data = p_data;
	busy_log = 0;
	while (*idx_s == ((q_cmd->idx_e + 1) % cmd_len)) {
		if (!busy_log) {
			print_log("%s: busy\n", __func__);
			busy_log = 1;
		}

		if (get_time() - cmd->time_put > TIMEOUT_NS) {
			print_log("%s: timeout: %d, %d, %d, %d, %d, %llu\n",
					__func__, cmd->id, cmd->cmd,
					cmd->arg[0], cmd->arg[1], cmd->arg[2],
					cmd->time_put);
			ret = ret_err;
			goto unlock;
		}
	}
	q_cmd->elem[q_cmd->idx_e] = *cmd;
	/* complete memcpy before increasing queue index */
	mb();
	q_cmd->idx_e = (q_cmd->idx_e + 1) % cmd_len;
unlock:
	tx_unlock();

	return ret;
}

int32_t abox_msg_flush(void)
{
	struct abox_msg_cmd_queue *q_cmd = &tx->q_cmd;
	volatile int32_t *idx_s = &q_cmd->idx_s;
	uint64_t time = get_time();

	while (*idx_s != q_cmd->idx_e) {
		if (get_time() - time > FLUSH_TIMEOUT_NS) {
			struct abox_msg_cmd *cmd;

			cmd = &q_cmd->elem[*idx_s];
			print_log("%s: timeout: %d, %d, %d, %d, %d, %llu\n",
				__func__, cmd->id, cmd->cmd, cmd->arg[0],
				cmd->arg[1], cmd->arg[2], cmd->time_put);
			return  ret_err;
		}
	}

	return ret_ok;
}

int32_t abox_msg_recv(struct abox_msg_cmd *cmd, void *data, int32_t size)
{
	struct abox_msg_cmd_queue *q_cmd = &rx->q_cmd;
	struct abox_msg_data_queue *q_data = &rx->q_data;
	volatile int32_t *idx_e = &q_cmd->idx_e;
	struct abox_msg_cmd *p_cmd;
	struct abox_msg_data *p_data;
	uint32_t cmd_len, data_len, data_size;
	int32_t ret;

	cmd_len = sizeof(q_cmd->elem) / sizeof(q_cmd->elem[0]);
	data_len = q_data->len;

	rx_lock();

	if (q_cmd->idx_s == *idx_e) {
		/* ipc read msg queue until empty on every interrupt.
		 * Reporting it through log is nothing but annoying.
		 * print_log("%s: empty\n", __func__);
		 */
		ret = ret_err;
		goto unlock;
	}

	p_cmd = &q_cmd->elem[q_cmd->idx_s];
	p_data = (struct abox_msg_data *)&q_data->elem[p_cmd->data_idx];
	if (size < p_data->size) {
		print_log("%s: buffer size(%u) < received size(%u)\n",
				__func__, size, p_data->size);
	}
	p_cmd->time_get = get_time();
	p_cmd->recv.data = p_data;

	if (cmd)
		*cmd = *p_cmd;
	if (data)
		memcpy(data, p_data->data, min(size, p_data->size));
	ret = p_data->size;
	data_size = p_data->size + offsetof(struct abox_msg_data, data);

	q_cmd->idx_s = (q_cmd->idx_s + 1) % cmd_len;
	q_data->idx_s = (p_cmd->data_idx + data_size) % data_len;
unlock:
	rx_unlock();

	return ret;
}

int32_t abox_msg_init(const struct abox_msg_cfg *cfg)
{
	unsigned int data_offset = offsetof(struct abox_msg_queue, q_data);
	unsigned int elem_offset = offsetof(struct abox_msg_data_queue, elem);

	ret_ok = cfg->ret_ok;
	ret_err = cfg->ret_err;

	if (cfg->tx_size < sizeof(*tx) || cfg->rx_size < sizeof(*rx))
		return ret_err;

	tx = cfg->tx_addr;
	tx->q_data.len = cfg->tx_size - data_offset - elem_offset;
	rx = cfg->rx_addr;
	rx->q_data.len = cfg->rx_size - data_offset - elem_offset;
	tx_lock = cfg->tx_lock_f;
	tx_unlock = cfg->tx_unlock_f;
	rx_lock = cfg->rx_lock_f;
	rx_unlock = cfg->rx_unlock_f;
	get_time = cfg->get_time_f;
	print_log = cfg->print_log_f;

	return ret_ok;
}
