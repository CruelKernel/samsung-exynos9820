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

#define LIMIT_DELAY_CNT		200
#define RECEIVEBUFFERSIZE	12
#define DEBUG_SHOW_DATA	0

void clean_msg(struct ssp_msg *msg)
{
	if (msg->free_buffer)
		kfree(msg->buffer);
	kfree(msg);
}

static int do_transfer(struct ssp_data *data, struct ssp_msg *msg,
		struct completion *done, int timeout)
{
	return bbd_do_transfer(data, msg, done, timeout);
}

int ssp_spi_async(struct ssp_data *data, struct ssp_msg *msg)
{
	int status = 0;
	u64 diff = get_current_timestamp() - data->resumeTimestamp;
	int timeout = diff < 5000000000ULL ? 400 : 1000; // unit: ms

	if (msg->length)
		return ssp_spi_sync(data, msg, timeout);

	status = do_transfer(data, msg, NULL, 0);

	return status;
}

int ssp_spi_sync(struct ssp_data *data, struct ssp_msg *msg, int timeout)
{
	DECLARE_COMPLETION_ONSTACK(done);
	int status = 0;

	if (msg->length == 0) {
		pr_err("[SSP] : %s length must not be 0\n", __func__);
		clean_msg(msg);
		return status;
	}

	status = do_transfer(data, msg, &done, timeout);

	return status;
}

int select_irq_msg(struct ssp_data *data)
{
	struct ssp_msg *msg, *n;
	bool found = false;
	u16 chLength = 0, msg_options = 0;
	u8 msg_type = 0;
	int iRet = 0;
	char *buffer;
	char chTempBuf[4] = { -1 };

	data->bHandlingIrq = true;
	iRet = spi_read(data->spi, chTempBuf, sizeof(chTempBuf));
	if (iRet < 0) {
		pr_err("[SSP] %s spi_read fail, ret = %d\n", __func__, iRet);
		data->bHandlingIrq = false;
		return iRet;
	}

	memcpy(&msg_options, &chTempBuf[0], 2);
	msg_type = msg_options & SSP_SPI_MASK;
	memcpy(&chLength, &chTempBuf[2], 2);

	switch (msg_type) {
	case AP2HUB_READ:
	case AP2HUB_WRITE:
		mutex_lock(&data->pending_mutex);
		if (!list_empty(&data->pending_list)) {
			list_for_each_entry_safe(msg, n,
				&data->pending_list, list) {

				if (msg->options == msg_options) {
					list_del(&msg->list);
					found = true;
					break;
				}
			}

			if (!found) {
				pr_err("[SSP]: %s %d - Not match error\n",
					__func__, msg_options);
				goto exit;
			}

			if (msg->dead && !msg->free_buffer) {
				msg->buffer = kzalloc(msg->length, GFP_KERNEL);
				msg->free_buffer = 1;
			} /* For dead msg, make a temporary buffer to read. */

			if (msg_type == AP2HUB_READ) {
				iRet = spi_read(data->spi,
						msg->buffer, msg->length);
			}
			if (msg_type == AP2HUB_WRITE) {
				iRet = spi_write(data->spi,
						msg->buffer, msg->length);

				if (msg_options & AP2HUB_RETURN) {
					msg->options = AP2HUB_READ
							| AP2HUB_RETURN;
					msg->length = 1;
					list_add_tail(&msg->list,
						&data->pending_list);
					goto exit;
				}
			}

			if (msg->done != NULL && !completion_done(msg->done))
				complete(msg->done);
			if (msg->dead_hook != NULL)
				*(msg->dead_hook) = true;

			clean_msg(msg);
		} else
			pr_err("[SSP]List empty error(%d)\n", msg_type);
exit:
		mutex_unlock(&data->pending_mutex);
		break;
	case HUB2AP_WRITE:
		buffer = kzalloc(chLength, GFP_KERNEL);
		iRet = spi_read(data->spi, buffer, chLength);
		if (iRet < 0)
			pr_err("[SSP] %s spi_read fail\n", __func__);
		else
			parse_dataframe(data, buffer, chLength);
		kfree(buffer);
		break;
	default:
		pr_err("[SSP]No type error(%d)\n", msg_type);
		break;
	}

	if (iRet < 0) {
		pr_err("[SSP]: %s - MSG2SSP_SSD error %d\n", __func__, iRet);
		data->bHandlingIrq = false;
		return ERROR;
	}

	data->bHandlingIrq = false;
	return SUCCESS;
}

