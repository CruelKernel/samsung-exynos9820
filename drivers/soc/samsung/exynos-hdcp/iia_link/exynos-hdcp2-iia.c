/*
 * drivers/soc/samsung/exynos-hdcp/exynos-hdcp.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/smc.h>
#include <asm/cacheflush.h>
#include <linux/smc.h>
#include "exynos-hdcp2-iia-auth.h"
#include "../exynos-hdcp2-teeif.h"
#include "exynos-hdcp2-iia-selftest.h"
#include "../exynos-hdcp2-encrypt.h"
#include "../exynos-hdcp2-log.h"
#include "../exynos-hdcp2-session.h"
#include "../dp_link/exynos-hdcp2-dplink-if.h"
#include "../dp_link/exynos-hdcp2-dplink.h"
#include "../dp_link/exynos-hdcp2-dplink-selftest.h"
#define EXYNOS_HDCP_DEV_NAME	"hdcp2"

extern struct hdcp_session_list g_hdcp_session_list;
enum hdcp_result hdcp_link_ioc_authenticate(void);

int state_init_flag;

enum hdcp_result hdcp_unwrap_key(char *wkey)
{

	int rval = TX_AUTH_SUCCESS;

	rval = teei_wrapped_key(wkey, UNWRAP, HDCP_STATIC_KEY);
	if (rval < 0) {
		hdcp_err("Wrap(%d) key failed (0x%08x)\n", UNWRAP, rval);
		return HDCP_ERROR_UNWRAP_FAIL;
	}

	return 0;
}

enum hdcp_result hdcp_link_authenticate(struct hdcp_msg_info *msg_info)
{
	struct hdcp_session_node *ss_node;
	struct hdcp_link_node *lk_node;
	struct hdcp_link_data *lk_data;
	int ret = HDCP_SUCCESS;
	int rval = TX_AUTH_SUCCESS;
	int ake_retry = 0;
	int lc_retry = 0;

	/* find Session node which contains the Link */
	ss_node = hdcp_session_list_find(msg_info->ss_handle, &g_hdcp_session_list);
	if (!ss_node)
		return HDCP_ERROR_INVALID_INPUT;

	lk_node = hdcp_link_list_find(msg_info->lk_id, &ss_node->ss_data->ln);
	if (!lk_node)
		return HDCP_ERROR_INVALID_INPUT;

	lk_data = lk_node->lk_data;

	if (!lk_data)
		return HDCP_ERROR_INVALID_INPUT;

	/**
	 * if Upstream Content Control Function call this API,
	 * it changes state to ST_A0_DETERMINE_RX_HDCP_CAP automatically.
	 * HDCP library do not check CP desire.
	 */

	if (state_init_flag == 0){
		UPDATE_LINK_STATE(lk_data, LINK_ST_A0_DETERMINE_RX_HDCP_CAP);
	}

	if (lk_data->state == LINK_ST_A0_DETERMINE_RX_HDCP_CAP){
		if (determine_rx_hdcp_cap(lk_data) < 0) {
			ret = HDCP_ERROR_RX_NOT_HDCP_CAPABLE;
			UPDATE_LINK_STATE(lk_data, LINK_ST_H1_TX_LOW_VALUE_CONTENT);
		} else
			UPDATE_LINK_STATE(lk_data, LINK_ST_A1_EXCHANGE_MASTER_KEY);
	}

	switch (lk_data->state) {
		case LINK_ST_H1_TX_LOW_VALUE_CONTENT:
			break;
		case LINK_ST_A1_EXCHANGE_MASTER_KEY:
			rval = exchange_master_key(lk_data, msg_info);
			if (rval == TX_AUTH_SUCCESS) {
				if (msg_info->next_step == DONE) {
					ake_retry = 0;
					UPDATE_LINK_STATE(lk_data, LINK_ST_A2_LOCALITY_CHECK);
					msg_info->next_step = SEND_MSG;
					state_init_flag = 1;
				}
			} else {
				ret = HDCP_ERROR_EXCHANGE_KM;
				UPDATE_LINK_STATE(lk_data, LINK_ST_H1_TX_LOW_VALUE_CONTENT);
			}
			break;
		case LINK_ST_A2_LOCALITY_CHECK:
			rval = locality_check(lk_data, msg_info);
			if (rval == TX_AUTH_SUCCESS) {
				if (msg_info->next_step == DONE) {
					lc_retry = 0;
					UPDATE_LINK_STATE(lk_data, LINK_ST_A3_EXCHANGE_SESSION_KEY);
					msg_info->next_step = SEND_MSG;
				}
			} else {
				UPDATE_LINK_STATE(lk_data, LINK_ST_H1_TX_LOW_VALUE_CONTENT);
			}
			break;
		case LINK_ST_A3_EXCHANGE_SESSION_KEY:
			if (exchange_hdcp_session_key(lk_data, msg_info) < 0) {
				ret = HDCP_ERROR_EXCHANGE_KS;
				UPDATE_LINK_STATE(lk_data, LINK_ST_H1_TX_LOW_VALUE_CONTENT);
			} else{
				msg_info->next_step = WAIT_STATE;
				UPDATE_LINK_STATE(lk_data, LINK_ST_A4_TEST_REPEATER);
			}
			break;
		case LINK_ST_A4_TEST_REPEATER:
			if (evaluate_repeater(lk_data) == TRUE){
				/* HACK: when we supports repeater, it should be removed */
				UPDATE_LINK_STATE(lk_data, LINK_ST_A6_WAIT_RECEIVER_ID_LIST);
				msg_info->next_step = RP_RECIEVE_MSG;
			}
			else{
				/* if it is not a repeater, complete authentication */
				UPDATE_LINK_STATE(lk_data, LINK_ST_A5_AUTHENTICATED);
			}
			break;
		case LINK_ST_A5_AUTHENTICATED:
			msg_info->next_step = AUTH_FINISHED;
			state_init_flag = 0;
			return HDCP_SUCCESS;
		case LINK_ST_A6_WAIT_RECEIVER_ID_LIST:
			ret = wait_for_receiver_id_list(lk_data, msg_info);
			if (ret < 0) {
				ret = HDCP_ERROR_WAIT_RECEIVER_ID_LIST;
				UPDATE_LINK_STATE(lk_data, LINK_ST_H1_TX_LOW_VALUE_CONTENT);
			} else {
				UPDATE_LINK_STATE(lk_data, LINK_ST_A7_VERIFY_RECEIVER_ID_LIST);
				msg_info->next_step = WAIT_STATE;
			}
			break;
		case LINK_ST_A7_VERIFY_RECEIVER_ID_LIST:
			UPDATE_LINK_STATE(lk_data, LINK_ST_A8_SEND_RECEIVER_ID_LIST_ACK);
			msg_info->next_step = RP_SEND_MSG;
			break;
		case LINK_ST_A8_SEND_RECEIVER_ID_LIST_ACK:
			rval = send_receiver_id_list_ack(lk_data, msg_info);
			if (rval == TX_AUTH_SUCCESS) {
				if (msg_info->next_step == DONE) {
					UPDATE_LINK_STATE(lk_data, LINK_ST_A5_AUTHENTICATED);
					state_init_flag = 0;
					msg_info->next_step = AUTH_FINISHED;
				}
			} else {
				UPDATE_LINK_STATE(lk_data, LINK_ST_H1_TX_LOW_VALUE_CONTENT);
			}
			break;
		case LINK_ST_A9_CONTENT_STREAM_MGT:
			/* do not support yet */
			ret = HDCP_ERROR_DO_NOT_SUPPORT_YET;
			break;
		default:
			ret = HDCP_ERROR_INVALID_STATE;
			UPDATE_LINK_STATE(lk_data, LINK_ST_H1_TX_LOW_VALUE_CONTENT);
			break;
	}

	return ret;
}

