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


bool ssp_dbg;
bool ssp_pkt_dbg;

#define dprint(fmt, args...) \
	do { \
		if (unlikely(ssp_dbg)) \
			pr_debug("[SSPBBD]:(%s:%d): " fmt, \
			__func__, __LINE__, ##args); \
	} while (0)

#define DEBUG_SHOW_HEX_SEND(msg, len) \
	do { \
		if (unlikely(ssp_pkt_dbg)) { \
			print_hex_dump(KERN_INFO, "SSP->MCU: ", \
			DUMP_PREFIX_NONE, 16, 1, (msg), (len), true); \
		} \
	} while (0)

#define DEBUG_SHOW_HEX_RECV(msg, len) \
	do { \
		if (unlikely(ssp_pkt_dbg)) {\
			print_hex_dump(KERN_INFO, "SSP<-MCU: ", \
			DUMP_PREFIX_NONE, 16, 1, (msg), (len), true); \
		} \
	} while (0)

enum packet_state_e {
	WAITFOR_PKT_HEADER = 0,
	WAITFOR_PKT_COMPLETE
};

#define MAX_SSP_DATA_SIZE	(4*BBD_MAX_DATA_SIZE)
struct ssp_packet_state {
	u8 buf[MAX_SSP_DATA_SIZE];
	s32 rxlen;

	enum packet_state_e state;
	bool islocked;

	u8 type;
	u16 opts;
	u16 required;
	struct ssp_msg *msg;
};
struct ssp_packet_state ssp_pkt;

/**
 * transfer ssp data to mcu thru bbd driver.
 *
 * @data:	ssp data pointer
 * @msg:	ssp message
 * @done:	completion
 * @timeout:	wait response time (ms)
 *
 * @return:	 1 = success, -1 = failed to send data to bbd
 *		-2 = failed to get response from mcu
 */
int bbd_do_transfer(struct ssp_data *data, struct ssp_msg *msg,
		struct completion *done, int timeout) {
	int status = 0;
	bool msg_dead = false, ssp_down = false;
	bool use_no_irq = false;
	int ssp_packet_size = 0;
	int ssp_msg_type = 0;

	if (data == NULL || msg == NULL) {
		pr_err("%s():[SSPBBD] data or msg is NULL\n", __func__);
		return -1;
	}

	ssp_packet_size = msg->length;
	ssp_msg_type = msg->options  & SSP_SPI_MASK;

	mutex_lock(&data->comm_mutex);

	if (timeout)
		wake_lock(&data->ssp_comm_wake_lock);

	ssp_down = data->bSspShutdown;

	if (ssp_down) {
		pr_err("[SSPBBD]: ssp_down == true. returning\n");
		clean_msg(msg);
		mdelay(5);
		if (timeout)
			wake_unlock(&data->ssp_comm_wake_lock);

		mutex_unlock(&data->comm_mutex);
		return -1;
	}

	msg->dead_hook = &msg_dead;
	msg->dead = false;
	msg->done = done;
	use_no_irq = (msg->length == 0);

	mutex_lock(&data->pending_mutex);

	if ((ssp_msg_type != AP2HUB_WRITE || ssp_packet_size <= MAX_SSP_PACKET_SIZE)
			&& bbd_send_packet((unsigned char *)msg, 9) > 0) {
		status = 1;
		DEBUG_SHOW_HEX_SEND(msg, 9);
	} else {
		pr_err("[SSP]: %s bbd_send_packet fail!!\n", __func__);
		if (ssp_packet_size <= MAX_SSP_PACKET_SIZE)
			data->uTimeOutCnt++;
		else
			pr_err("[SSPBBD]: packet size of ssp must be less than %d, but %d\n",
					MAX_SSP_PACKET_SIZE, (int)msg->length);
		clean_msg(msg);
		mutex_unlock(&data->pending_mutex);
		if (timeout)
			wake_unlock(&data->ssp_comm_wake_lock);

		mutex_unlock(&data->comm_mutex);
		return -1;
	}

	if (!use_no_irq) {
		/* moved UP */
		/* mutex_lock(&data->pending_mutex);*/
		list_add_tail(&msg->list, &data->pending_list);
		/* moved down */
		/* mutex_unlock(&data->pending_mutex);*/
	}

	mutex_unlock(&data->pending_mutex);

	if (status == 1 && done != NULL) {
		dprint("waiting completion ...\n");
		if (wait_for_completion_timeout(done,
				msecs_to_jiffies(timeout)) == 0) {

			pr_err("[SSPBBD] %s(): completion is timeout!\n",
				__func__);

			if (data->regulator_vdd_mcu_1p8 != NULL) {
				pr_err("[SSPBBD] %s(): state of mcu power(%d)\n", __func__,
						regulator_is_enabled(data->regulator_vdd_mcu_1p8));
			} else if (data->shub_en >= 0) {
				pr_err("[SSPBBD] %s: shub_en(%d), is enabled %d\n", __func__,
						data->shub_en, gpio_get_value(data->shub_en));
			}

			bcm4773_debug_info();

			status = -2;
                        
			mutex_lock(&data->pending_mutex);
			if (!use_no_irq && !msg_dead) {
				if ((msg->list.next != NULL) &&
					(msg->list.next != LIST_POISON1))
					list_del(&msg->list);
			}
			mutex_unlock(&data->pending_mutex);
		} else {
			dprint("completion is cleared !\n");
		}
	}

	mutex_lock(&data->pending_mutex);
	if (msg != NULL && !msg_dead) {
		msg->done = NULL;
		msg->dead_hook = NULL;

		if (status != 1)
			msg->dead = true;
		if (status == -2)
			data->uTimeOutCnt++;
	}
	mutex_unlock(&data->pending_mutex);

	if (use_no_irq)
		clean_msg(msg);

	if (timeout)
		wake_unlock(&data->ssp_comm_wake_lock);

	mutex_unlock(&data->comm_mutex);

	if (status == -2) {
		u64 current_timestamp = get_current_timestamp();

		pr_err("[SSPBBD] %s(): queue work to sensorhub reset(%lld, %lld)\n",
				__func__, data->resumeTimestamp, current_timestamp);
		schedule_delayed_work(&data->work_ssp_reset, msecs_to_jiffies(100));
	}

	return status;
}

/****************************************************************
 *
 * Callback functios
 *
 ****************************************************************/

/**
 * callback function:
 *	called this function by bbdpl when packet comes from MCU
 *
 * @ssh_data:	ssh data pointer
 *
 * @return:	0 = success, -1 = failure
 *
 */
int callback_bbd_on_packet_alarm(void *ssh_data)
{
	struct ssp_data *data = (struct ssp_data *)ssh_data;

	if (ssh_data == NULL)
		return -1;
#if 0
	/* check if already processing work queue */
	if (mutex_is_locked(&data->pending_mutex)) {
		dprint("already processing work queue\n");
		return 0;
	}
#endif
	/*data->bbd_on_packet_wq could be destroyed while shutdown. */
	mutex_lock(&shutdown_lock);

	if (data->bbd_on_packet_wq &&
		queue_work(data->bbd_on_packet_wq,
			&data->work_bbd_on_packet)) {

		/* in case of adding the work to queue */
		mutex_unlock(&shutdown_lock);
		dprint("queue_work ok!!\n");
		return 0;
	}
	mutex_unlock(&shutdown_lock);

	return 0;
}

/**
 * callback function
 *	called this function by bbdpl whenever mcu is(not) ready
 *
 * @ssh_data:	ssh data pointer
 * @ready:	true = ready, false = not ready
 *
 * @return:	0 = success, -1 = failure
 */
int callback_bbd_on_mcu_ready(void *ssh_data, bool ready)
{
	struct ssp_data *data = (struct ssp_data *)ssh_data;

	if (data == NULL)
		return -1;

	if (ready == true) {
		/* Start queue work for initializing MCU */
		data->bFirstRef == true ? data->bFirstRef = false : data->uResetCnt++;
		if (data->IsGpsWorking)
			data->resetCntGPSisOn++;
		memset(&ssp_pkt, 0, sizeof(ssp_pkt));
		ssp_pkt.required = 4;
		wake_lock_timeout(&data->ssp_wake_lock, HZ);
		queue_work(data->bbd_mcu_ready_wq, &data->work_bbd_mcu_ready);
	} else {
		/* Disable SSP */
		ssp_enable(data, false);
		dprint("MCU is not ready and disabled SSP\n");
	}

	return 0;
}

/**
 * callback function
 *	called this function by bbdpl when received control command from LHD
 *
 * @ssh_data:	ssh data pointer
 * @str_ctrl:	string type control command
 *
 * @return:	0 = success, -1 = failure
 */
 void makeResetInfoString(char *src, char *dst)
{
        int i, idx = 0, dstLen = (int)strlen(dst), totalLen = (int)strlen(src);
        
        for(i = 0; i < totalLen && idx < dstLen; i++) {
            if(src[i] == '"' || src[i] == '<' || src[i] == '>')
                continue;
            if(src[i] == ';')
                break;
                dst[idx++] = src[i];
        }
}
int callback_bbd_on_control(void *ssh_data, const char *str_ctrl)
{
	struct ssp_data *data = (struct ssp_data *)ssh_data;

	if (!ssh_data || !str_ctrl)
		return -1;

	if (strstr(str_ctrl, ESW_CTRL_CRASHED)) {
		data->IsMcuCrashed = true;
		data->mcuCrashedCnt++;
	} else if (strstr(str_ctrl, BBD_CTRL_GPS_OFF) || strstr(str_ctrl, BBD_CTRL_GPS_ON)) {
		data->IsGpsWorking = (strstr(str_ctrl, "CORE_OFF") ? 0 : 1);
	} else if (strstr(str_ctrl, BBD_CTRL_LHD_STOP)) {
		int prefixLen = (int)strlen(BBD_CTRL_LHD_STOP) + 1; //puls one is for blank ex) "LHD:STOP "
		int totalLen = (int)strlen(str_ctrl);
		int copyLen = totalLen - prefixLen <= sizeof(data->resetInfo) ? totalLen - prefixLen : sizeof(data->resetInfo);

		memcpy(data->resetInfo, str_ctrl + prefixLen, copyLen);
		
		if(totalLen != prefixLen) {			
			/* this value is used on debug work func */
			memset(data->resetInfoDebug, 0, sizeof(data->resetInfoDebug));
			makeResetInfoString(data->resetInfo, data->resetInfoDebug);
			data->resetInfoDebugTime = get_current_timestamp();
		}

	} else if (strstr(str_ctrl, SSP_GET_HW_REV)) {
		return data->ap_rev;
	} else if (strstr(str_ctrl, SSP_GET_AP_REV)) {
                return data->ap_type;
        } else if (strstr(str_ctrl, SSP_OIS_NOTIFY_RESET)) {
		if (data->ois_reset->ois_func != NULL) {
			data->ois_reset->ois_func(data->ois_reset->core);
		}
	}
	dprint("Received string command from LHD(=%s)\n", str_ctrl);

	return 0;
}
/**
 * callback function
 *	called this function by bbdpl when reset mcu from bbd
 *
 * @ssh_data:	ssh data pointer
 *
 * @return:	0 = success, -1 = failure
 */
int callback_bbd_on_mcu_reset(void *ssh_data, bool IsNoResp)
{
	struct ssp_data *data = (struct ssp_data *)ssh_data;

	if (!data)
		return -1;
        if(IsNoResp && !data->resetting)
            data->IsNoRespCnt++;

	data->resetting = true;
	//data->uResetCnt++;

	return 0;
}

/****************************************************************
 *
 * Work queue functios
 *
 ****************************************************************/

/**
 * Work queue function for MCU ready
 *	This is called by bbdpl when MCU is ready and then
 *	initialize MCU

 * @work:	work structure
 *
 * @return:	none
 */
void bbd_mcu_ready_work_func(struct work_struct *work)
{
	struct ssp_data *data = container_of(work,
					struct ssp_data, work_bbd_mcu_ready);
	int ret = 0;
	int retries = 0;


	if (data->vdd_mcu_1p8_name != NULL) {
		//clean_pending_list(data);
		ret = wait_for_completion_timeout(&data->hub_data->mcu_init_done, COMPLETION_TIMEOUT);
		if (unlikely(!ret))
			pr_err("[SSPBBD] Sensors of MCU are not ready!\n");
		else
			pr_err("[SSPBBD] Sensors of MCU are ready!\n");
	} else
		msleep(1000); //this model doesn't vdd divided

	dprint("MCU is ready.(work_queue)\n");

	clean_pending_list(data);
	ssp_enable(data, true);
retries:
	ret = initialize_mcu(data);
	if (ret != SUCCESS) {
		mdelay(100);
		if (++retries > 3) {
			pr_err("[SSPBBD] fail to initialize mcu\n");
			ssp_enable(data, false);
                        data->resetting = false;
			return;
		}
		goto retries;
	}
	dprint("mcu is initiialized (retries=%d)\n", retries);

#ifdef CONFIG_SENSORS_SSP_HIFI_BATCHING
	/* initialize variables for timestamp */
	ssp_reset_batching_resources(data);
#endif

	/* recover previous state */
	sync_sensor_state(data);
	ssp_sensorhub_report_notice(data, MSG2SSP_AP_STATUS_RESET);

	if (data->uLastAPState != 0)
		ssp_send_cmd(data, data->uLastAPState, 0);
	if (data->uLastResumeState != 0)
		ssp_send_cmd(data, data->uLastResumeState, 0);

	data->resetting = false;
}

/**
 * This work queue function starts from alarm callback function
 *
 * @work:	work structure
 *
 * @return:	none
 */
/* the timeout for getting data from bbdpl */
#define BBD_PULL_TIMEOUT	1000

#define MAX_SSP_DATA_SIZE	(4*BBD_MAX_DATA_SIZE)
unsigned char rBuff[MAX_SSP_DATA_SIZE] = {-1};

extern s64 get_sensor_time_delta_us(void);
void bbd_on_packet_work_func(struct work_struct *work)
{
	struct ssp_data *data = container_of(work,
					struct ssp_data, work_bbd_on_packet);
	unsigned short chLength = 0, msg_options = 0;
	unsigned char msg_type = 0;
	int iRet = 0;
	unsigned char *pData = NULL, *p, *q;
	int nDataLen = 0;
	u64 timestamp;

	iRet = bbd_pull_packet(rBuff, sizeof(rBuff), BBD_PULL_TIMEOUT);
	if (iRet <= 0) {
		/* This is not error condition because previous run
		 * of this function may have read all packets from BBD
		 * This happens if this work function is scheduled
		 * while it starts running.
		 */
		return;
	}

	p = rBuff;
	q = rBuff + iRet;/* q points end of currently received data bytes */

process_one:
	DEBUG_SHOW_HEX_RECV(p, 4);

	memcpy(&msg_options, p, 2); p += 2;
	msg_type = msg_options & SSP_SPI_MASK;
	memcpy(&chLength, p, 2); p += 2;
	pData = p; /* pData points current frame's data */
	if (msg_type == AP2HUB_READ || msg_type == HUB2AP_WRITE)
		p += chLength;	/* now p points next frame */

	nDataLen = q - pData;

	/* wait until we receive full frame */
	while (q < p && q < rBuff+sizeof(rBuff)) {
		iRet = bbd_pull_packet(q, rBuff+sizeof(rBuff)-q,
					BBD_PULL_TIMEOUT);
		if (iRet <= 0) {
			pr_err("[SSP]: %s bbd_pull_packet fail 2!! (iRet=%d)\n", __func__, iRet);
			return;
		}
		q += iRet;
		nDataLen += iRet;
	}

	switch (msg_type) {
	case AP2HUB_READ:
	case AP2HUB_WRITE:
		mutex_lock(&data->pending_mutex);
		if (!list_empty(&data->pending_list)) {
			struct ssp_msg *msg, *n;
			bool found = false;

			list_for_each_entry_safe(msg, n, &data->pending_list, list) {
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

			if (msg->buffer == NULL) {
				pr_err("[SSPBBD]: %s() : msg->buffer is NULL\n",
					__func__);
				goto exit;
			}
			if (msg_type == AP2HUB_READ) {
				if (nDataLen <= 0) {
					dprint("Waiting 2nd message...(msg=%pK, length=%d)\n",
						msg, msg->length);
					iRet = bbd_pull_packet(msg->buffer, msg->length,
								BBD_PULL_TIMEOUT);
					dprint("Received 2nd message. (iRet=%d)\n", iRet);
				} else {
					memcpy(msg->buffer, pData, msg->length);
					nDataLen -= msg->length;
				}
				DEBUG_SHOW_HEX_RECV(msg->buffer, msg->length);
			}
			if (msg_type == AP2HUB_WRITE) {
				iRet = bbd_send_packet(msg->buffer, msg->length);
				if (iRet <= 0) {
					pr_err("[SSP]: %s bbd_send_packet fail!!(AP2HUB_WRITE)\n",
						 __func__);
					goto exit;
				}

				DEBUG_SHOW_HEX_SEND(msg->buffer, msg->length);

				if (msg_options & AP2HUB_RETURN) {
					msg->options = AP2HUB_READ | AP2HUB_RETURN;
					msg->length = 1;
					list_add_tail(&msg->list, &data->pending_list);
					goto exit;
				}
			}
			if (msg->done != NULL && !completion_done(msg->done)) {
				dprint("complete(mg->done)\n");
				complete(msg->done);
			}
			if (msg->dead_hook != NULL)
				*(msg->dead_hook) = true;

			clean_msg(msg);
		} else
			pr_err("[SSP]List empty error(%d)\n", msg_type);
exit:
		mutex_unlock(&data->pending_mutex);
		break;
	case HUB2AP_WRITE:
		{
		char *buffer = kzalloc(chLength, GFP_KERNEL);

		if (nDataLen <= 0) {
			dprint("Waiting 2nd message...(chLength=%d)\n", chLength);
			iRet = bbd_pull_packet(buffer, chLength, BBD_PULL_TIMEOUT);
			dprint("Received 2nd message. (iRet=%d)\n",  iRet);
		} else {
			memcpy(buffer, pData, chLength);
			iRet = chLength;
		}
		DEBUG_SHOW_HEX_RECV(buffer, chLength);
		if (iRet < 0)
			pr_err("[SSP] %s bbd_pull_packet fail.(iRet=%d)\n", __func__, iRet);
		else {
			timestamp = get_current_timestamp();
			data->timestamp = timestamp;
			parse_dataframe(data, buffer, chLength);
		}
		kfree(buffer);
		break;
		}
	default:
		pr_err("[SSP]No type error(%d)\n", msg_type);
		break;
	}

	if (iRet < 0)
		pr_err("[SSP]: %s - MSG2SSP_SSD error %d\n", __func__, iRet);

	if (p < q)
		goto process_one;
}

#define reset_ssp_pkt() \
do { \
	ssp_pkt.rxlen = 0; \
	ssp_pkt.required = 4; \
	ssp_pkt.state = WAITFOR_PKT_HEADER; \
} while (0)

static inline struct ssp_msg *bbd_find_ssp_msg(struct ssp_data *data)
{
	struct ssp_msg *msg, *n;
	bool found = false;

	mutex_lock(&data->pending_mutex);
	if (list_empty(&data->pending_list)) {
		pr_err("[SSP] list empty error!\n");
	    mutex_unlock(&data->pending_mutex);
		goto errexit;
	}

	list_for_each_entry_safe(msg, n, &data->pending_list, list) {
		if (msg->options == ssp_pkt.opts) {
			list_del(&msg->list);
			found = true;
			break;
		}
	}
	mutex_unlock(&data->pending_mutex);

	if (!found) {
		pr_err("[SSP] %s %d - Not match error\n", __func__, ssp_pkt.opts);
		goto errexit;
	}

	/* For dead msg, make a temporary buffer to read. */
	if (msg->dead && !msg->free_buffer) {
		msg->buffer = kzalloc(msg->length, GFP_KERNEL);
		msg->free_buffer = 1;
	}
	if (msg->buffer == NULL) {
		pr_err("[SSPBBD]: %s() : msg->buffer is NULL\n", __func__);
		goto errexit;
	} else if (msg->length <= 0) {
		pr_err("[SSPBBD]: %s() : msg->length is less than 0\n", __func__);
		goto errexit;
	}
	return msg;

errexit:
	pr_err("[SSPBBD] %s opts:%d, state:%d, type:%d\n", __func__,
			ssp_pkt.opts, ssp_pkt.state, ssp_pkt.type);
	print_hex_dump(KERN_INFO, "[SSPBBD]: ",
			DUMP_PREFIX_NONE, 16, 1, ssp_pkt.buf, 4, true);
	reset_ssp_pkt();

	return NULL;
}

static inline void bbd_complete_ssp_msg(struct ssp_data *data, struct ssp_msg *msg)
{
	if (msg->done != NULL && !completion_done(msg->done)) {
		dprint("complete(mg->done)\n");
		complete(msg->done);
	}
	if (msg->dead_hook != NULL)
		*(msg->dead_hook) = true;

	clean_msg(msg);
}

static inline void bbd_send_timestamp(void)
{
		struct ssp_msg msg;
		unsigned char *bmsg = kzalloc(SSP_INSTRUCTION_PACKET + sizeof(s64), GFP_KERNEL);
		struct timespec curr_ts = ktime_to_timespec(ktime_get_boottime());
		s64 timestamp = timespec_to_ns(&curr_ts);

		memset(&msg, 0, sizeof(struct ssp_msg));
		msg.cmd = MSG2SSP_INST_CURRENT_TIMESTAMP; //you should modify this to proper command
		msg.options = AP2HUB_WRITE;
		msg.length = sizeof(s64);

		memcpy(bmsg, &msg, SSP_INSTRUCTION_PACKET);
		memcpy(bmsg + SSP_INSTRUCTION_PACKET, &timestamp, sizeof(s64));

		bbd_send_packet(bmsg, SSP_INSTRUCTION_PACKET + sizeof(s64));

		kfree(bmsg);
}

static int count = 1;

int callback_bbd_on_packet(void *ssh_data, const char *buf, size_t size)
{
	struct ssp_data *data = (struct ssp_data *)ssh_data;
	/*
	 *static char *str_state[] = {
	 *        "WAITFOR_PKT_HEADER",
	 *        "WAITFOR_PKT_COMPLETE"
	 *};
	 */

	int idx = 0;
	struct ssp_msg *msg;

	   /*
	    *printk("%s() sensor time delta:%lld\n", __func__,
	    *             get_sensor_time_delta_us());
	    */
	while (idx < size) {
		s32 remain = size - idx;
		s32 required = 0;

		/*
		 *printk("[SSPBBD] %s rxlen:%d, required:%d, remain:%d\n",
		 *        str_state[ssp_pkt.state], ssp_pkt.rxlen,
		 *        ssp_pkt.required, remain);
		 */

		required = (remain < ssp_pkt.required) ?
				remain : ssp_pkt.required;

		//BUG_ON(ssp_pkt.rxlen + required >= MAX_SSP_DATA_SIZE);
		if (ssp_pkt.rxlen + required >= MAX_SSP_DATA_SIZE)
			return -1;

		memcpy(&ssp_pkt.buf[ssp_pkt.rxlen], buf + idx, required);
		ssp_pkt.rxlen += required;
		ssp_pkt.required -= required;
		idx += required;

		//BUG_ON(ssp_pkt.required < 0);
		if (ssp_pkt.required)
			continue;

		switch (ssp_pkt.state) {
		case WAITFOR_PKT_HEADER:
			/* header completed */
			DEBUG_SHOW_HEX_RECV(ssp_pkt.buf, 4);
			memcpy(&ssp_pkt.opts, ssp_pkt.buf, 2);
			ssp_pkt.type = ssp_pkt.opts & SSP_SPI_MASK;

			switch (ssp_pkt.type) {
			case AP2HUB_READ:
				msg = bbd_find_ssp_msg(data);
				if (!msg)
					return -1;

				ssp_pkt.state = WAITFOR_PKT_COMPLETE;
				ssp_pkt.msg = msg;
				ssp_pkt.required = msg->length;
				break;
			case HUB2AP_WRITE:
				ssp_pkt.state = WAITFOR_PKT_COMPLETE;
				memcpy(&ssp_pkt.required, &ssp_pkt.buf[2], 2);
				if (count++ % 3 == 0)
					bbd_send_timestamp();
				break;
			case AP2HUB_WRITE:
				msg = bbd_find_ssp_msg(data);
				if (!msg)
					return -1;

				reset_ssp_pkt();
				if (bbd_send_packet(msg->buffer, msg->length) < 0) {
					pr_err("[SSP]: %s bbd_send_packet failed(AP2HUB_WRITE)\n", __func__);
                    break;
				}
				DEBUG_SHOW_HEX_SEND(msg->buffer, msg->length);
                mutex_lock(&data->pending_mutex);
				if (ssp_pkt.opts & AP2HUB_RETURN) {
					msg->options = AP2HUB_READ | AP2HUB_RETURN;
					msg->length = 1;
					list_add_tail(&msg->list, &data->pending_list);
				}
                else
                {
				    bbd_complete_ssp_msg(data, msg);
                }
                mutex_unlock(&data->pending_mutex);
				break;
			default:
				pr_err("[SSP]No type error(%d)\n", ssp_pkt.type);
				print_hex_dump(KERN_INFO, "[SSP] : ",
						DUMP_PREFIX_NONE, 16, 1, ssp_pkt.buf, 4, true);
				reset_ssp_pkt();
				return -1;
			}
			ssp_pkt.rxlen = 0;
			break;
		case WAITFOR_PKT_COMPLETE:
			/* packet completed!! */
			switch (ssp_pkt.type) {
			case AP2HUB_READ:
				//BUG_ON(ssp_pkt.msg == NULL);
				//BUG_ON(ssp_pkt.rxlen != ssp_pkt.msg->length);
				if (ssp_pkt.msg != NULL) {
                    mutex_lock(&data->pending_mutex);

					memcpy(ssp_pkt.msg->buffer, ssp_pkt.buf, ssp_pkt.rxlen);
					bbd_complete_ssp_msg(data, ssp_pkt.msg);

                    mutex_unlock(&data->pending_mutex);
					ssp_pkt.msg = NULL;
				}
				break;
			case HUB2AP_WRITE:
				DEBUG_SHOW_HEX_RECV(ssp_pkt.buf, ssp_pkt.rxlen);
				data->timestamp = get_current_timestamp();
				parse_dataframe(data, ssp_pkt.buf, ssp_pkt.rxlen);
				break;
			default:
				BUG_ON(true);
				break;
			}
			reset_ssp_pkt();
			break;
		}
	}
	return 0;
}