void clean_pending_list(struct ssp_data *data)
{
	struct ssp_msg *msg, *n;

	mutex_lock(&data->pending_mutex);
	list_for_each_entry_safe(msg, n, &data->pending_list, list)	{
		list_del(&msg->list);
		if (msg->done != NULL && !completion_done(msg->done))
			complete(msg->done);
		if (msg->dead_hook != NULL)
			*(msg->dead_hook) = true;

		clean_msg(msg);
	}
	mutex_unlock(&data->pending_mutex);
}

int ssp_send_cmd(struct ssp_data *data, char command, int arg)
{
	int iRet = 0;
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	if (msg == NULL) {
		iRet = -ENOMEM;
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n",
			__func__);
		return iRet;
	}
	msg->cmd = command;
	msg->length = 0;
	msg->options = AP2HUB_WRITE;
	msg->data = arg;
	msg->free_buffer = 0;

	iRet = ssp_spi_async(data, msg);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - command 0x%x failed %d\n",
				__func__, command, iRet);
		return ERROR;
	}

	ssp_dbg("[SSP]: %s - command 0x%x %d\n", __func__, command, arg);

	return SUCCESS;
}

int send_instruction(struct ssp_data *data, u8 uInst,
	u8 uSensorType, u8 *uSendBuf, u16 uLength)
{
	char command;
	int iRet = 0;
	struct ssp_msg *msg;

	//u64 current_Ts = 0;
#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING
	u64 timestamp;
#endif

	if ((!(data->uSensorState & (1ULL << uSensorType)))
		&& (uInst <= CHANGE_DELAY)) {
		pr_err("[SSP]: %s - Bypass Inst Skip! - %u\n",
			__func__, uSensorType);
		return FAIL;
	}

	if (uSensorType >= SENSOR_MAX && (uInst == ADD_SENSOR || uInst == CHANGE_DELAY)){
		pr_err("[SSP]: %s - Invalid SensorType! - %u\n",
			__func__, uSensorType);
		return FAIL;
	}

	switch (uInst) {
	case REMOVE_SENSOR:
		command = MSG2SSP_INST_BYPASS_SENSOR_REMOVE;
		break;
	case ADD_SENSOR:
		command = MSG2SSP_INST_BYPASS_SENSOR_ADD;
#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING
		timestamp = get_current_timestamp();
		data->lastTimestamp[uSensorType] = data->LastSensorTimeforReset[uSensorType] = timestamp;
		data->ts_avg_buffer_idx[uSensorType] = 0;
		data->ts_avg_buffer_cnt[uSensorType] = 0;
		data->ts_avg_buffer_sum[uSensorType] = 0;
		data->ts_prev_index[uSensorType] = 0;
		data->ts_avg_skip_cnt[uSensorType] = SKIP_CNT_MOVING_AVG_ADD;
		data->ts_last_enable_cmd_time = timestamp;
		memset(data->ts_avg_buffer[uSensorType], 0,
				sizeof(u64)*SIZE_MOVING_AVG_BUFFER);
#endif
		data->first_sensor_data[uSensorType] = true;
		break;
	case CHANGE_DELAY:
		command = MSG2SSP_INST_CHANGE_DELAY;
#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING
		timestamp = get_current_timestamp();
		data->LastSensorTimeforReset[uSensorType] = timestamp;
		data->ts_avg_buffer_idx[uSensorType] = 0;
		data->ts_avg_buffer_cnt[uSensorType] = 0;
		data->ts_avg_buffer_sum[uSensorType] = 0;
		data->ts_prev_index[uSensorType] = 0;
		data->ts_avg_skip_cnt[uSensorType] = SKIP_CNT_MOVING_AVG_CHANGE;
		memset(data->ts_avg_buffer[uSensorType], 0,
				sizeof(u64)*SIZE_MOVING_AVG_BUFFER);
#endif
		data->first_sensor_data[uSensorType] = true;
		break;
	case GO_SLEEP:
		command = MSG2SSP_AP_STATUS_SLEEP;
		data->uLastAPState = MSG2SSP_AP_STATUS_SLEEP;
		break;
	case REMOVE_LIBRARY:
		command = MSG2SSP_INST_LIBRARY_REMOVE;
		break;
	case ADD_LIBRARY:
		command = MSG2SSP_INST_LIBRARY_ADD;
		break;
	default:
		command = uInst;
		break;
	}

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	if (msg == NULL) {
		iRet = -ENOMEM;
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n",
			__func__);
		return iRet;
	}
	msg->cmd = command;
	msg->length = uLength + 1;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(uLength + 1, GFP_KERNEL);
	msg->free_buffer = 1;

	msg->buffer[0] = uSensorType;
	memcpy(&msg->buffer[1], uSendBuf, uLength);

	ssp_dbg("[SSP]: %s - Inst = 0x%x, Sensor Type = 0x%x, data = %u\n",
			__func__, command, uSensorType, msg->buffer[1]);

	iRet = ssp_spi_async(data, msg);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - Instruction CMD Fail %d\n", __func__, iRet);
		return ERROR;
	}

	if (uInst == ADD_SENSOR || uInst == CHANGE_DELAY) {
		unsigned int BatchTimeforReset = 0;
	//current_Ts = get_current_timestamp();
		if (uLength >= 9)
			BatchTimeforReset = *(unsigned int *)(&uSendBuf[4]);// Add / change normal case, not factory.
	//pr_info("[SSP] %s timeForRest %d", __func__, BatchTimeforReset);
		data->IsBypassMode[uSensorType] = (BatchTimeforReset == 0);
	//pr_info("[SSP] sensor%d mode%d Time %lld\n", uSensorType, data->IsBypassMode[uSensorType], current_Ts);
	}
	return iRet;
}