enum hdcp_result hdcp_link_stream_manage(struct hdcp_stream_info *stream_info)
{
	struct hdcp_session_node *ss_node;
	struct hdcp_link_node *lk_node;
	struct hdcp_link_data *lk_data;
	struct contents_info stream_ctrl;
	int rval = TX_AUTH_SUCCESS;
	int i;

	/* find Session node which contain the Link */
	ss_node = hdcp_session_list_find(stream_info->ss_id, &g_hdcp_session_list);
	if (!ss_node)
		return HDCP_ERROR_INVALID_INPUT;

	lk_node = hdcp_link_list_find(stream_info->lk_id, &ss_node->ss_data->ln);
	if (!lk_node)
		return HDCP_ERROR_INVALID_INPUT;

	lk_data = lk_node->lk_data;
	if (!lk_data)
		return HDCP_ERROR_INVALID_INPUT;

	if (lk_data->state < LINK_ST_A4_TEST_REPEATER)
		return HDCP_ERROR_INVALID_STATE;

	stream_ctrl.str_num = stream_info->num;
	if (stream_info->num > HDCP_TX_REPEATER_MAX_STREAM)
		return HDCP_ERROR_INVALID_INPUT;

	for (i = 0; i < stream_info->num; i++) {
		stream_ctrl.str_info[i].ctr = stream_info->stream_ctr;
		stream_ctrl.str_info[i].type = stream_info->type;
		stream_ctrl.str_info[i].pid = stream_info->stream_pid;
	}

	rval = manage_content_stream(lk_data, &stream_ctrl, stream_info);
	if (rval < 0) {
		hdcp_err("manage_content_stream fail(0x%08x)\n", rval);
		return HDCP_ERROR_STREAM_MANAGE;

	}

	return HDCP_SUCCESS;
}

enum hdcp_result hdcp_wrap_key(struct hdcp_wrapped_key *key_info)
{
	int rval = TX_AUTH_SUCCESS;
	char key_str[HDCP_WRAP_MAX_SIZE];

	if (key_info->key_len <= HDCP_WRAP_KEY)
		memcpy(key_str, key_info->key, HDCP_WRAP_KEY);
	else
		return HDCP_ERROR_WRONG_SIZE;

	rval = teei_wrapped_key(key_str, key_info->wrapped, key_info->key_len);
	if (rval < 0) {
		hdcp_err("Wrap(%d) key failed (0x%08x)\n",key_info->wrapped, rval);
		return HDCP_ERROR_WRAP_FAIL;
	}

	if (key_info->wrapped == WRAP)
		memcpy(key_info->enc_key, key_str, HDCP_WRAP_KEY + HDCP_WRAP_AUTH_TAG);

	return 0;
}