int send_instruction_sync(struct ssp_data *data, u8 uInst,
	u8 uSensorType, u8 *uSendBuf, u16 uLength)
{
	char command;
	int iRet = 0;
	char buffer[10] = { 0, };
	struct ssp_msg *msg;

	//u64 current_Ts = 0;
#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING
	u64 timestamp;
#endif

	if ((!(data->uSensorState & (1ULL << uSensorType)))
		&& (uInst <= CHANGE_DELAY)) {
		pr_err("[SSP]: %s - Bypass Inst Skip! - %u\n",
			__func__, uSensorType);
		return FAIL;
	}

	switch (uInst) {
	case REMOVE_SENSOR:
		command = MSG2SSP_INST_BYPASS_SENSOR_REMOVE;
		break;
	case ADD_SENSOR:
		command = MSG2SSP_INST_BYPASS_SENSOR_ADD;
#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING
		timestamp = get_current_timestamp();
		data->lastTimestamp[uSensorType] = data->LastSensorTimeforReset[uSensorType] = timestamp;
		data->ts_avg_buffer_idx[uSensorType] = 0;
		data->ts_avg_buffer_cnt[uSensorType] = 0;
		data->ts_avg_buffer_sum[uSensorType] = 0;
		data->ts_prev_index[uSensorType] = 0;
		data->ts_avg_skip_cnt[uSensorType] = SKIP_CNT_MOVING_AVG_ADD;
		data->ts_last_enable_cmd_time = timestamp;
		memset(data->ts_avg_buffer[uSensorType], 0,
				sizeof(u64)*SIZE_MOVING_AVG_BUFFER);
#endif
		data->first_sensor_data[uSensorType] = true;
		break;
	case CHANGE_DELAY:
		command = MSG2SSP_INST_CHANGE_DELAY;
#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING
		timestamp = get_current_timestamp();
		data->LastSensorTimeforReset[uSensorType] = timestamp;
		data->ts_avg_buffer_idx[uSensorType] = 0;
		data->ts_avg_buffer_cnt[uSensorType] = 0;
		data->ts_avg_buffer_sum[uSensorType] = 0;
		data->ts_prev_index[uSensorType] = 0;
		data->ts_avg_skip_cnt[uSensorType] = SKIP_CNT_MOVING_AVG_CHANGE;
		memset(data->ts_avg_buffer[uSensorType], 0,
				sizeof(u64)*SIZE_MOVING_AVG_BUFFER);
#endif
		data->first_sensor_data[uSensorType] = true;
		break;
	case GO_SLEEP:
		command = MSG2SSP_AP_STATUS_SLEEP;
		data->uLastAPState = MSG2SSP_AP_STATUS_SLEEP;
		break;
	case REMOVE_LIBRARY:
		command = MSG2SSP_INST_LIBRARY_REMOVE;
		break;
	case ADD_LIBRARY:
		command = MSG2SSP_INST_LIBRARY_ADD;
		break;
	default:
		command = uInst;
		break;
	}

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	if (msg == NULL) {
		iRet = -ENOMEM;
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n",
			__func__);
		return iRet;
	}
	msg->cmd = command;
	msg->length = uLength + 1;
	msg->options = AP2HUB_WRITE | AP2HUB_RETURN;
	msg->buffer = buffer;
	msg->free_buffer = 0;

	msg->buffer[0] = uSensorType;
	memcpy(&msg->buffer[1], uSendBuf, uLength);

	ssp_dbg("[SSP]: %s - Inst Sync = 0x%x, Sensor Type = %u, data = %u\n",
			__func__, command, uSensorType, msg->buffer[0]);

	iRet = ssp_spi_sync(data, msg, 1000);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - Instruction CMD Fail %d\n", __func__, iRet);
		return ERROR;
	}

	if (uInst == ADD_SENSOR || uInst == CHANGE_DELAY) {
		unsigned int BatchTimeforReset = 0;
	//current_Ts = get_current_timestamp();
		if (uLength >= 9)
			BatchTimeforReset = *(unsigned int *)(&uSendBuf[4]);// Add / change normal case, not factory.
	//pr_info("[SSP] %s timeForRest %d", __func__, BatchTimeforReset);
		data->IsBypassMode[uSensorType] = (BatchTimeforReset == 0);
	//pr_info("[SSP] sensor%d mode%d Time %lld\n", uSensorType, data->IsBypassMode[uSensorType], current_Ts);
	}
	return buffer[0];
}

int flush(struct ssp_data *data, u8 uSensorType)
{
	int iRet = 0;
	char buffer = 0;
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	if (msg == NULL) {
		iRet = -ENOMEM;
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n",
			__func__);
		return iRet;
	}
	msg->cmd = MSG2SSP_AP_MCU_BATCH_FLUSH;
	msg->length = 1;
	msg->options = AP2HUB_READ;
	msg->data = uSensorType;
	msg->buffer = &buffer;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - fail %d\n", __func__, iRet);
		return ERROR;
	}

	ssp_dbg("[SSP]: %s Sensor Type = 0x%x, data = %u\n",
			__func__, uSensorType, buffer);

	return buffer ? 0 : -1;
}

int get_batch_count(struct ssp_data *data, u8 uSensorType)
{
	int iRet = 0;
	s32 result = 0;
	char buffer[4] = {0, };
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	if (msg == NULL) {
		iRet = -ENOMEM;
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n",
			__func__);
		return iRet;
	}
	msg->cmd = MSG2SSP_AP_MCU_BATCH_COUNT;
	msg->length = 4;
	msg->options = AP2HUB_READ;
	msg->data = uSensorType;
	msg->buffer = buffer;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - fail %d\n", __func__, iRet);
		return ERROR;
	}

	memcpy(&result, buffer, 4);

	ssp_dbg("[SSP]: %s Sensor Type = 0x%x, data = %u\n",
			__func__, uSensorType, result);

	return result;
}

int get_chipid(struct ssp_data *data)
{
	int iRet, iReties = 0;
	char buffer = 0;
	struct ssp_msg *msg;

retries:
	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	if (msg == NULL) {
		iRet = -ENOMEM;
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n",
			__func__);
		return iRet;
	}
	msg->cmd = MSG2SSP_AP_WHOAMI;
	msg->length = 1;
	msg->options = AP2HUB_READ;
	msg->buffer = &buffer;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);

	if (buffer != DEVICE_ID && iReties++ < 2) {
		mdelay(5);
		pr_err("[SSP] %s - get chip ID retry\n", __func__);
		goto retries;
	}

	if (iRet == SUCCESS)
		return buffer;

	pr_err("[SSP] %s - get chip ID failed %d\n", __func__, iRet);
	return ERROR;
}

int set_ap_information(struct ssp_data *data)
{
	int iRet = 0;
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	if (msg == NULL) {
		iRet = -ENOMEM;
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n",
				__func__);
		return iRet;
	}
	msg->cmd = MSG2SSP_AP_INFORMATION;
	msg->length = 2;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(2, GFP_KERNEL);
	msg->free_buffer = 1;
	msg->buffer[0] = (char)data->ap_type;
	msg->buffer[1] = (char)data->ap_rev;

	iRet = ssp_spi_async(data, msg);

	pr_info("[SSP] AP TYPE = %d AP REV = %d\n", data->ap_type, data->ap_rev);

	if (iRet != SUCCESS) {
		pr_err("[SSP] %s -fail to %s %d\n",
			__func__, __func__, iRet);
		iRet = ERROR;
	}

	return iRet;
}

int set_sensor_position(struct ssp_data *data)
{
	int iRet = 0;
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	if (msg == NULL) {
		iRet = -ENOMEM;
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n",
			__func__);
		return iRet;
	}
	msg->cmd = MSG2SSP_AP_SENSOR_FORMATION;
	msg->length = 3;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(3, GFP_KERNEL);
	msg->free_buffer = 1;

	msg->buffer[0] = data->accel_position;
	msg->buffer[1] = data->accel_position;
	msg->buffer[2] = data->mag_position;

	iRet = ssp_spi_async(data, msg);

	pr_info("[SSP] Sensor Posision A : %u, G : %u, M: %u, P: %u\n",
			data->accel_position,
			data->accel_position,
			data->mag_position, 0);

	if (iRet != SUCCESS) {
		pr_err("[SSP] %s -fail to %s %d\n",
			__func__, __func__, iRet);
		iRet = ERROR;
	}

	return iRet;
}

#ifdef CONFIG_SENSORS_MULTIPLE_GLASS_TYPE
int set_glass_type(struct ssp_data *data)
{
	int iRet = 0;
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	if (msg == NULL) {
		iRet = -ENOMEM;
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n",
			__func__);
		return iRet;
	}
	msg->cmd = MSG2SSP_AP_GLASS_TYPE;
	msg->length = 1;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(1, GFP_KERNEL);
	msg->free_buffer = 1;

	msg->buffer[0] = data->glass_type;

	iRet = ssp_spi_async(data, msg);

	pr_info("[SSP] glass_type : %u\n", data->glass_type);

	if (iRet != SUCCESS) {
		pr_err("[SSP] %s -fail to %s %d\n",
			__func__, __func__, iRet);
		iRet = ERROR;
	}

	return iRet;
}
#endif

int set_magnetic_static_matrix(struct ssp_data *data)
{
	int iRet = 0;
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	if (msg == NULL) {
		iRet = -ENOMEM;
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n",
			__func__);
		return iRet;
	}
	msg->cmd = MSG2SSP_AP_SET_MAGNETIC_STATIC_MATRIX;
	msg->length = data->mag_matrix_size;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(data->mag_matrix_size, GFP_KERNEL);
	msg->free_buffer = 1;

	memcpy(msg->buffer, data->mag_matrix, data->mag_matrix_size);

	iRet = ssp_spi_async(data, msg);
	if (iRet != SUCCESS) {
		pr_err("[SSP] %s -fail to %s %d\n",
				__func__, __func__, iRet);
		iRet = ERROR;
	}

	return iRet;
}

void set_proximity_threshold(struct ssp_data *data)
{
	int iRet = 0;

	struct ssp_msg *msg;

	if (!(data->uSensorState & (1 << PROXIMITY_SENSOR))) {
		pr_info("[SSP]: %s - Skip this function!!!,proximity sensor is not connected(0x%llx)\n",
			__func__, data->uSensorState);
		return;
	}

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
#if defined(CONFIG_SENSORS_SSP_TMG399x)
	msg->cmd = MSG2SSP_AP_SENSOR_PROXTHRESHOLD;
	msg->length = 2;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(2, GFP_KERNEL);
	msg->free_buffer = 1;

	msg->buffer[0] = (char)data->uProxHiThresh;
	msg->buffer[1] = (char)data->uProxLoThresh;
#elif defined(CONFIG_SENSORS_SSP_PROX_AUTOCAL_AMS)
	msg->cmd = MSG2SSP_AP_SENSOR_PROXTHRESHOLD;
	msg->length = 8;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(8, GFP_KERNEL);
	msg->free_buffer = 1;

	msg->buffer[0] = ((char) (data->uProxHiThresh >> 8) & 0xff);
	msg->buffer[1] = (char) data->uProxHiThresh;
	msg->buffer[2] = ((char) (data->uProxLoThresh >> 8) & 0xff);
	msg->buffer[3] = (char) data->uProxLoThresh;
	msg->buffer[4] = ((char) (data->uProxHiThresh_detect >> 8) & 0xff);
	msg->buffer[5] = (char) data->uProxHiThresh_detect;
	msg->buffer[6] = ((char) (data->uProxLoThresh_detect >> 8) & 0xff);
	msg->buffer[7] = (char) data->uProxLoThresh_detect;
#else /* CONFIG_SENSORS_SSP_PROX_FACTORYCAL */
	msg->cmd = MSG2SSP_AP_SENSOR_PROXTHRESHOLD;
	msg->length = 4;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(4, GFP_KERNEL);
	msg->free_buffer = 1;

	msg->buffer[0] = ((char) (data->uProxHiThresh >> 8) & 0xff);
	msg->buffer[1] = (char) data->uProxHiThresh;
	msg->buffer[2] = ((char) (data->uProxLoThresh >> 8) & 0xff);
	msg->buffer[3] = (char) data->uProxLoThresh;
#endif

	iRet = ssp_spi_async(data, msg);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - SENSOR_PROXTHRESHOLD CMD fail %d\n",
			__func__, iRet);
		return;
	}

#if defined(CONFIG_SENSORS_SSP_PROX_AUTOCAL_AMS)
	pr_info("[SSP]: Proximity Threshold - %u, %u, %u, %u, efs file status %d\n",
		data->uProxHiThresh, data->uProxLoThresh,
		data->uProxHiThresh_detect, data->uProxLoThresh_detect, data->light_efs_file_status);
#else
	pr_info("[SSP]: Proximity Threshold - %u, %u\n",
		data->uProxHiThresh, data->uProxLoThresh);
#endif
}

void set_proximity_alert_threshold(struct ssp_data *data)
{
	int iRet = 0;

	struct ssp_msg *msg;

	if (!(data->uSensorState & (1 << PROXIMITY_ALERT_SENSOR))) {
		pr_info("[SSP]: %s - Skip this function!!!,proximity alert sensor is not connected(0x%llx)\n",
			__func__, data->uSensorState);
		return;
	}

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	msg->cmd = MSG2SSP_AP_SENSOR_PROX_ALERT_THRESHOLD;
	msg->length = 2;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(2, GFP_KERNEL);
	msg->free_buffer = 1;

	msg->buffer[0] = ((char) (data->uProxAlertHiThresh >> 8) & 0xff);
	msg->buffer[1] = (char) data->uProxAlertHiThresh;

	iRet = ssp_spi_async(data, msg);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - SENSOR_PROX_ALERT_THRESHOLD CMD fail %d\n",
			__func__, iRet);
		return;
	}

	pr_info("[SSP]: %s Proximity alert Threshold - %u\n",
		__func__, data->uProxAlertHiThresh);
}

void set_light_coef(struct ssp_data *data)
{
	int iRet = 0;
	struct ssp_msg *msg;

	if (!(data->uSensorState & (1 << LIGHT_SENSOR))) {
		pr_info("[SSP]: %s - Skip this function!!!,light sensor is not connected(0x%llx)\n",
			__func__, data->uSensorState);
		return;
	}

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	msg->cmd = MSG2SSP_AP_SET_LIGHT_COEF;
	msg->length = sizeof(data->light_coef);
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(sizeof(data->light_coef), GFP_KERNEL);
	msg->free_buffer = 1;

	memcpy(msg->buffer, data->light_coef, sizeof(data->light_coef));

	iRet = ssp_spi_async(data, msg);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - MSG2SSP_AP_SET_LIGHT_COEF CMD fail %d\n",
			__func__, iRet);
		return;
	}

	pr_info("[SSP]: %s - %d %d %d %d %d %d %d\n", __func__,
		data->light_coef[0], data->light_coef[1], data->light_coef[2],
		data->light_coef[3], data->light_coef[4], data->light_coef[5], data->light_coef[6]);
}

void set_proximity_barcode_enable(struct ssp_data *data, bool bEnable)
{
	int iRet = 0;
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	msg->cmd = MSG2SSP_AP_SENSOR_BARCODE_EMUL;
	msg->length = 1;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(1, GFP_KERNEL);
	msg->free_buffer = 1;

	data->bBarcodeEnabled = bEnable;
	msg->buffer[0] = bEnable;

	iRet = ssp_spi_async(data, msg);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - SENSOR_BARCODE_EMUL CMD fail %d\n",
				__func__, iRet);
		return;
	}

	pr_info("[SSP] Proximity Barcode En : %u\n", bEnable);
}

void set_gesture_current(struct ssp_data *data, unsigned char uData1)
{
	int iRet = 0;
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	msg->cmd = MSG2SSP_AP_SENSOR_GESTURE_CURRENT;
	msg->length = 1;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(1, GFP_KERNEL);
	msg->free_buffer = 1;

	msg->buffer[0] = uData1;
	iRet = ssp_spi_async(data, msg);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - SENSOR_GESTURE_CURRENT CMD fail %d\n",
			__func__, iRet);
		return;
	}

	pr_info("[SSP]: Gesture Current Setting - %u\n", uData1);
}

int set_hall_threshold(struct ssp_data *data)
{
	int iRet = 0;
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	if (msg == NULL) {
		iRet = -ENOMEM;
		pr_err("[SSP]: %s - failed to alloc memory for ssp_msg\n",
			__func__);
		return iRet;
	}

	msg->cmd = MSG2SSP_AP_SET_HALL_THRESHOLD;
	msg->length = sizeof(data->hall_threshold);
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(sizeof(data->hall_threshold), GFP_KERNEL);
	msg->free_buffer = 1;

	memcpy(msg->buffer, data->hall_threshold,
			sizeof(data->hall_threshold));

	iRet = ssp_spi_async(data, msg);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - fail to %s %d\n",
			__func__, __func__, iRet);
		iRet = ERROR;
	}

	return iRet;
}

u64 get_sensor_scanning_info(struct ssp_data *data)
{
	int iRet = 0, z = 0, cnt = 1;
	u64 result = 0, len = sizeof(data->sensor_state);
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);
#if defined (CONFIG_SENSORS_SSP_DAVINCI)
	int disable_sensor_type[10] = {7,9,10,24,30,31,32,35,39,40};
#endif
	
	if (msg == NULL) {
		iRet = -ENOMEM;
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n",
			__func__);
		return iRet;
	}
	msg->cmd = MSG2SSP_AP_SENSOR_SCANNING;
	msg->length = 8;
	msg->options = AP2HUB_READ;
	msg->buffer = (char *) &result;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);

	if (iRet != SUCCESS)
		pr_err("[SSP]: %s - i2c fail %d\n", __func__, iRet);

	for (z = 0; z < SENSOR_MAX; z++) {
		if(z % 10 == 0 && z != 0)
			data->sensor_state[len - 1 - cnt++] = ' ';
		data->sensor_state[len - 1 - cnt++] = (result & (1ULL << z)) ? '1' : '0';
	}
	data->sensor_state[cnt - 1] = '\0';
#if defined (CONFIG_SENSORS_SSP_DAVINCI)
	if (data->ap_rev == 18 &&  data->ap_type <= 1) {
		int j;
		iRet = 0; len = 0;
		for (j = 0; j < SENSOR_MAX; j++) {
			if (disable_sensor_type[iRet] == j) {
				iRet++;
				continue;
			}
			if (result & (1ULL << j))
				len |= (1ULL << j);
		}
//		for(int i = 0; i < ARRAY_SIZE(disable_sensor_type); i++) {
//			data->sensor_state[sizeof(data->sensor_state)- (disable_sensor_type[i] + (disable_sensor_type[i] / 10)+1)] = '0'; // divide 10 for blank
//		}
		result = len;
	} // D1 D1x && hw _rev 18 disable prox light sensor type
#endif
	pr_err("[SSP]: state: %s\n", data->sensor_state);

	return result;
}

unsigned int get_firmware_rev(struct ssp_data *data)
{
	int iRet;
	u32 result = SSP_INVALID_REVISION;
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	if (msg == NULL) {
		iRet = -ENOMEM;
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n",
			__func__);
		return iRet;
	}
	msg->cmd = MSG2SSP_AP_FIRMWARE_REV;
	msg->length = 4;
	msg->options = AP2HUB_READ;
	msg->buffer = (char *) &result;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);
	if (iRet != SUCCESS)
		pr_err("[SSP]: %s - transfer fail %d\n", __func__, iRet);

	return result;
}

int set_big_data_start(struct ssp_data *data, u8 type, u32 length)
{
	int iRet = 0;
	struct ssp_msg *msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	if (msg == NULL) {
		iRet = -ENOMEM;
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n",
			__func__);
		return iRet;
	}
	msg->cmd = MSG2SSP_AP_START_BIG_DATA;
	msg->length = 5;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(5, GFP_KERNEL);
	msg->free_buffer = 1;

	msg->buffer[0] = type;
	memcpy(&msg->buffer[1], &length, 4);

	iRet = ssp_spi_async(data, msg);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - i2c fail %d\n", __func__, iRet);
		iRet = ERROR;
	}

	return iRet;
}

int set_time(struct ssp_data *data)
{
	int iRet;
	struct ssp_msg *msg;
	struct timespec ts;
	struct rtc_time tm;

	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec, &tm);
	pr_info("[SSP]: %s %d-%02d-%02d %02d:%02d:%02d.%09lu UTC\n",
			__func__,
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	if (msg == NULL) {
		iRet = -ENOMEM;
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n",
			__func__);
		return iRet;
	}
	msg->cmd = MSG2SSP_AP_MCU_SET_TIME;
	msg->length = 12;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(12, GFP_KERNEL);
	msg->free_buffer = 1;

	msg->buffer[0] = tm.tm_hour;
	msg->buffer[1] = tm.tm_min;
	msg->buffer[2] = tm.tm_sec;
	msg->buffer[3] = tm.tm_hour > 11 ? 64 : 0;
	msg->buffer[4] = tm.tm_wday;
	msg->buffer[5] = tm.tm_mon + 1;
	msg->buffer[6] = tm.tm_mday;
	msg->buffer[7] = tm.tm_year % 100;
	memcpy(&msg->buffer[8], &ts.tv_nsec, 4);

	iRet = ssp_spi_async(data, msg);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - i2c fail %d\n", __func__, iRet);
		iRet = ERROR;
	}

	return iRet;
}

int get_time(struct ssp_data *data)
{
	int iRet;
	char buffer[12] = { 0, };
	struct ssp_msg *msg;
	struct timespec ts;
	struct rtc_time tm;

	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec, &tm);
	pr_info("[SSP]: %s ap %d-%02d-%02d %02d:%02d:%02d.%09lu UTC\n",
			__func__,
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	if (msg == NULL) {
		iRet = -ENOMEM;
		pr_err("[SSP] %s, failed to alloc memory for ssp_msg\n",
			__func__);
		return iRet;
	}
	msg->cmd = MSG2SSP_AP_MCU_GET_TIME;
	msg->length = 12;
	msg->options = AP2HUB_READ;
	msg->buffer = buffer;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - i2c failed %d\n", __func__, iRet);
		return 0;
	}

	tm.tm_hour = buffer[0];
	tm.tm_min = buffer[1];
	tm.tm_sec = buffer[2];
	tm.tm_mon = msg->buffer[5] - 1;
	tm.tm_mday = buffer[6];
	tm.tm_year = buffer[7] + 100;
	rtc_tm_to_time(&tm, &ts.tv_sec);
	memcpy(&ts.tv_nsec, &msg->buffer[8], 4);

	rtc_time_to_tm(ts.tv_sec, &tm);
	pr_info("[SSP]: %s mcu %d-%02d-%02d %02d:%02d:%02d.%09lu UTC\n",
			__func__,
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);

	return iRet;
}

void set_gyro_cal_lib_enable(struct ssp_data *data, bool bEnable)
{
	int iRet = 0;
	u8 cmd;
	struct ssp_msg *msg;

	pr_info("[SSP] %s - enable %d(cur %d)\n", __func__, bEnable, data->gyro_lib_state);

	if (bEnable)
		cmd = SH_MSG2AP_GYRO_CALIBRATION_START;
	else
		cmd = SH_MSG2AP_GYRO_CALIBRATION_STOP;

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	msg->cmd = cmd;
	msg->length = 1;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(1, GFP_KERNEL);
	msg->free_buffer = 1;

	msg->buffer[0] = bEnable;

	iRet = ssp_spi_async(data, msg);

	if (iRet == SUCCESS) {
		if (bEnable)
			data->gyro_lib_state = GYRO_CALIBRATION_STATE_REGISTERED;
		else
			data->gyro_lib_state = GYRO_CALIBRATION_STATE_DONE;
	} else
		pr_err("[SSP] %s - gyro lib enable cmd fail\n", __func__);

}

#if defined(CONFIG_SSP_MOTOR_CALLBACK)
int send_motor_state(struct ssp_data *data)
{
	int iRet = 0;
	struct ssp_msg *msg;

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	msg->cmd = MSG2SSP_AP_MCU_SET_MOTOR_STATUS;
	msg->length = 1;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(1, GFP_KERNEL);
	msg->free_buffer = 1;

	/*if 1: start, 0: stop*/
	msg->buffer[0] = data->motor_state;

	iRet = ssp_spi_async(data, msg);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - fail %d\n",
				__func__, iRet);
		return iRet;
	}
	pr_info("[SSP] %s - En : %u\n", __func__, data->motor_state);
	return data->motor_state;
}
#endif
u8 get_accel_range(struct ssp_data *data)
{
	int iRet = 0;
	struct ssp_msg *msg;
	u8 rxbuffer[1] = {0x00};

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);

	msg->cmd = MSG2SSP_AP_MCU_GET_ACCEL_RANGE;
	msg->length = 1;
	msg->options = AP2HUB_READ;
	msg->buffer = rxbuffer;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);
	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - fail %d\n",
				__func__, iRet);
		return iRet;
	}

	pr_info("[SSP] %s - Range : %u\n", __func__, rxbuffer[0]);
	return rxbuffer[0];
}


